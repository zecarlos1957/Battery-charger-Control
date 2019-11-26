

;***** COMPILATION MESSAGES & WARNINGS *****

	ERRORLEVEL -207 	; Found label after column 1.
	ERRORLEVEL -302 	; Register in operand not in bank 0.



	#include "p16f873a.inc"




   global SetupConfig

   CODE

;****************************************************
 
SetupConfig
    ;  config I/O Ports 
    banksel TRISA
    movlw b'00001111'  
          ; -------1'  AN0 input  U1 analog signal 
          ; ------1-   AN1 input  U2 analog signal
          ; -----1--   AN2 input  I1 analog signal
          ; ----1---   AN3 input  temperature   analog signal
          ; --00----   RA4/RA5 output RW/RS display
    movwf TRISA
    

    movlw b'11100000'
          ; 1-------   RB7 input button Up
          ; -1------   RB6 input button Down
          ; --1-----   RB5 input button Ok
          ; ---0----   RB4 display Enable
          ; ----0000   RB0-RB3 display data
    movwf TRISB
    

    movlw b'11010010' 
          ; xxxxxxx0   RC0   output alarm sirene                  *
          ; xxxxxx1x   RC1   DTR   
          ; xxxxx0xx   RC2   PWM 
          ; xxxx0xxx   RC3   DSR
          ; xxx1xxxx   RC4   RTS
          ; xx0xxxxx   RC5   CTS 
          ; xxxxxxxx   RC6   TX serial
          ; xxxxxxxx   RC7   RX serial
    movwf TRISC
   
  
 
  ; TRM0 config 
   movlw b'00000111'       ; configure Timer0
         ; 0-------       PORTB pull-up resistor enable 
         ; --0-----       TMR0 source clock internal(TOCS = 0)
         ; ----0---       prescaler assigned to Timer0 (PSA = 0)
         ; -----111       prescale = 1:256 (PS = 111)
   movwf OPTION_REG
  
; Setup AD Converter
; ADCON0  b'11xxx001'

   movlw b'10000010' 
         ; 1-------      right just = 10 bits result   (ADFM =1)
         ; -0------      AD clock from internal RC Oscillator (ADCS2 = 0)
         ; ----0010    	   AN0,1,2,3,4  Vdd Vref+, Vss Vref-
   movwf ADCON1
  

; set highest PWM value 
 
   movlw 0xff   ;
   movwf PR2    ; Set PWM period
  
;  Setup PWM module
  banksel CCP1CON
   movf CCP1CON,w
   andlw 0xf0
   iorlw 0x0c    
   movwf CCP1CON
      ; --xx----  2 LSB bits duty cicle
      ; ----11xx  enable PWM mode
 
    clrf PORTA
    clrf PORTB 
    clrf PORTC

 

 ;setup TMR2 
   movlw b'00000000' 
       ;   -0000--- TMR2 output postscaler 1:1
       ;   -----x-- TMR2 On/Off 
       ;   ------01 clock prescaler 1:1
   movwf T2CON
    clrf CCPR1L
   bsf T2CON,TMR2ON 

   
    banksel SPBRG
    movlw 0x19     ; 9600 bdps    (BRH=1)
    movwf SPBRG
    movlw b'00100100'
      ;     -0------     8 bits transmit
      ;     --1-----     transmite enable
      ;     ---0----     Asynchronous mode
      ;     -----1--     hi speed (BRGH)
      ;     ------x-     TRMT
      ;     -------0     Data9BitTransmit  
    movwf TXSTA

    bsf PIE1,RCIE      ; Enable USART interrupt
    banksel RCSTA
    movlw b'00010000'
      ;     0-------  Serial port dissable
      ;     -0------  8 bits reception
      ;     ---1----  enable continuous receive
      ;     ----0---  disable addr detection. 9 can be parity bit
      ;     -----0--  framing error 
      ;     ------0-  no overrun error
      ;     -------0  Data9BitReceive
    movwf RCSTA

    bcf PIR1,RCIF    ; Clear interrupt flag
    bsf INTCON,PEIE  ; Enable Peripheral interrupt

  
;   movlw BAUD_9600
;   pagesel UART_Init
;   call UART_Init


 

   return




;*************************************************************************
 

    END
