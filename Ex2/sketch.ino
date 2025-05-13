// Jogo da Cobrinha para Arduino UNO usando MAX7219
// Utiliza joystick, matriz de LEDs e buzzer

// Bibliotecas necessárias
#include <LedControl.h>

// Definição dos pinos
#define PIN_JOYSTICK_X A2
#define PIN_JOYSTICK_Y A0
#define PIN_BUZZER     3

// Configuração do MAX7219
#define PIN_DIN       13  // Pino DIN do MAX7219
#define PIN_CS        10  // Pino CS (ou LOAD) do MAX7219
#define PIN_CLK       11  // Pino CLK do MAX7219
#define NUM_DEVICES   1   // Agora apenas uma matriz MAX7219

// Inicializar objeto LedControl
LedControl lc = LedControl(PIN_DIN, PIN_CLK, PIN_CS, NUM_DEVICES);

// Variáveis do jogo
#define TAMANHO_MAXIMO_COBRA 64
#define TEMPO_ATUALIZACAO 200 // ms
#define GRID_WIDTH  8   // 8 colunas
#define GRID_HEIGHT 8   // 8 linhas

// Estrutura para representar uma posição no grid
struct Posicao {
  byte x;
  byte y;
};

enum Direcao {CIMA, DIREITA, BAIXO, ESQUERDA};

// Variáveis globais
Posicao cobra[TAMANHO_MAXIMO_COBRA]; // Posições da cobra
Posicao comida;                      // Posição da comida
byte tamanho_cobra = 1;              // Tamanho inicial da cobra
Direcao direcao = DIREITA;           // Direção inicial
Direcao ultimaDirecao = DIREITA;     // Última direção processada
unsigned long tempoAnterior = 0;     // Para controle do tempo
boolean jogoAtivo = true;            // Estado do jogo

// Função para atualizar o display de LEDs
void atualizarDisplay() {
  lc.clearDisplay(0);
  // Desenhar a cobra
  for (int i = 0; i < tamanho_cobra; i++) {
    int localX = cobra[i].x;
    int localY = cobra[i].y;
    lc.setLed(0, localY, localX, true);
  }
  // Desenhar a comida (piscando)
  if ((millis() / 200) % 2 == 0) {
    int localX = comida.x;
    int localY = comida.y;
    lc.setLed(0, localY, localX, true);
  }
}

// Gerar nova posição para a comida
void gerarComida() {
  boolean posicaoValida = false;
  while (!posicaoValida) {
    comida.x = random(GRID_WIDTH);
    comida.y = random(GRID_HEIGHT);
    posicaoValida = true;
    for (int i = 0; i < tamanho_cobra; i++) {
      if (cobra[i].x == comida.x && cobra[i].y == comida.y) {
        posicaoValida = false;
        break;
      }
    }
  }
}

// Verificar colisão com a comida
boolean verificarComida() {
  return (cobra[0].x == comida.x && cobra[0].y == comida.y);
}

// Verificar colisão com a própria cobra
boolean verificarColisaoCorpo() {
  for (int i = 1; i < tamanho_cobra; i++) {
    if (cobra[0].x == cobra[i].x && cobra[0].y == cobra[i].y) {
      return true;
    }
  }
  return false;
}

// Verificar colisão com as paredes
boolean verificarColisaoParedes() {
  return (cobra[0].x < 0 || cobra[0].x >= GRID_WIDTH || 
          cobra[0].y < 0 || cobra[0].y >= GRID_HEIGHT);
}

// Tocar som de game over
void tocarGameOver() {
  // Melodia simples de game over
  tone(PIN_BUZZER, 400, 200);
  delay(250);
  tone(PIN_BUZZER, 300, 200);
  delay(250);
  tone(PIN_BUZZER, 200, 300);
  delay(300);
}

