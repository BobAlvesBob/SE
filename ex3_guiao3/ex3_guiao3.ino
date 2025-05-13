/**
 * Controle da Comporta com Leitura Contínua e Mensagem de Erro
 * Sistema de Represa para Peixes
 * Com função de emergência por botão
 */

#include <Stepper.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Pinos do Joystick
#define JOYSTICK_X A1
#define JOYSTICK_Y A0

// Pinos do Sensor Ultrassônico
#define TRIGGER_PIN 11
#define ECHO_PIN 12

// Pinos do Motor de Passo
#define MOTOR_PIN1 2
#define MOTOR_PIN2 4
#define MOTOR_PIN3 5
#define MOTOR_PIN4 6

// NOVOS PINOS PARA BOTÃO E BUZZER
#define PUSH_BUTTON_PIN 8    // Pino do pushbutton
#define BUZZER_PIN 3         // Pino do buzzer

// Pinos dos LEDs (AJUSTADO para resolver conflito com o botão)
#define LED1 A2
#define LED2 A3
#define LED3 7
#define LED4 10              // Alterado de 8 para 10 para evitar conflito com o botão
#define LED5 9
#define LED6 13
#define LED7 A4
#define LED8 A5

// Constantes do sistema
#define STEPS_PER_REVOLUTION 2048      // Passos por revolução do motor 28BYJ-48
#define STEPS_PER_CYCLE 32             // Passos por ciclo de leitura do joystick
#define MAX_HEIGHT 100.0               // Altura máxima (cm) - comporta fechada
#define MIN_HEIGHT 0.0                 // Altura mínima (cm) - comporta aberta
#define HEIGHT_PER_LED 12.5            // Altura por LED (100/8 cm)
#define LCD_ADDRESS 0x27               // Endereço I2C do LCD
#define MIN_SAFE_DISTANCE 10.0         // Distância mínima de segurança (10 cm)
#define EMERGENCY_DURATION 1000        // Duração da emergência em milissegundos (1 segundo)

// Objetos
Stepper stepperMotor(STEPS_PER_REVOLUTION, MOTOR_PIN1, MOTOR_PIN3, MOTOR_PIN2, MOTOR_PIN4);
LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);  // LCD 16x2

// Variáveis globais
float gateHeight = 100.0;              // Altura inicial da comporta (cm)
float rawDistance = 0.0;               // Distância bruta medida pelo sensor
int ledPins[] = {LED1, LED2, LED3, LED4, LED5, LED6, LED7, LED8};
bool errorState = false;               // Indica se há um erro de altura excedida
unsigned long lastJoystickRead = 0;    // Última leitura do joystick
unsigned long lastDisplayUpdate = 0;   // Última atualização do display
unsigned long lastButtonTime = 0;      // Para debounce do botão
bool emergencyActive = false;          // Indica se emergência está ativa
unsigned long emergencyStartTime = 0;  // Tempo de início da emergência

void setup() {
  Serial.begin(9600);
  Serial.println("Sistema de Controle da Comporta - Com Emergência");
  
  // Inicializa pinos do ultrassom
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Inicializa os pinos dos LEDs
  for (int i = 0; i < 8; i++) {
    pinMode(ledPins[i], OUTPUT);
  }
  
  // NOVAS INICIALIZAÇÕES
  pinMode(PUSH_BUTTON_PIN, INPUT_PULLUP);  // Botão com resistor pull-up
  pinMode(BUZZER_PIN, OUTPUT);             // Buzzer como saída
  digitalWrite(BUZZER_PIN, LOW);           // Garante que o buzzer comece desligado
  
  // Inicializa o LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Altura Comporta:");
  
  // Define a velocidade do motor de passo
  stepperMotor.setSpeed(10);  // 10 RPM
  
  // Faz a primeira leitura do ultrassom
  readUltrasonicSensor();
  
  // Atualiza os LEDs e display
  updateLEDs();
  updateDisplay();
}

void loop() {
  unsigned long currentMillis = millis();
  
  // VERIFICA O BOTÃO DE EMERGÊNCIA (com debounce)
  if (!emergencyActive && digitalRead(PUSH_BUTTON_PIN) == LOW && currentMillis - lastButtonTime > 200) {
    lastButtonTime = currentMillis;
    activateEmergency();
  }
  
  // PROCESSA A EMERGÊNCIA SE ESTIVER ATIVA
  if (emergencyActive) {
    handleEmergency(currentMillis);
  } else {
    // Lê o sensor ultrassônico CONTINUAMENTE
    readUltrasonicSensor();
    
    // Lê o joystick e controla o motor
    if (currentMillis - lastJoystickRead >= 100) {
      handleJoystick();
      lastJoystickRead = currentMillis;
    }
  }
  
  // Atualiza os LEDs continuamente com base na altura calculada
  updateLEDs();
  
  // Atualiza o display regularmente (menor frequência para evitar flicker)
  if (currentMillis - lastDisplayUpdate >= 200) {
    updateDisplay();
    lastDisplayUpdate = currentMillis;
  }
}

// NOVA FUNÇÃO: Ativa o modo de emergência
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

// NOVA FUNÇÃO: Processa o estado de emergência
void handleEmergency(unsigned long currentTime) {
  // Se o tempo de emergência acabou
  if (currentTime - emergencyStartTime >= EMERGENCY_DURATION) {
    // Desliga o buzzer
    digitalWrite(BUZZER_PIN, LOW);
    
    // Atualiza o estado da comporta após a emergência
    readUltrasonicSensor();
    
    // Finaliza o estado de emergência
    emergencyActive = false;
    
    // Retorna à exibição normal
    updateDisplay();
    
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
  // Se estiver em estado de erro ou emergência, pisque todos os LEDs
  if (errorState || emergencyActive) {
    // Faz os LEDs piscarem para indicar erro/emergência
    if ((millis() / 500) % 2 == 0) {
      for (int i = 0; i < 8; i++) {
        digitalWrite(ledPins[i], HIGH);
      }
    } else {
      for (int i = 0; i < 8; i++) {
        digitalWrite(ledPins[i], LOW);
      }
    }
    return;
  }
  
  // Operação normal - calcula quantos LEDs devem estar acesos
  int ledsToLight = round(gateHeight / HEIGHT_PER_LED);
  ledsToLight = constrain(ledsToLight, 0, 8);
  
  // Atualiza cada LED
  for (int i = 0; i < 8; i++) {
    if (i < ledsToLight) {
      digitalWrite(ledPins[i], HIGH);  // Liga o LED
    } else {
      digitalWrite(ledPins[i], LOW);   // Desliga o LED
    }
  }
}

// Atualiza o display LCD
void updateDisplay() {
  // Se estiver em emergência, não atualiza o display aqui
  if (emergencyActive) {
    return;
  }
  
  // Limpa a linha
  lcd.setCursor(0, 1);
  lcd.print("                ");
  
  // Se estiver em estado de erro
  if (errorState) {
    lcd.setCursor(0, 1);
    lcd.print("ERRO: ALT EXCED!");
    return;
  }
  
  // Exibição normal
  lcd.setCursor(0, 1);
  lcd.print(gateHeight, 1);  // Mostra com 1 casa decimal
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
}