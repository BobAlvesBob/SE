#include "avr/io.h"         ; Inclusão das definições dos registradores e bits do AVR

.global configure
.global cycle

;----------------------------------------------------------------
; Função: configure
; Descrição: Configura as portas I/O:
;   - PB0, PB1 e PB5 como saída;
;   - PB4 (sensor esquerdo) como entrada com pull-up;
;   - PD7 (sensor direito) como entrada com pull-up.
;----------------------------------------------------------------
configure:
        ; Configurar DDRB: PB0, PB1 e PB5 como saída (PB4 permanece entrada)
        ldi    r16, (1<<PB0)|(1<<PB1)|(1<<PB5)
        out    DDRB, r16

        ; Configurar DDRD: todos os pinos como entrada (usamos somente PD7)
        ldi    r16, 0x00
        out    DDRD, r16

        ; Inicializar PORTB:
        ; - Saídas (PB0, PB1, PB5) em nível alto (desativado, pois são ativos em 0)
        ; - Ativar pull-up em PB4 (entrada)
        ldi    r16, (1<<PB0)|(1<<PB1)|(1<<PB4)|(1<<PB5)
        out    PORTB, r16

        ; Ativar pull-up em PD7 (entrada)
        ldi    r16, (1<<PD7)
        out    PORTD, r16

        ret

;----------------------------------------------------------------
; Função: cycle
; Descrição: Lê os sensores e comuta a direção do motor:
;   - Se o sensor esquerdo (PB4) for acionado (nível 0),
;     inverte para virar à direita (ativa PB1 em 0).
;   - Se o sensor direito (PD7) for acionado (nível 0),
;     inverte para virar à esquerda (ativa PB0 em 0).
;----------------------------------------------------------------
cycle:
        ; Lê sensor esquerdo (PB4)
        in     r16, PINB
        sbrs   r16, PB4      ; Se PB4 estiver alto (não acionado), pula
        rjmp   sensor_left_pressed

        ; Lê sensor direito (PD7)
        in     r16, PIND
        sbrs   r16, PD7      ; Se PD7 estiver alto, pula
        rjmp   sensor_right_pressed

        ret                 ; Se nenhum sensor acionado, retorna

;----------------------------------------------------------------
; Bloco: sensor_left_pressed
; Ação: Se PB4 estiver acionado (nível 0) → virar à direita.
; Configuração: PB1 em nível 0 (ativo) e PB0 em nível alto.
;----------------------------------------------------------------
sensor_left_pressed:
        ldi    r16, (1<<PB0)|(1<<PB4)|(1<<PB5)  ; PB0=1, PB1=0 (não incluído)
        out    PORTB, r16
        rjmp   end_cycle

;----------------------------------------------------------------
; Bloco: sensor_right_pressed
; Ação: Se PD7 estiver acionado (nível 0) → virar à esquerda.
; Configuração: PB0 em nível 0 (ativo) e PB1 em nível alto.
;----------------------------------------------------------------
sensor_right_pressed:
        ldi    r16, (1<<PB1)|(1<<PB4)|(1<<PB5)  ; PB1=1, PB0=0 (não incluído)
        out    PORTB, r16

end_cycle:
        ret
