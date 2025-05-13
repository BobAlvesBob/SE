/**
 * Controle da Comporta com Leitura Contínua, Emergência, Sensor de Temperatura
 * e Navegação por Encoder
 * Sistema de Represa para Peixes
 */

#include <Stepper.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Pinos do Joystick
#define JOYSTICK_X A1
#define JOYSTICK_Y A0

// Pinos do Encoder (corrigidos para Arduino UNO)
#define ENCODER_CLK 7    // Pino digital 7 (era D7)
#define ENCODER_DT A2    // Pino analógico A2
#define ENCODER_SW A3    // Pino analógico A3

// Pinos do Sensor Ultrassônico
#define TRIGGER_PIN 11
#define ECHO_PIN 12

// Pinos do Motor de Passo
#define MOTOR_PIN1 2
#define MOTOR_PIN2 4
#define MOTOR_PIN3 5
#define MOTOR_PIN4 6

// Pinos para Botão, Buzzer e Sensor de Temperatura
#define PUSH_BUTTON_PIN 8
#define BUZZER_PIN 3
#define TEMP_SENSOR_PIN 9

// Pinos dos LEDs
#define LED1 A2
#define LED2 A3
#define LED3 7
#define LED4 10
#define LED5 9
#define LED6 13
#define LED7 A4
#define LED8 A5

// Constantes do sistema
#define STEPS_PER_REVOLUTION 2048
#define STEPS_PER_CYCLE 32
#define MAX_HEIGHT 100.0
#define MIN_HEIGHT 0.0
#define HEIGHT_PER_LED 12.5
#define LCD_ADDRESS 0x27
#define MIN_SAFE_DISTANCE 10.0
#define EMERGENCY_DURATION 1000
#define MIN_SAFE_TEMP 10.0
#define MAX_SAFE_TEMP 30.0
#define TEMP_CHECK_INTERVAL 2000

// Objetos
Stepper stepperMotor(STEPS_PER_REVOLUTION, MOTOR_PIN1, MOTOR_PIN3, MOTOR_PIN2, MOTOR_PIN4);
LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);
OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature sensors(&oneWire);

// Variáveis globais
float gateHeight = 0.0;
float rawDistance = 0.0;
float waterTemperature = 20.0;
int ledPins[] = {LED1, LED2, LED3, LED4, LED5, LED6, LED7, LED8};
bool errorState = false;
bool tempAlarmActive = false;
unsigned long lastJoystickRead = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastTempRead = 0;
unsigned long lastButtonTime = 0;
unsigned long lastEncoderRead = 0;
bool emergencyActive = false;
unsigned long emergencyStartTime = 0;
unsigned long tempAlarmStartTime = 0;

// Variáveis do encoder
int encoderPos = 0;
int lastEncoderCLK;
int currentScreen = 0;  // 0 = Altura da comporta, 1 = Temperatura, etc.
#define NUM_SCREENS 2   // Número total de telas disponíveis

// ATENÇÃO: Existe conflito de pinos entre o encoder e os LEDs!
// LED1 (A2) conflita com ENCODER_DT (A2)
// LED3 (7) conflita com ENCODER_CLK (7)
// Para uso prático, desabilitar esses LEDs ou usar pinos diferentes

