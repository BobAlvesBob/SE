#include "avr/io.h"         ; Definições dos registradores e bits do AVR

.global configure
.global cycle

;----------------------------------------------------------------
; Função: configure
; Descrição: Configura PB0 como saída e o inicializa em nível 0.
;----------------------------------------------------------------
configure:
        ; 1) Configura PB0 como saída
        ldi    r16, (1 << PB0)  ; Define o bit PB0 como 1
        out    DDRB, r16        ; Escreve em DDRB, tornando PB0 uma saída

        ; 2) Coloca PB0 em nível 0 (aciona o motor se ativo em 0)
        ldi    r16, 0x00        ; Valor 0
        out    PORTB, r16       ; PB0 = 0

        ret

;----------------------------------------------------------------
; Função: cycle
; Descrição: Mantém o estado do pino (motor ligado). Nada mais.
;----------------------------------------------------------------
cycle:
        ; Aqui poderíamos ler sensores, etc., mas para algo simples, não há instruções
        ret
