# Sistema de Controle de Comporta para Represa de Peixes

Este projeto implementa um sistema automatizado para controle de comporta em uma represa de peixes, incluindo monitoramento de temperatura, alimentação automática, sistema de emergência e interface de usuário via LCD e LEDs.

## Funcionalidades Principais

- **Controle da altura da comporta** usando motor de passo e joystick
- **Monitoramento da temperatura da água** (sensor DS18B20)
- **Sistema de emergência** (botão + buzzer)
- **Alimentação automática dos peixes** (servo motor)
- **Interface de usuário** em display LCD I2C
- **Indicação visual** com LEDs

---

## Componentes Utilizados

- **Arduino** (qualquer compatível)
- **Motor de passo** (com driver ULN2003)
- **Servo motor** (para alimentação)
- **Sensor de temperatura DS18B20**
- **Sensor ultrassônico** (HC-SR04)
- **Joystick analógico**
- **Display LCD 16x2 I2C**
- **Buzzer**
- **LEDs**
- **Botões**

---

## Estrutura do Código

### 1. Definições e Inicializações
- Definição dos pinos para sensores, atuadores, LEDs, botões, etc.
- Definição de constantes do sistema (intervalos, limites, endereços).
- Criação de objetos para motores, LCD, sensores e servo.

### 2. Função `setup()`
- Inicializa comunicação serial, pinos, sensores, LCD, motores e servo.
- Exibe mensagens iniciais no LCD.
- Faz leituras iniciais dos sensores.
- Atualiza LEDs e display.

### 3. Função `loop()`
Executa continuamente:
- Verifica botão do encoder para trocar de tela no LCD.
- Verifica se é hora de alimentar os peixes.
- Lê temperatura periodicamente e ativa alarme se necessário.
- Verifica botão de emergência.
- Gerencia estados prioritários: emergência, alarme de temperatura, alimentação.
- Se tudo normal, lê sensor ultrassônico, joystick e atualiza display.
- Atualiza LEDs.

---

## Funcionalidades Detalhadas

### Controle da Comporta
- **Joystick**: Move a comporta para cima/baixo via motor de passo, respeitando limites de segurança.
- **Sensor Ultrassônico**: Mede a altura da água/comporta e atualiza variáveis.
- **LEDs**: Indicam visualmente a altura da comporta (mais LEDs acesos = comporta mais fechada).

### Monitoramento de Temperatura
- **Sensor DS18B20**: Mede temperatura da água.
- **Alarme**: Se a temperatura sair dos limites seguros, ativa buzzer e exibe alerta no LCD.

### Emergência
- **Botão**: Ao pressionar, ativa modo emergência, abre a comporta rapidamente, aciona buzzer e exibe mensagem no LCD.

### Alimentação Automática
- **Servo Motor**: Abre/fecha compartimento de ração em intervalos definidos.
- **LCD**: Mostra tempo restante para próxima alimentação.

### Interface LCD
- 3 telas principais:
  1. Altura da comporta (com status: aberta, fechada, parcial)
  2. Temperatura da água (com status: fria, normal, quente)
  3. Tempo para próxima alimentação

### Estados Prioritários
- Emergência, alarme de temperatura e alimentação têm prioridade sobre operação normal.

---

## Principais Funções do Código

- `checkFeedingTime()`: Verifica se é hora de alimentar os peixes.
- `startFeeding()`, `handleFeeding()`: Controlam o processo de alimentação.
- `checkEncoderButton()`: Troca de tela no LCD.
- `updateScreenContent()`: Atualiza o conteúdo do LCD conforme a tela selecionada.
- `readTemperature()`: Lê a temperatura da água.
- `activateTempAlarm()`, `handleTempAlarm()`: Gerenciam o alarme de temperatura.
- `activateEmergency()`, `handleEmergency()`: Gerenciam o modo de emergência.
- `readUltrasonicSensor()`: Mede a altura da comporta.
- `handleJoystick()`: Controla o motor de passo via joystick.
- `updateLEDs()`: Atualiza os LEDs conforme a altura da comporta ou estado de alarme.

---

## Fluxo Simplificado

1. **Inicialização**: Tudo é configurado e o sistema exibe mensagens iniciais.
2. **Operação Normal**: Usuário pode controlar a comporta, ver temperatura e tempo para alimentação.
3. **Alarme/Emergência**: Se detectado problema, sistema entra em modo prioritário (alarme ou emergência).
4. **Alimentação**: O servo libera ração automaticamente em intervalos definidos.

---

## Observações
- O sistema prioriza segurança: emergência e alarmes interrompem a operação normal.
- O LCD facilita a interação e monitoramento pelo usuário.
- O código pode ser expandido para incluir mais sensores ou funcionalidades.

---

## Autor
- Projeto acadêmico para disciplina de Sistemas Embarcados (IPB) 