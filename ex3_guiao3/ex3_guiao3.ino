/**
 * Controle da Comporta com Leitura Contínua, Emergência, Sensor de Temperatura
 * e Sistema de Alimentação Automática
 * Sistema de Represa para Peixes
 */

#include <Stepper.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Servo.h>  // Biblioteca para o servo motor

// Pinos do Joystick
#define JOYSTICK_X A1
#define JOYSTICK_Y A0

// Pinos do Encoder 
#define ENCODER_SW A3    // Botão do encoder (push button)

// Pinos do Sensor Ultrassônico
#define TRIGGER_PIN 11
#define ECHO_PIN 12

// Pino do Servo Motor
#define SERVO_PIN 10

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
#define ENCODER_DEBOUNCE 300

// Constantes para alimentação dos peixes
#define FEEDING_INTERVAL 30000      // Intervalo entre alimentações (30 segundos)
#define FEEDING_DURATION 3000       // Duração da alimentação (3 segundos)
#define SERVO_CLOSED_POS 0          // Posição do servo fechado (graus)
#define SERVO_OPEN_POS 90           // Posição do servo aberto (graus)

// Objetos
Stepper stepperMotor(STEPS_PER_REVOLUTION, MOTOR_PIN1, MOTOR_PIN3, MOTOR_PIN2, MOTOR_PIN4);
LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);
OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature sensors(&oneWire);
Servo feedingServo;                 // Novo objeto para o servo motor

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
unsigned long lastEncoderButtonTime = 0;
bool emergencyActive = false;
unsigned long emergencyStartTime = 0;
unsigned long tempAlarmStartTime = 0;

// Variáveis do encoder
int currentScreen = 0;  // 0 = Altura da comporta, 1 = Temperatura, 2 = Alimentação
#define NUM_SCREENS 3   // Número total de telas disponíveis

// Variáveis para alimentação
unsigned long lastFeedingTime = 0;  // Último momento em que os peixes foram alimentados
bool feedingActive = false;         // Indica se a alimentação está em andamento
unsigned long feedingStartTime = 0; // Tempo de início da alimentação