// Animar LEDs para o game over
void animacaoGameOver() {
  for (int i = 0; i < 3; i++) {
    // Acender todos os LEDs em todas as matrizes
    for (int dev = 0; dev < NUM_DEVICES; dev++) {
      for (int row = 0; row < 8; row++) {
        lc.setRow(dev, row, 0xFF);
      }
    }
    delay(200);
    
    // Apagar todos os LEDs
    for (int dev = 0; dev < NUM_DEVICES; dev++) {
      lc.clearDisplay(dev);
    }
    delay(200);
  }
}

// Tocar som quando come a comida
void tocarComida() {
  tone(PIN_BUZZER, 800, 100);
  delay(100);
  noTone(PIN_BUZZER);
}

void setup() {
  // Inicializar comunicação serial
  Serial.begin(9600);
  
  // Inicializar pinos
  pinMode(PIN_JOYSTICK_X, INPUT);
  pinMode(PIN_JOYSTICK_Y, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  
  // Inicializar todas as matrizes MAX7219
  for (int i = 0; i < NUM_DEVICES; i++) {
    lc.shutdown(i, false);  // Acordar do modo economia de energia
    lc.setIntensity(i, 8);  // Definir brilho médio (0-15)
    lc.clearDisplay(i);     // Limpar o display
  }
  
  // Inicializar posição da cobra
  cobra[0].x = 1;
  cobra[0].y = 0;
  
  // Gerar posição inicial da comida
  randomSeed(analogRead(A3)); // Usar pino flutuante para melhor aleatoriedade
  gerarComida();
  
  // Animar LEDs no início
  for (int dev = 0; dev < NUM_DEVICES; dev++) {
    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 8; j++) {
        lc.setLed(dev, i, j, true);
        delay(10);
      }
    }
  }
  
  // Apagar todos os LEDs
  for (int dev = 0; dev < NUM_DEVICES; dev++) {
    lc.clearDisplay(dev);
  }
  delay(300);
}

void loop() {
  if (!jogoAtivo) {
    // Se o jogo acabou, reiniciar após um tempo
    delay(2000);
    jogoAtivo = true;
    tamanho_cobra = 1;
    cobra[0].x = 1;
    cobra[0].y = 0;
    direcao = DIREITA;
    ultimaDirecao = DIREITA;
    gerarComida();
    return;
  }
  
  // Verificar joystick para determinar a direção
  int valorX = analogRead(PIN_JOYSTICK_X);
  int valorY = analogRead(PIN_JOYSTICK_Y);
  
  // Mapear valores do joystick (normalmente 0-1023) para direções
  if (valorX < 300 && ultimaDirecao != DIREITA) {
    direcao = ESQUERDA;
  } else if (valorX > 700 && ultimaDirecao != ESQUERDA) {
    direcao = DIREITA;
  } else if (valorY < 300 && ultimaDirecao != BAIXO) {
    direcao = CIMA;
  } else if (valorY > 700 && ultimaDirecao != CIMA) {
    direcao = BAIXO;
  }
  
  // Atualizar o jogo a cada TEMPO_ATUALIZACAO ms
  unsigned long tempoAtual = millis();
  if (tempoAtual - tempoAnterior >= TEMPO_ATUALIZACAO) {
    tempoAnterior = tempoAtual;
    ultimaDirecao = direcao;
    
    // Mover a cobra (primeiro mover o corpo)
    for (int i = tamanho_cobra - 1; i > 0; i--) {
      cobra[i].x = cobra[i-1].x;
      cobra[i].y = cobra[i-1].y;
    }
    
    // Mover a cabeça na direção atual
    switch (direcao) {
      case CIMA:
        cobra[0].y--;
        break;
      case DIREITA:
        cobra[0].x++;
        break;
      case BAIXO:
        cobra[0].y++;
        break;
      case ESQUERDA:
        cobra[0].x--;
        break;
    }
    
    // Verificar colisões
    if (verificarColisaoParedes() || verificarColisaoCorpo()) {
      jogoAtivo = false;
      tocarGameOver();
      animacaoGameOver();
      return;
    }
    
    // Verificar se comeu a comida
    if (verificarComida()) {
      tocarComida();
      tamanho_cobra++;
      gerarComida();
    }
    
    // Atualizar o display
    atualizarDisplay();
  }
}