void setup() {
  Serial.begin(9600);
  Serial.println("Sistema de Controle da Comporta - Com Navegação por Encoder");
  
  // Inicializa pinos do ultrassom
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Inicializa pinos do encoder
  pinMode(ENCODER_CLK, INPUT);
  pinMode(ENCODER_DT, INPUT);
  pinMode(ENCODER_SW, INPUT_PULLUP);  // Botão do encoder com pull-up
  lastEncoderCLK = digitalRead(ENCODER_CLK);
  
  // Inicializa os pinos dos LEDs - NOTA: Conflito de pinos com encoder
  // Ajuste conforme necessário
  for (int i = 0; i < 8; i++) {
    if (ledPins[i] != ENCODER_CLK && ledPins[i] != ENCODER_DT && ledPins[i] != ENCODER_SW) {
      pinMode(ledPins[i], OUTPUT);
    }
  }
  
  // Inicializações para botão e buzzer
  pinMode(PUSH_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Inicializa o LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Iniciando...");
  
  // Inicializa o sensor de temperatura
  sensors.begin();
  
  // Define a velocidade do motor de passo
  stepperMotor.setSpeed(10);
  
  // Faz a primeira leitura do ultrassom
  readUltrasonicSensor();
  
  // Faz a primeira leitura de temperatura
  readTemperature();
  
  // Atualiza os LEDs e display
  updateLEDs();
  updateScreenContent();
  
  // Mensagem de inicialização
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Use o encoder");
  lcd.setCursor(0, 1);
  lcd.print("para navegar");
  delay(1500);
  
  // Exibe a tela inicial
  updateScreenContent();
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Lê o encoder a cada 5ms
  if (currentMillis - lastEncoderRead >= 5) {
    readEncoder();
    lastEncoderRead = currentMillis;
  }
  
  // Lê o sensor de temperatura a cada TEMP_CHECK_INTERVAL
  if (currentMillis - lastTempRead >= TEMP_CHECK_INTERVAL) {
    readTemperature();
    lastTempRead = currentMillis;
    
    // Verifica se a temperatura está fora dos limites seguros
    if ((waterTemperature < MIN_SAFE_TEMP || waterTemperature > MAX_SAFE_TEMP) && !tempAlarmActive && !emergencyActive) {
      activateTempAlarm();
    }
  }
  
  // Verifica o botão de emergência
  if (!emergencyActive && !tempAlarmActive && digitalRead(PUSH_BUTTON_PIN) == LOW && currentMillis - lastButtonTime > 200) {
    lastButtonTime = currentMillis;
    activateEmergency();
  }
  
  // Processa alarmes ativos
  if (emergencyActive) {
    handleEmergency(currentMillis);
  } else if (tempAlarmActive) {
    handleTempAlarm(currentMillis);
  } else {
    // Lê o sensor ultrassônico continuamente
    readUltrasonicSensor();
    
    // Lê o joystick e controla o motor
    if (currentMillis - lastJoystickRead >= 100) {
      handleJoystick();
      lastJoystickRead = currentMillis;
    }
  }
  
  // Atualiza os LEDs
  updateLEDs();
  
  // Atualiza o display regularmente
  if (currentMillis - lastDisplayUpdate >= 200 && !emergencyActive && !tempAlarmActive) {
    updateScreenContent();
    lastDisplayUpdate = currentMillis;
  }
}

// Nova função: Lê o encoder rotativo
void readEncoder() {
  // Lê o estado atual do pino CLK do encoder
  int currentCLK = digitalRead(ENCODER_CLK);
  
  // Se houve uma mudança no estado do CLK (mudança de LOW para HIGH ou vice-versa)
  if (currentCLK != lastEncoderCLK) {
    // Se o pino DT tem um estado diferente do CLK, estamos girando no sentido horário
    if (digitalRead(ENCODER_DT) != currentCLK) {
      // Incrementa a posição (sentido horário)
      encoderPos++;
      // Muda para a próxima tela
      currentScreen = (currentScreen + 1) % NUM_SCREENS;
      
      Serial.print("Encoder girado no sentido horário. Tela atual: ");
      Serial.println(currentScreen);
    } else {
      // Decrementa a posição (sentido anti-horário)
      encoderPos--;
      // Muda para a tela anterior
      currentScreen = (currentScreen + NUM_SCREENS - 1) % NUM_SCREENS;
      
      Serial.print("Encoder girado no sentido anti-horário. Tela atual: ");
      Serial.println(currentScreen);
    }
    
    // Atualiza o display imediatamente com a nova tela
    updateScreenContent();
  }
  
  // Verifica se o botão do encoder foi pressionado
  if (digitalRead(ENCODER_SW) == LOW) {
    // Função para o botão do encoder (pode ser usado para expandir funcionalidades)
    Serial.println("Botão do encoder pressionado");
    delay(300);  // Debounce simples
  }
  
  // Salva o estado atual do CLK para a próxima comparação
  lastEncoderCLK = currentCLK;
}

// Nova função: Atualiza o conteúdo da tela com base na tela selecionada
void updateScreenContent() {
  // Limpa o LCD
  lcd.clear();
  
  // Exibe o conteúdo com base na tela selecionada
  switch (currentScreen) {
    case 0:  // Tela de altura da comporta
      lcd.setCursor(0, 0);
      lcd.print("Altura Comporta:");
      lcd.setCursor(0, 1);
      lcd.print(gateHeight, 1);
      lcd.print(" cm");
      
      // Indicador visual de posição
      lcd.setCursor(10, 1);
      if (gateHeight >= 75) {
        lcd.print("[FECHADA]");
      } else if (gateHeight <= 25) {
        lcd.print("[ABERTA] ");
      } else {
        lcd.print("[PARCIAL]");
      }
      break;
      
    case 1:  // Tela de temperatura
      lcd.setCursor(0, 0);
      lcd.print("Temperatura Agua:");
      lcd.setCursor(0, 1);
      lcd.print(waterTemperature, 1);
      lcd.print(" C");
      
      // Indicador de status da temperatura
      lcd.setCursor(10, 1);
      if (waterTemperature < MIN_SAFE_TEMP) {
        lcd.print("[FRIA]");
      } else if (waterTemperature > MAX_SAFE_TEMP) {
        lcd.print("[QUENTE]");
      } else {
        lcd.print("[NORMAL]");
      }
      break;
      
    // Aqui podem ser adicionadas mais telas no futuro
    
    default:
      // Caso de erro, voltar para a primeira tela
      currentScreen = 0;
      updateScreenContent();
      break;
  }
}

// Lê a temperatura da água
void readTemperature() {
  sensors.requestTemperatures();
  float tempReading = sensors.getTempCByIndex(0);
  
  // Verifica se a leitura é válida
  if (tempReading != DEVICE_DISCONNECTED_C && tempReading > -127) {
    waterTemperature = tempReading;
    Serial.print("Temperatura da água: ");
    Serial.print(waterTemperature);
    Serial.println(" °C");
  } else {
    Serial.println("Erro ao ler o sensor de temperatura");
  }
}

// Ativa o alarme de temperatura
void activateTempAlarm() {
  tempAlarmActive = true;
  tempAlarmStartTime = millis();
  
  Serial.println("ALERTA: Temperatura fora dos limites seguros!");
  
  // Atualiza o display para mostrar alerta de temperatura
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ALERTA TEMP!");
  lcd.setCursor(0, 1);
  lcd.print(waterTemperature, 1);
  lcd.print(" C - PERIGO!");
  
  // Liga o buzzer
  digitalWrite(BUZZER_PIN, HIGH);
}

// Gerencia o alarme de temperatura
void handleTempAlarm(unsigned long currentTime) {
  // Se o tempo de alarme acabou (1 segundo)
  if (currentTime - tempAlarmStartTime >= EMERGENCY_DURATION) {
    // Desliga o buzzer
    digitalWrite(BUZZER_PIN, LOW);
    
    // Finaliza o estado de alarme
    tempAlarmActive = false;
    
    // Restaura a tela atual
    updateScreenContent();
    
    Serial.println("Alarme de temperatura finalizado");
  }
}

// Ativa o modo de emergência
void activateEmergency() {
  emergencyActive = true;
  emergencyStartTime = millis();
  
  Serial.println("EMERGÊNCIA: Abrindo comporta!");
  
  // Atualiza o display para mostrar EMERGÊNCIA
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("EMERGENCIA");
  lcd.setCursor(0, 1);
  lcd.print("Abrindo comporta!");
  
  // Liga o buzzer
  digitalWrite(BUZZER_PIN, HIGH);
}

// Processa o estado de emergência
void handleEmergency(unsigned long currentTime) {
  // Se o tempo de emergência acabou
  if (currentTime - emergencyStartTime >= EMERGENCY_DURATION) {
    // Desliga o buzzer
    digitalWrite(BUZZER_PIN, LOW);
    
    // Finaliza o estado de emergência
    emergencyActive = false;
    
    // Restaura a visualização da tela atual
    updateScreenContent();
    
    Serial.println("Emergência finalizada");
    return;
  }
  
  // Durante a emergência, move o motor para abrir a comporta
  stepperMotor.step(-5);  // Move pequenos passos continuamente para abrir
}

// Lê o sensor ultrassônico e calcula a altura da comporta
void readUltrasonicSensor() {
  // Gera um pulso curto no pino Trigger
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  
  // Lê o tempo que o pulso levou para retornar ao sensor
  long duration = pulseIn(ECHO_PIN, HIGH);
  
  // Calcula a distância em centímetros
  rawDistance = duration * 0.0343 / 2;
  
  // Verifica se a distância está abaixo da distância mínima de segurança
  if (rawDistance < MIN_SAFE_DISTANCE) {
    // ERRO: Distância menor que o mínimo de segurança
    errorState = true;
    Serial.println("ERRO: Altura da comporta excedida! Distância abaixo do mínimo de segurança.");
    
    // Não altera a altura aqui, mantém a altura máxima
    gateHeight = MAX_HEIGHT;
  } 
  else if (rawDistance > 110) {
    // Se a distância for maior que o curso total + margem
    gateHeight = MIN_HEIGHT;
    errorState = false;
  } 
  else {
    // Cálculo normal: Altura = Máximo - (Distância - Mínimo de Segurança)
    gateHeight = MAX_HEIGHT - (rawDistance - MIN_SAFE_DISTANCE);
    
    // Limita a valores válidos
    if (gateHeight < MIN_HEIGHT) gateHeight = MIN_HEIGHT;
    if (gateHeight > MAX_HEIGHT) gateHeight = MAX_HEIGHT;
    
    errorState = false;
    
    Serial.print("Dist: ");
    Serial.print(rawDistance);
    Serial.print("cm, Alt: ");
    Serial.print(gateHeight);
    Serial.println("cm");
  }
}

// Lê o joystick e controla o motor de passo
void handleJoystick() {
  int joystickValue = analogRead(JOYSTICK_Y);
  
  // Zona morta central (não faz nada)
  if (joystickValue > 400 && joystickValue < 600) {
    return;
  }
  
  int stepsToMove = 0;
  
  // Determina a direção e o número de passos
  if (joystickValue >= 600) {  // Joystick para cima - fecha a comporta
    // Só permite fechar se não estiver em erro ou no limite máximo
    if (!errorState && rawDistance > MIN_SAFE_DISTANCE) {
      stepsToMove = STEPS_PER_CYCLE;
    } else {
      Serial.println("AVISO: Não pode subir mais! Distância mínima de segurança atingida.");
    }
  } 
  else if (joystickValue <= 400) {  // Joystick para baixo - abre a comporta
    // Só permite abrir se não estiver no limite mínimo
    if (gateHeight > MIN_HEIGHT) {
      stepsToMove = -STEPS_PER_CYCLE;
    } else {
      Serial.println("AVISO: Comporta já está totalmente aberta!");
    }
  }
  
  // Move o motor se necessário
  if (stepsToMove != 0) {
    Serial.print("Movendo motor: ");
    Serial.print(stepsToMove);
    Serial.println(" passos");
    
    // Move o motor de passo
    stepperMotor.step(stepsToMove);
  }
}

// Atualiza os LEDs para mostrar a altura da comporta
void updateLEDs() {
  // Se estiver em estado de erro, emergência ou alarme de temperatura, pisque todos os LEDs
  if (errorState || emergencyActive || tempAlarmActive) {
    // Faz os LEDs piscarem para indicar erro/emergência
    if ((millis() / 500) % 2 == 0) {
      for (int i = 0; i < 8; i++) {
        if (ledPins[i] != ENCODER_CLK && ledPins[i] != ENCODER_DT && ledPins[i] != ENCODER_SW) {
          digitalWrite(ledPins[i], HIGH);
        }
      }
    } else {
      for (int i = 0; i < 8; i++) {
        if (ledPins[i] != ENCODER_CLK && ledPins[i] != ENCODER_DT && ledPins[i] != ENCODER_SW) {
          digitalWrite(ledPins[i], LOW);
        }
      }
    }
    return;
  }
  
  // Operação normal - calcula quantos LEDs devem estar acesos
  int ledsToLight = round(gateHeight / HEIGHT_PER_LED);
  ledsToLight = constrain(ledsToLight, 0, 8);
  
  // Atualiza cada LED
  for (int i = 0; i < 8; i++) {
    // Não mexe nos LEDs que estão compartilhando pinos com o encoder
    if (ledPins[i] != ENCODER_CLK && ledPins[i] != ENCODER_DT && ledPins[i] != ENCODER_SW) {
      digitalWrite(ledPins[i], (i < ledsToLight) ? HIGH : LOW);
    }
  }
}