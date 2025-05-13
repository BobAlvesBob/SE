# Documentação: Jogo da Cobrinha para Arduino UNO R4 V5 WiFi Mini

## Descrição do Projeto

Este projeto implementa o clássico jogo da cobrinha (Snake) utilizando um Arduino UNO R4 V5 WiFi Mini com matriz de LEDs embutida, um joystick analógico para controlar a direção da cobra e um buzzer para efeitos sonoros.

## Mapeamento de Pinos

### Conexões Físicas

| Componente | Pino do Componente | Pino do Arduino |
|------------|--------------------|--------------------|
| Joystick   | VCC                | 5V                 |
| Joystick   | GND                | GND                |
| Joystick   | VRx (eixo X)       | A0                 |
| Joystick   | VRy (eixo Y)       | A1                 |
| Joystick   | SW (botão)         | D2                 |
| Buzzer     | Pino Positivo (+)  | D3                 |
| Buzzer     | Pino Negativo (-)  | GND                |
| Matriz LED | (Embutida)         | (Integrada)        |

### Definição dos Pinos no Código

```cpp
// Definição dos pinos
#define PIN_JOYSTICK_X A0   // Eixo X do joystick
#define PIN_JOYSTICK_Y A1   // Eixo Y do joystick
#define PIN_JOYSTICK_SW 2   // Botão do joystick
#define PIN_BUZZER     3    // Buzzer para efeitos sonoros
```

## Funcionamento do Código

### Estrutura Geral

O código está organizado nas seguintes seções principais:

1. **Inicialização e definições** - Configuração de pinos, objetos e variáveis
2. **Controle do display** - Funções para manipular a matriz de LEDs
3. **Mecânica do jogo** - Movimento da cobra, colisões e geração de comida
4. **Interface de usuário** - Controle via joystick, sons e animações
5. **Funções de loop principal** - Gerenciamento do fluxo do jogo

### Principais Componentes do Código

#### Estruturas de Dados

```cpp
// Estrutura para representar uma posição no grid
struct Posicao {
  byte x;
  byte y;
};

// Direções possíveis para a cobra
enum Direcao {CIMA, DIREITA, BAIXO, ESQUERDA};
```

#### Buffer de Tela

O jogo utiliza um buffer bidimensional para representar o estado do jogo antes de renderizá-lo na matriz de LEDs:

```cpp
// Buffer para o frame da matriz de LEDs
byte frameBuffer[8][12] = {0}; // Matriz 8x12
```

#### Controle da Cobra

A cobra é representada como um array de posições:

```cpp
Posicao cobra[TAMANHO_MAXIMO_COBRA]; // Posições da cobra
```

O movimento da cobra ocorre movendo cada segmento para a posição do segmento anterior, e depois movendo a cabeça na direção atual:

```cpp
// Mover a cobra (primeiro mover o corpo)
for (int i = tamanho_cobra - 1; i > 0; i--) {
  cobra[i].x = cobra[i-1].x;
  cobra[i].y = cobra[i-1].y;
}

// Mover a cabeça na direção atual
switch (direcao) {
  case CIMA:
    cobra[0].y = (cobra[0].y > 0) ? cobra[0].y - 1 : GRID_HEIGHT - 1;
    break;
  // outras direções...
}
```

#### Detecção de Colisões

O jogo detecta dois tipos de colisões:

1. **Colisão com a comida** - Aumenta o tamanho da cobra e gera nova comida
2. **Colisão com o próprio corpo** - Termina o jogo

```cpp
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
```

#### Controle do Joystick

O joystick controla a direção da cobra, com valores invertidos conforme necessário:

```cpp
// Verificar joystick para determinar a direção
int valorX = analogRead(PIN_JOYSTICK_X);
int valorY = analogRead(PIN_JOYSTICK_Y);

// Mapear valores do joystick (invertidos conforme necessário)
if (valorX < 300 && ultimaDirecao != ESQUERDA) {
  direcao = DIREITA;  // Invertido
} else if (valorX > 700 && ultimaDirecao != DIREITA) {
  direcao = ESQUERDA; // Invertido
} // etc...
```

O botão do joystick é usado para pausar/retomar o jogo ou reiniciar após perder:

```cpp
// Verificar o botão do joystick
if (verificarBotaoJoystick()) {
  if (!jogoAtivo) {
    // Reiniciar o jogo
  } else {
    // Alternar pausa
    jogoPausado = !jogoPausado;
  }
}
```

#### Efeitos Sonoros

O buzzer é usado para fornecer feedback sonoro em vários eventos:

```cpp
// Som ao comer comida
tone(PIN_BUZZER, 800, 50);

// Som de game over
void tocarGameOver() {
  tone(PIN_BUZZER, 400, 200);
  delay(250);
  tone(PIN_BUZZER, 300, 200);
  delay(250);
  tone(PIN_BUZZER, 200, 300);
  delay(300);
}

// Som ao pausar/despausar
tone(PIN_BUZZER, jogoPausado ? 500 : 600, 50);
```

## Lógica do Jogo

### Fluxo Básico

1. **Inicialização**: Configura a matriz de LEDs, o joystick e o buzzer, e define a posição inicial da cobra.
2. **Loop Principal**:
   - Verifica input do joystick (direção e botão)
   - Atualiza a posição da cobra
   - Verifica colisões
   - Atualiza o display
3. **Eventos**:
   - Comer comida: aumenta o tamanho da cobra e gera nova comida
   - Colisão com o corpo: termina o jogo
   - Pressionar botão: pausa/retoma o jogo ou reinicia após game over

### Características Especiais

1. **Limite Wraparound**: Quando a cobra atinge a borda do grid, ela reaparece do lado oposto.
2. **Efeito de Piscar**: A comida pisca para ser mais visível.
3. **Modo de Pausa**: O jogo pode ser pausado/retomado pressionando o botão do joystick.
4. **Animações**: Animações de início e game over melhoram a experiência do usuário.

## Instruções de Uso

1. **Controles**:
   - **Joystick**: Move a cobra nas quatro direções (cima, baixo, esquerda, direita)
   - **Botão do Joystick**: Pausa/retoma o jogo ou reinicia após perder
   
2. **Objetivo**:
   - Comer o máximo de comida possível sem colidir com o próprio corpo
   - Cada comida aumenta o tamanho da cobra em 1 unidade
   
3. **Game Over**:
   - Ocorre quando a cobra colide com seu próprio corpo
   - Uma animação e som específicos são mostrados
   - Pressione o botão do joystick para reiniciar

## Troubleshooting

- **Direções Invertidas**: Se as direções do joystick parecerem invertidas, ajuste o mapeamento no código
- **Resposta Lenta**: Reduza o valor de TEMPO_ATUALIZACAO para tornar o jogo mais rápido
- **Colisões Incorretas**: Verifique as funções de detecção de colisões e as dimensões do grid

## Extensões Possíveis

1. Sistema de pontuação exibido no Serial Monitor
2. Diferentes níveis de dificuldade (velocidade)
3. Obstáculos adicionais no grid
4. Efeitos visuais mais elaborados
5. Integração com outros componentes (como display LCD externo para mostrar pontuação)