void setup() {
  Serial.begin(9600);
  Serial.println("Sistema de Controle da Comporta - Com Alimentação Automatica");
  
  // Inicializa pinos do ultrassom
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Inicializa pino do botão do encoder
  pinMode(ENCODER_SW, INPUT_PULLUP);
  
  // Inicializa os pinos dos LEDs
  for (int i = 0; i < 8; i++) {
    if (ledPins[i] != ENCODER_SW && ledPins[i] != SERVO_PIN) {
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
  
  // Inicializa o servo motor
  feedingServo.attach(SERVO_PIN);
  feedingServo.write(SERVO_CLOSED_POS);  // Inicia na posição fechada
  
  // Inicializa a última alimentação
  lastFeedingTime = millis();
  
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
  lcd.print("Pressione botao");
  lcd.setCursor(0, 1);
  lcd.print("para navegar");
  delay(1500);
  
  // Exibe a tela inicial
  updateScreenContent();
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Verifica o botão do encoder para mudar tela
  checkEncoderButton();
  
  // Verifica se é hora de alimentar os peixes
  checkFeedingTime(currentMillis);
  
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
  if (!emergencyActive && !tempAlarmActive && !feedingActive && digitalRead(PUSH_BUTTON_PIN) == LOW && currentMillis - lastButtonTime > 200) {
    lastButtonTime = currentMillis;
    activateEmergency();
  }
  
  // Processa estados prioritários
  if (emergencyActive) {
    handleEmergency(currentMillis);
  } else if (tempAlarmActive) {
    handleTempAlarm(currentMillis);
  } else if (feedingActive) {
    handleFeeding(currentMillis);
  } else {
    // Operação normal
    // Lê o sensor ultrassônico continuamente
    readUltrasonicSensor();
    
    // Lê o joystick e controla o motor
    if (currentMillis - lastJoystickRead >= 100) {
      handleJoystick();
      lastJoystickRead = currentMillis;
    }
    
    // Atualiza o display regularmente
    if (currentMillis - lastDisplayUpdate >= 200) {
      updateScreenContent();
      lastDisplayUpdate = currentMillis;
    }
  }
  
  // Atualiza os LEDs
  updateLEDs();
}

// Nova função: Verifica se é hora de alimentar os peixes
void checkFeedingTime(unsigned long currentTime) {
  // Se não estiver já em algum estado de alarme ou alimentação
  if (!feedingActive && !emergencyActive && !tempAlarmActive) {
    // Verifica se o intervalo de alimentação passou
    if (currentTime - lastFeedingTime >= FEEDING_INTERVAL) {
      startFeeding();
    }
  }
}

// Nova função: Inicia o processo de alimentação
void startFeeding() {
  feedingActive = true;
  feedingStartTime = millis();
  
  // Abre a porta de alimentação
  feedingServo.write(SERVO_OPEN_POS);
  
  // Atualiza o display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Peixes sendo");
  lcd.setCursor(0, 1);
  lcd.print("alimentados!");
  
  Serial.println("Iniciando alimentação dos peixes");
}

// Nova função: Gerencia o processo de alimentação
void handleFeeding(unsigned long currentTime) {
  // Se o tempo de alimentação acabou
  if (currentTime - feedingStartTime >= FEEDING_DURATION) {
    // Fecha a porta de alimentação
    feedingServo.write(SERVO_CLOSED_POS);
    
    // Atualiza variáveis
    feedingActive = false;
    lastFeedingTime = currentTime;
    
    // Restaura a visualização da tela atual
    updateScreenContent();
    
    Serial.println("Alimentação finalizada");
  }
}

// Verifica o botão do encoder para mudar de tela
void checkEncoderButton() {
  // Verifica se o botão do encoder foi pressionado (com debounce)
  if (digitalRead(ENCODER_SW) == LOW) {
    unsigned long currentTime = millis();
    
    // Se passou tempo suficiente desde o último clique (debounce)
    if (currentTime - lastEncoderButtonTime > ENCODER_DEBOUNCE) {
      lastEncoderButtonTime = currentTime;
      
      // Avança para a próxima tela
      currentScreen = (currentScreen + 1) % NUM_SCREENS;
      
      Serial.print("Botão do encoder pressionado. Tela atual: ");
      Serial.println(currentScreen);
      
      // Atualiza o display imediatamente com a nova tela
      updateScreenContent();
      
      // Pequeno delay adicional para evitar múltiplos triggers
      delay(50);
    }
  }
}

// Atualiza o conteúdo da tela com base na tela selecionada
void updateScreenContent() {
  // Se estiver em alimentação, emergência ou alarme, não atualiza
  if (feedingActive || emergencyActive || tempAlarmActive) {
    return;
  }
  
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
      
    case 2:  // Nova tela de alimentação
      lcd.setCursor(0, 0);
      lcd.print("Prox Alimentacao:");
      
      // Calcula o tempo até a próxima alimentação
      unsigned long currentTimeMs = millis();
      unsigned long timeElapsed = currentTimeMs - lastFeedingTime;
      
      if (timeElapsed < FEEDING_INTERVAL) {
        // Converte para segundos e formata
        unsigned long timeRemaining = (FEEDING_INTERVAL - timeElapsed) / 1000; // em segundos
        unsigned long secondsRemaining = timeRemaining % 60;
        unsigned long minutesRemaining = (timeRemaining / 60) % 60;
        
        lcd.setCursor(0, 1);
        lcd.print("Em ");
        
        if (minutesRemaining > 0) {
          lcd.print(minutesRemaining);
          lcd.print("m ");
        }
        
        lcd.print(secondsRemaining);
        lcd.print("s");
      } else {
        lcd.setCursor(0, 1);
        lcd.print("AGORA!");
      }
      break;
      
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
  // Se estiver em estado de erro, emergência, alimentação ou alarme, pisque todos os LEDs
  if (errorState || emergencyActive || tempAlarmActive || feedingActive) {
    // Faz os LEDs piscarem
    if ((millis() / 500) % 2 == 0) {
      for (int i = 0; i < 8; i++) {
        if (ledPins[i] != ENCODER_SW && ledPins[i] != SERVO_PIN) {
          digitalWrite(ledPins[i], HIGH);
        }
      }
    } else {
      for (int i = 0; i < 8; i++) {
        if (ledPins[i] != ENCODER_SW && ledPins[i] != SERVO_PIN) {
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
    // Não mexe nos LEDs que estão compartilhando pinos com outros componentes
    if (ledPins[i] != ENCODER_SW && ledPins[i] != SERVO_PIN) {
      digitalWrite(ledPins[i], (i < ledsToLight) ? HIGH : LOW);
    }
  }
}