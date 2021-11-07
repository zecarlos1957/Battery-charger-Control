;**********************************************************************
;  Filename: Battery Charge Control                                   *
;  Uses PIC16F876A                                                    *
;  Date: 1.5 2016                                                     *
;  Author:  Jose Jesus                                                *
;                                                                     *
;  Uses:
;        A/D  voltage monitor
;        PWM to control battery charger                                *
;        USART data manager
;        LCD2X16 display interface
;                                                                  *
;  4 MHz Osc                                                         *
;                                                                     *
;                                                                     *
;                                                                     *
;                                                                     *                           *
;
; Tosc = (1/4  MHz)   = 0.00000025s = 250ns                            *
; 1 instruction  = TOsc * 4 = 1us                                   *
;
; PWM period = 20KHz
;     = [PR2+1]*4*Tosc*(TMR2 prescaler)
; Duty cicle = (CCPR1L<5:4>)*Tosc*(TMR2 prescaler)
; Average voltage = U0 * Duty cicle   ????
; RMs= Vp* RootSquare(T1/T)
;
;                                                                    *
;                                                                     *
;**********************************************************************
;                                                                     *
;**********************************************************************
;                                                                     *                                   *
;    Pin assignments:                                                 *                                   *
;     2 - RA0-1-AN0         = Painel voltage                                *                                 *
;     3 - RA1-1-AN1         = U_out
;     4 - RA2-1-AN2         = I_out                                         *                                 *
;     5 - RA3-1-AN3         = NTC temperature                           *
;     6 - RA4-0             = RS display                                *
;     7 - RA5-0-AN4                                           *
;    21 - RB0-0             = D4 display                                *
;    22 - RB1-0             = D5 display                                *
;    23 - RB2-0             = D6 display                                *
;    24 - RB3-0             = D7 display         |   PGM                         *
;    25 - RB4-0             = E  display                                *                                 *
;    26 - RB5-1             = button Ok                                 *
;    27 - RB6-1             = button Down        |   PGC                       *
;    28 - RB7-1             = button Up          |   PGD                               *
;    11 - RC0-0             = alarm sirene  !!
;    12 - RC1-0             = DTR                                  *
;    13 - RC2-0-PWM1        = PWM output
;    14 - RC3-0             = DSR
;    15 - RC4-1             = RTS
;    16 - RC5-O             = CTS                                      *
;    17 - RC6-0-TX          = USART Tx                                  *
;    18 - RC7-1-RX          = USART Rx                                  *
;
;                                                                    *
;                                                                    *
;**********************************************************************
;                                                                     *
;         *******  Baterias de chumbo/ácido ******                    *
;                                                                     *
;   Descarregada !         <   1.75          V_off    -> 10,5v        *
;   Flutua��o (normal)         2.15/2.20     V_float  -> 12.9v/13.2v  *
;   Equaliza��o (recarga)      2.36/2.40     V_oct    -> 14.16/14.4v  *  
;   Sobretens�o (sobrecarga  > 2.70                   -> 16.2v        *
;                                                                     *
;   Equalização (recarga)    10% da capacidade   I_blk                *
;   Flutuaçao                 1%  "       "      I_tric               *
;   Retenção                 I_blk/5             I_oct                *
;                                                                     *
;                                                                     *
;                                                                     *
;           *** Compensação da variação da temperatura ***            *
;                                                                     *
;     Ref 25ºC:                                                       *
;                -0.33V por cada +10ºC                                *
;                +0.33V por cada -10ºC                                *
;                                                                     *
;**********************************************************************
;     Conversão A/D
;                         U = 6.7353 * (5*AD/1023)
;                         I = (5 / 1024) * AD / (R=0.33)
;
;        Resitência  na saída 0,31 hom para calculo de I
;                      ( 1 / (1/1 + 1/1 + 1/1) = 0.33 )
;
;***************************************************************************
;   ERRORS:
;           1 -  Battery not found or lower from 11 volts
;
;
;***** COMPILATION MESSAGES & WARNINGS *****

	ERRORLEVEL -207 	; Found label after column 1.
	ERRORLEVEL -302 	; Register in operand not in bank 0.

;***** PROCESSOR DECLARATION & CONFIGURATION *****






   ;  EEPROM addr

DEV_ID  equ 0x00
W_CAP   equ 0x06
U_MODE  equ 0x07
U_ADJ   equ 0x08
I_ADJ   equ 0x09
R_CNT   equ 0x0a



	PROCESSOR 16F876a
	#include "p16f876a.inc"

	; embed Configuration Data within .asm File.
	__CONFIG   _CPD_OFF & _CP_OFF & _WDT_OFF & _PWRTE_ON & _XT_OSC & _LVP_OFF & _DEBUG_OFF



   #define  SLOW_CHARGE 1
   #define  FAST_CHARGE 2
   #define  OVER_CHARGE 3
   #define  FLOAT_CHARGE 4





 ; ********* Math library



    extern Update_PWM
    extern FXM1616U
    extern SetupConfig

 
 

   #define AEXP   0xa8
   #define AARG   0xa7
   #define AARGB0 0xa7
   #define AARGB1 0xa6
   #define AARGB2 0xa5
   #define AARGB3 0xa4
   #define AARGB4 0xa3
   #define AARGB5 0xa2

   #define BEXP   0xad
   #define BARG   0xac
   #define BARGB0 0xac
   #define BARGB1 0xab
   #define BARGB2 0xaa
   #define BARGB3 0xa9


   #define TEMPB1 0xb2
   #define TEMPB0 0xb3

;    #define Vinteger 0xc6
;    #define ExpFract 0xc0
;   #define BINdata 0xca



;*********************************





     #define bin   0xc0   ; separate exp:fraction ex: 5.735 = 5 735
   #define bcd   0xc4
   #define pti    0xce
   #define pto    0xcf
   #define ii     0xd0
   #define temp   0xd1
   #define cnt    0xd2


 ;*******************************



  ;*************************


    #define LOW_INLET_VOLTAGE err_flag,0
    #define MSG_BLINK err_flag, 1
    #define CHARGE_ON err_flag, 2
    #define NO_BATTERY err_flag,3
    #define RS_ERROR   err_flag,3

    #define DTR_PIN PORTC,1
    #define DSR_PIN PORTC,3
    #define RTS_PIN PORTC,4
    #define CTS_PIN PORTC,5




;***** MACROS *****


;***** PORT DECLARATION *****


;***** CONSTANT DECLARATION *****

	CONSTANT BASE = 0x20		; base address of user file registers

;***** CONSTANT DECLARATION *****

	IFNDEF	LCDLINENUM	; use default value, if unspecified
		CONSTANT LCDLINENUM = 0x02	; by default, 2 lines
	ENDIF
	IFNDEF	LCDTYPE		; use default value, if unspecified
		CONSTANT LCDTYPE = 0x00		; standard HD44780 LCD
		;CONSTANT LCDTYPE = 0x01	; EADIP204-4 (w/ KS0073)
	ENDIF
	IFNDEF	LCDSPEED	; use default value, if unspecified
		;CONSTANT LCDSPEED = 0x00	; clk in [0..9] MHz
		CONSTANT LCDSPEED = 0x01	; clk in [9..20] MHz, default
	ENDIF
	IFNDEF	LCDWAIT		; use default value, if unspecified
		CONSTANT LCDWAIT = 0x01		; for Tosc <= 5 MHz
	ENDIF
	IFNDEF	LCDCLRWAIT	; use default value, if unspecified
		CONSTANT LCDCLRWAIT = 0x08	; wait after LCDCLR until LCD is ready again
	ENDIF

;***** REGISTER DECLARATION *****

	;WCYCLE	->	BASE+d'0'	; wait cycle counter

	LCDbuf	set	BASE+d'1'	; LCD data buffer
	LCDtemp	set	BASE+d'2'	; LCD temporary register

           ; m_lcdv08
	LO	equ	BASE+d'3'
	HI	equ	BASE+d'4'
	LO_TEMP	set	BASE+d'5'
	HI_TEMP	set	BASE+d'6'


	TEMP1	equ	BASE+d'7'	; Universal Temporary Register
	TEMP3	equ	BASE+d'8'
	TEMP2	equ	BASE+d'9'
	TEMP4	equ	BASE+d'10'
	TEMP5	equ	BASE+d'11'

	LCDFLAGreg	equ	BASE+d'12'

    STR_NUM equ BASE+d'13'
    INDEX   equ BASE+d'14'

	#define	Digit0  LCDFLAGreg,0x00	; LCD busy flag declared within flag register
	#define	Digit1  LCDFLAGreg,0x01
    #define Digit2   LCDFLAGreg,0x02
    #define	LJustify LCDFLAGreg,0x04 ; swap zeros by space at right if this bit is 0
	#define	BCflag   LCDFLAGreg,0x05


	;*** LCD module versions for fixed ports (e.g. PortB) ***
	LCDtris	equ	TRISB
	LCDport	equ	PORTB

;    #define	LCD_ENtris TRISB,0x04	; EN on portB,1
;    #define	LCD_RStris TRISA,0x04	; RS on portB,2
;    #define	LCD_RWtris TRISA,0x05	; RW on portB,3

    #define	LCD_EN     PORTB,0x04	; Enable Output / "CLK"
    #define	LCD_RS     PORTA,0x04	; Register Select
    ;#define	LCD_RW  Hard connection to ground  	; Read/Write


;***** LCD COMMANDS *****

  ;*** Standard LCD COMMANDS for INIT ***	( HI-NIBBLE only )
	; for 4 bit mode: send only one nibble as high-nibble [DB7:DB4]
	CONSTANT  LCDEM8  = b'0011'	; entry mode set: 8 bit mode, 2 lines
	CONSTANT  LCDEM4  = b'0010'	; entry mode set: 4 bit mode, 2 lines
	CONSTANT  LCDDZ   = b'1000'	; set Display Data Ram Address to zero

  ;*** Standard LCD COMMANDS ***		( HI- / LO-NIBBLE )
	; USE THESE COMMANDS BELOW AS FOLLOW: "LCDcmd LCDCLR"
	CONSTANT  LCDCLR  = b'00000001'	; clear display: resets address counter & cursor
	CONSTANT  LCDCH   = b'00000010'	; cursor home
	CONSTANT  LCDCR   = b'00000110'	; entry mode set: cursor moves right, display auto-shift off
	CONSTANT  LCDCL   = b'00000100'	; entry mode set: cursor moves left, display auto-shift off
	CONSTANT  LCDCONT = b'00001100'	; display control: display on, cursor off, blinking off
	CONSTANT  LCDMCL  = b'00010000'	; cursor/disp control: move cursor left
	CONSTANT  LCDMCR  = b'00010100'	; cursor/disp control: move cursor right
	CONSTANT  LCDSL   = b'00011000'	; cursor/disp control: shift display content left
	CONSTANT  LCDSR   = b'00011100'	; cursor/disp control: shift display content right
	CONSTANT  LCD2L   = b'00101000'	; function set: 4 bit mode, 2 lines, 5x7 dots
	IF (LCDLINENUM == 0x2)
	  CONSTANT  LCDL1 = b'10000000'	; DDRAM address: 0x00, selects line 1 (2xXX LCD)
	  CONSTANT  LCDL2 = b'11000000'	; DDRAM address: 0x40, selects line 2 (2xXX LCD)
	  CONSTANT  LCDL3 = b'10010100'	; (DDRAM address: 0x14, fallback)
	  CONSTANT  LCDL4 = b'11010100'	; (DDRAM address: 0x54, fallback)
	ELSE
	  CONSTANT  LCDL1 = b'10000000'	; DDRAM address: 0x00, selects line 1 (4xXX LCD)
	  CONSTANT  LCDL2 = b'10010100'	; DDRAM address: 0x14, selects line 2 (4xXX LCD)
	  CONSTANT  LCDL3 = b'11000000'	; DDRAM address: 0x40, selects line 3 (4xXX LCD)
	  CONSTANT  LCDL4 = b'11010100'	; DDRAM address: 0x54, selects line 4 (4xXX LCD)
	ENDIF
	; special configuration for EA DIP204-4
	CONSTANT  LCDEXT  = b'00001001'	; extended function set EA DIP204-4
	CONSTANT  LCD2L_A = b'00101100'	; enter ext. function set: 4 bit mode, 2 lines, 5x7 dots
	CONSTANT  LCD2L_B = b'00101000'	; exit ext. function set: 4 bit mode, 2 lines, 5x7 dots

;***** MACROS *****



LCDw	macro			; write content of w to LCD
	call	LCDdata
	endm

LCDcmd	macro	LCDcommand	; write command to LCD
	movlw	LCDcommand
	call	LCDcomd
	endm



LCD_DDAdr macro	DDRamAddress
	Local	value = DDRamAddress | b'10000000'	; mask command
	IF (DDRamAddress > 0x67)
		ERROR "Wrong DD-RAM-Address specified in LCD_DDAdr"
	ELSE
		movlw	value
		call	LCDcomd
	ENDIF
	endm

LCD_CGAdr macro	CGRamAddress
	Local	value = CGRamAddress | b'01000000'	; mask command
	IF (CGRamAddress > b'00111111')
		ERROR "Wrong CG-RAM-Address specified in LCD_CGAdr"
	ELSE
		movlw	value
		call	LCDcomd
	ENDIF
	endm

clrLCDport macro		; clear/reset LCD data lines
	movlw	b'11110000'	; get mask
	andwf	LCDport,f	; clear data lines only
	endm



WAIT	macro	timeconst_1
	IF (timeconst_1 != 0)
	    movlw	timeconst_1
	    call delay_ms
	ENDIF
	endm



;***** MEMORY STRUCTURE *****





	ORG     0x00			; processor reset vector
    clrf PCLATH
     goto MAIN

 	ORG     0x04			; interrupt vector location
     movwf w_save
     swapf STATUS,w
     movwf status_save
     andlw 0x06
     btfsc STATUS,Z         ; test if we ara in bank0
     goto jmp1
     movfw status_save      ; no, we are not
     bcf STATUS,RP0         ; save context to bank0
     movwf status_save
     bsf STATUS,RP0
     movfw w_save
     bcf STATUS,RP0
     movwf w_save
jmp1
     movf FSR,w
     movwf fsr_save
     movf PCLATH,w
     movwf pclath_save

     pagesel Timer0_isr
     btfsc INTCON,TMR0IF
     call Timer0_isr

     pagesel UsartRX
     banksel PIR1
     btfsc PIR1,RCIF
     bsf STATUS,RP0
     btfsc PIE1,RCIE
     call UsartRX

     banksel w_save
     movf pclath_save,w
     movwf PCLATH
     movf fsr_save,w
     movwf FSR
     swapf status_save,w
     movwf STATUS
     movf w_save,w
     retfie





;************** MAIN **************


MAIN
    pagesel SetupConfig
    call SetupConfig
    pagesel MAIN
    clrf t_cnt
    clrf err_flag
    clrf RS_len
    clrf RS_sz
    clrf RS_chkSum
    clrf Require
    clrf Require+1
    clrf V_cal
    clrf V_cal+1
    clrf I_cal
    clrf I_cal+1
    clrf Err_Symbol

    movlw 0x0e
    movwf Refresh_rate
    clrf charge_status
    movlw 0x07      ; enable 3 digits Right justify on LCDVal08
    movwf LCDFLAGreg
	call LCDinit
			; LCD Initialization

  movlw 0x80
   call LCDcomd
   movlw 9
   call OutText
   movlw 0xc0
   call LCDcomd
   movlw .10
   call OutText

; Enter Config mode

   movlw 0x0f
   movwf t1
L2

    movlw .200
    call delay_ms

    btfsc PORTB,7   ; UP
    goto L3 ; if RB7 not pressed goto L3
    btfsc PORTB,6   ; Down
    goto L3 ; if RB6 not pressed goto L3
    movlw .100
    call delay_ms
    ;
    movlw 0x80
    call LCDcomd  ; set Line 1
    movlw 0     ; print "Config Tensão"
    call OutText

   btfss PORTB,6 ; while RB6 not pressed
    goto $-1
    btfss PORTB,7 ; and while RB7 not pressed
    goto $-1
    movlw .100
    call delay_ms


   movlw 'U'
   movwf typ
   movlw 'V'
   movwf uni
   movlw 0   ; default 12V
   call set_voltage
   movlw 'P'
   movwf typ
   movlw 'A'
   movwf uni
   movlw 0   ; default index 1 = 40Ah
   call set_capacity
   goto L4
L3
  decfsz t1,F
   goto L2

L4


   movlw 1  ; LCDClear
   call LCDcomd


    call initdata

   movlw 0x80
   call LCDcomd
   movlw 2         ;  A INICIAR

   call OutText
   movlw 0xc0
   call LCDcomd
   movlw 'U'
   call LCDdata
   movlw '='
   call LCDdata
   movf V_batt,W
   call LCDval08
   movlw 'V'
   call LCDdata
   movlw '/'
   call LCDdata
   movlw 'P'
   call LCDdata
   movlw '='
   call LCDdata
   movf W_batt,W
   call LCDval08
   movlw 'A'
   call LCDdata
   movlw 'h'
   call LCDdata


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


   movlw 0x80
   call Beep


;********************************************



    clrf TMR0
    bsf INTCON, TMR0IE  ; enable TMR0 overflow interrupt bits    clrf TMR0


    bsf INTCON, GIE   ; Global INT enable bit



;    bsf CHARGE_ON




;*********************************************



Loop
  ;   clrwdt





    movlw U_in
    movwf FSR
    movlw b'11000000'   ; AN0 U_in
    call refresh

;*******************

    movlw U_out
    movwf FSR
    movlw b'11001000'   ; AN1 U_out
    call refresh


;***************

    movlw I_out
    movwf FSR
    movlw b'11010000'   ; AN2 I_out
    call refresh

;***********************************

   ; if(U_out < 8.4V) set NO_BATTERY
   movfw U_out+1
   bz err
   goto err_ok
err:
   bsf NO_BATTERY
   movlw 0xf8
   movwf Err_Symbol
   clrf CCPR1L
   movlw 0x0f
   andwf CCP1CON,F
   goto Control_done


;*************************************
err_ok

   btfsc CHARGE_ON
   goto do_it
   bsf CHARGE_ON
   pagesel Update_PWM
   call Update_PWM
   pagesel MAIN
   goto do_it
teste_ok


   movlw SLOW_CHARGE
   subwf charge_status,W
   btfsc STATUS,Z
   goto do_slow

   movlw FAST_CHARGE
   subwf charge_status,W
   btfsc STATUS,Z
   goto do_fast

   movlw OVER_CHARGE
   subwf charge_status,W
   btfsc STATUS,Z
   goto do_over

   movlw FLOAT_CHARGE
   subwf charge_status,W
   btfsc STATUS,Z
   goto do_float

;************************

chk_slow:
  ; if(U_out < V_off) goto do_slow
   movfw U_out+1
   subwf V_off+1,W
   bnz cmp_Voff
   movfw U_out
   subwf V_off,W
cmp_Voff
   btfss STATUS,C
   goto chk_fast

   movlw SLOW_CHARGE
   movwf charge_status
do_slow:
   ; I Require = I_tric - AD_Iout
   movf I_tric,W
   movwf Require
   movf I_tric+1,W
   movwf Require+1
   movf I_out ,W
   subwf Require,F
   btfss STATUS,C
   decf Require+1,F
   movf I_out+1,W
   subwf Require+1,F
   ; if (  U_out > V_off ) set FAST
   movfw V_off+1
   subwf U_out+1,W
   bnz cmp_Voff2
   movfw V_off
   subwf U_out,W
cmp_Voff2
   btfss STATUS,C
   goto do_it
   btfsc STATUS,Z
   goto do_it
   movlw FAST_CHARGE
   movwf charge_status
   goto do_it

;********************************

chk_fast:
  ; if(Uout < V_float) goto do_fast

   movfw U_out+1
   subwf V_float+1,W
   bnz cmp_Vfloat
   movfw U_out
   subwf V_float,W
cmp_Vfloat
   btfss STATUS,C
   goto chk_over   ;do_float

   movlw FAST_CHARGE
   movwf charge_status
do_fast:
   ; I Require = I_blk - AD_Iout
   movf I_blk,W
   movwf Require
   movf I_blk+1,W
   movwf Require+1
   movf I_out ,W
   subwf Require,F
   btfss STATUS,C
   decf Require+1,F
   movf I_out+1,W
   subwf Require+1,F
   ; if (U_out >= V_oct-1) OVER_CHARGE
   movfw V_oct+1
   subwf U_out+1,W
   bnz cmp_Voct2
   movlw 1
   subwf V_oct,W
   subwf U_out,W
cmp_Voct2
   btfss STATUS,C
   goto do_it
;   movlw OVER_CHARGE
;   movwf charge_status
;   goto do_it

;**********************************
chk_over:

   movlw OVER_CHARGE
   movwf charge_status
do_over
   ; if (U_out >= V_oct-1) Req--
   movfw V_oct+1
   subwf U_out+1,W
   bnz cmp_over
   movlw 1
   subwf V_oct,W
   subwf U_out,W
cmp_over
   btfss STATUS,C
   goto Control_done

   movlw 0xff
   movwf Require
   movwf Require+1

   ; if I_oct >= I_out set FLOAT
   movfw I_out+1
   subwf I_oct+1,W
   bnz cmp_oct
   movfw I_out
   subwf I_oct,W
cmp_oct
   btfss STATUS,C
   goto do_it
   movlw FLOAT_CHARGE
   movwf charge_status
   goto do_it


chk_float:
do_float:
   ; if (U_out > V_float) Req--
   movfw V_oct+1 ;V_float+1
   subwf U_out+1,W
   bnz cmp_float
   movfw V_oct ;V_float
   subwf U_out,W
cmp_float
   btfss STATUS,C
   goto incReq
   btfsc STATUS,Z
   goto Control_done

   movlw 0xff        
   movwf Require
   movwf Require+1
   goto do_it

incReq
   movlw 1
   movwf Require
   clrf Require+1


;*********************************************************+

do_it:
   movf Require,W
   iorwf Require+1,W
   btfsc STATUS,Z
   goto Control_done

   btfss CHARGE_ON
   goto Control_done

   bcf INTCON,TMR0IE



   btfss Require+1,7
   call I_goUp
   btfsc Require+1,7
   call I_goDown


    bsf INTCON,TMR0IE

Control_done

   clrf Require
   clrf Require+1

;****************************************

     btfss RCSTA,SPEN  ; if  USART disable
     btfss DTR_PIN     ; and DTE inline
     goto RS_ON       
     bsf RCSTA,SPEN    ; enable USART
     bsf DSR_PIN       ; set DSR
RS_ON
     btfsc DTR_PIN     ; else if DTE offline
     goto RS_DONE
     bcf RCSTA,SPEN    ; disable USART
     bcf DSR_PIN       ; clear DSR
RS_DONE






     movfw RS_chkSum
     btfsc STATUS,Z     ; if RS_chkSum != 0 
     goto continue
                        ; we have received a command frame so
     bcf INTCON,GIE
     call check_cmd     ; Parse command frame
     bsf INTCON,GIE
continue:

     call RBInt_isr
     goto Loop




;***************************************




Timer0_isr
   bcf INTCON, TMR0IF
   incf t_cnt, F


   movfw Refresh_rate   ; 0x0e
   subwf t_cnt,W         ; 14 * 0.071s = 0.994
   btfss STATUS,Z
   return
       ;  case 0x0e -> +- 1.0s
   clrf t_cnt
   call read_temp

       ; print charge_status

   movlw 0x80
   pagesel LCDcomd
   call LCDcomd  ; set LINE 1
   movf charge_status, W
   addlw 3
   call OutText

;***************************************
   ; print Volts

   movlw 0xc0
   pagesel LCDcomd
   call LCDcomd
   movlw ' '
   call LCDdata


     movfw U_out
    banksel AARGB1
    movwf AARGB1
    banksel U_out
    movfw U_out+1
    banksel AARGB0
    movwf AARGB0
    movlw 0x7d ; 0x80
    movwf BARGB0
    movlw 0x47 ; 0x97
    movwf BARGB1
    pagesel FXM1616U
    call FXM1616U



    movfw AARGB3
    banksel LO
    movwf LO
    banksel AARGB2
    movfw AARGB2
    banksel HI
    movwf HI
    banksel AARGB1
    movfw AARGB1
    banksel LO_TEMP
    movwf LO_TEMP
    banksel AARGB0
    movfw AARGB0
    banksel HI_TEMP
    movwf HI_TEMP


    pagesel LCDval32
    call LCDval32

    movlw 'V'
    call LCDdata

;******************************
  ; print Amps
   movlw ' '
   call LCDdata
   movlw ':'
   call LCDdata


     movfw I_out
    banksel AARGB1
    movwf AARGB1
    banksel I_out
    movfw I_out+1
    banksel AARGB0
    movwf AARGB0
    movlw 0x5f
    movwf BARGB0
    movlw 0x73
    movwf BARGB1
    pagesel FXM1616U
    call FXM1616U



    movfw AARGB3
    banksel LO
    movwf LO
    banksel AARGB2
    movfw AARGB2
    banksel HI
    movwf HI
    banksel AARGB1
    movfw AARGB1
    banksel LO_TEMP
    movwf LO_TEMP
    banksel AARGB0
    movfw AARGB0
    banksel HI_TEMP
    movwf HI_TEMP


    pagesel LCDval32
    call LCDval32

    banksel LO



   movlw 'A'
   call LCDdata


;*******************************


  btfss RTS_PIN
  call Monitor

;************************************

   ; Show message symbols
;   banksel err_flag

   banksel err_flag

   btfsc NO_BATTERY
   goto msg2

   btfsc MSG_BLINK
   goto del_msg
   btfss LOW_INLET_VOLTAGE
   return
del_msg
   movlw 0x8f
   call LCDcomd
   movlw 0x20 ; space
   btfss MSG_BLINK
   movlw 0xb2      ;    LOW_INLET_VOLTAGE symbol
   call LCDdata
   bcf LOW_INLET_VOLTAGE
  ; set blink status
  comf err_flag,W
  andlw 0x02
  bcf MSG_BLINK
  iorwf err_flag,F
  return

msg2:

   btfsc MSG_BLINK
   goto del_msg2
   btfss NO_BATTERY
   return
del_msg2
   movlw 0x8f
   call LCDcomd
   movlw 0x20 ; space
   btfss MSG_BLINK
;   movlw 0xf8       ; NO_BATTERY Symbol
   movfw Err_Symbol
   call LCDdata
   bcf NO_BATTERY
  ; set blink status
  comf err_flag,W
  andlw 0x02
  bcf MSG_BLINK
  iorwf err_flag,F

   movlw 0x80
   call Beep


   return


;*****************************************************



    org 0x200


GetStrAddr:
    addwf PCL, F
    retlw Str0-Str0
    retlw Str1-Str0
    retlw Str2-Str0
    retlw Str3-Str0
    retlw Str4-Str0
    retlw Str5-Str0
    retlw Str6-Str0
    retlw Str7-Str0
    retlw Str8-Str0
    retlw Str9-Str0
    retlw Str10-Str0
    retlw Str11-Str0
    retlw Str12-Str0
    retlw Str13-Str0




;*****************************************************



GetChar
    addwf PCL, F
Str0:
    dt "CONFIG Tensao U",0x00  ; len=11
Str1:
    dt "CONFIG Ampere/h",0x00  ;  10
Str2:
    dt "   A INICIAR   ",0x00 ; 7
Str3:
    dt "    CALIBRAR   ",0x00  ; 9
Str4:
    dt "  Carga Lenta  ",0x00 ; 14
Str5:
    dt "  Carga Rapida ",0x00  ; 14
Str6:
    dt "   A Finalizar ",0x00  ; 14
Str7:
    dt "Carga Completa ",0x00  ; 14
Str8:
    dt " Temp.  ",0x00
Str9:
    dt "*   Control   *", 0x00 ; 15
Str10:
    dt "**** A Z A ****", 0x00 ; 16
Str11:
    dt "     ERRO ", 0x00 ; 16
Str12:
    dt "!!!! ALARME !!!!",0x00
Str13:
    dt "   Sem Bateria  ",0x00

;******************************************************************************



OutText
    movwf STR_NUM
   movlw high GetStrAddr
   movwf PCLATH
   movf STR_NUM,W
   call GetStrAddr
   movwf INDEX
   movlw high GetChar
   movwf PCLATH
next
    movf INDEX,W
    call GetChar
    addlw d'0'
    btfsc STATUS,Z
    goto done

    call LCDdata
    incf INDEX, F
    goto next
done
    return

;*****************************************************




;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


delay_ms            ; W*1ms  at 4MHz
    movwf delay_mult
del_20m             ; 20ms

    movlw  d'200'
    movwf delay_k200
;   clrwdt
del_2u
    nop
    nop
    nop
    decfsz delay_k200,f
    goto del_2u

    decfsz delay_mult,f
    goto del_20m
    return


;*****************************************

Beep1
   comf PORTC,W
   andlw 0x01
   bcf PORTC,0
   iorwf PORTC,F

   return



;*********************
;   fica em espera até que
;   termine o beep t1=0.5s

Beep
   movwf t1     ; beep time long
   movlw 0x1a
   movwf t2     ; frequency
loop1

   btfsc PORTC,0
   bcf PORTC,0
   btfss PORTC,0
   bsf PORTC,0

   decfsz t2
   goto $-1

   decfsz t1
   goto loop1
   return


;************************************************************************

I_goUp

   movfw CCPR1L
   sublw 0xfc
   btfsc STATUS,C
   goto I_UpOK

   ; !! Low inlet voltage?
    bsf LOW_INLET_VOLTAGE
    goto I_done

I_UpOK

    swapf CCP1CON,W
    andlw 0x03
    movwf Old
    incf Old
    btfsc Old,2
    incf CCPR1L,F
    movlw 0x03
    andwf Old
    swapf Old,F
    movfw CCP1CON
    andlw 0x0f
    iorwf Old,W
    movwf CCP1CON

   movlw .4
   call delay_ms
   clrf Old
   clrf Old+1
   goto in
WaitRizing:
   ; Require = Require - Old
    movfw Require
    subwf Old,W
    btfss STATUS,C
    decf Require+1,F
    movwf Require
    movfw Require+1
    subwf Old+1,W
    movwf Require+1
in
   movfw I_out
   movwf Old
   movfw I_out+1
   movwf Old+1

    movlw I_out
    movwf FSR
    movlw b'11010000'   ; AN2 I1
    call refresh

    movfw Old
    subwf I_out,W
    btfss STATUS,C
    decf Old+1,F
    movwf Old
    movfw Old+1
    subwf I_out+1,W
    movwf Old+1

   movfw Old
   iorwf Old+1,W
   btfss STATUS,Z
   goto WaitRizing

;************
    movfw Require
   iorwf Require+1,W
   btfss STATUS,C

   goto I_goUp

I_done

    return

;****************************************

I_goDown

    movf CCPR1L,W
   btfsc STATUS,Z
   goto I_Ddone

    swapf CCP1CON,W
    andlw 0x03
    movwf Old
    decf Old
    btfsc Old,7
    decf CCPR1L,F
    movlw 0x03
    andwf Old
    swapf Old,F
    movfw CCP1CON
    andlw 0x0f
    iorwf Old,W
    movwf CCP1CON

   movlw .4
   call delay_ms
   clrf Old
   clrf Old+1
   goto in1
WaitDown
    ; Require = Require + Old
    movfw Old
    addwf Require,F
    btfsc STATUS,C
    incf Require+1,F
    movfw Old+1
    addwf Require+1,F

    btfss Require+1,7
    goto I_Ddone
in1
   movfw I_out
   movwf Old
   movfw I_out+1
   movwf Old+1


    movlw I_out
    movwf FSR
    movlw b'11010000'   ; AN2 I_out
    call refresh

;    call refresh_data
    movfw I_out
    subwf Old,W
   btfss STATUS,C
   decf Old+1,F
   movwf Old
   movfw I_out+1
   subwf Old+1,F

   movfw Old
   iorwf Old+1,W
   btfss STATUS,Z
   goto WaitDown


   movfw Require+1
   btfss STATUS,Z
   goto I_goDown


I_Ddone

    return





;************************************************************************


; Read a Command frame data

UsartRX:
   bcf STATUS,RP0
   movfw RS_sz
   btfsc STATUS,Z      ; if frame size == 0
   goto init_frame     ; start build frame
                       ; if chkSum>0 return
   movfw RS_chkSum     ; else
   btfsc STATUS,Z      ; if frame not complete (RS_chkSum==0)
   goto Rec            ; add data to frame
   movfw RCREG         ; else discard data byte
   return
Rec:
   movlw RS_sz         ; frame buffer first byte address to W 
   addwf RS_sz,w       ; add current offset
   movwf FSR           ; update frame pointer index

  ; check error
   btfss RCSTA,FERR
   goto NO_FERR
   bsf RS_ERROR      
   movlw 0x46
   movwf Err_Symbol
NO_FERR:
   btfss RCSTA,OERR
   goto NO_OERR2
   bcf RCSTA,CREN
   bsf RS_ERROR
   movlw 0x4f
   movwf Err_Symbol
   bsf RCSTA,CREN     ;  enable continuous receive
NO_OERR2

   movfw RCREG        ; Load data
   movwf INDF         ; save data to frame buffer
   incf RS_sz,F       ; update offset
   movfw RS_sz 
   sublw 5            ; Frame Cmd as 5 bytes length
   btfsc STATUS,Z     ; check if it is last byte
   return             ; if so return
   btfss PIR1,RCIF    ; else wait new byte received
   goto $-1
   goto UsartRX

init_frame:
  ; check errors
   btfss RCSTA,FERR
   goto NO_ERR1
   bsf RS_ERROR
   movlw 0x46
   movwf Err_Symbol
NO_ERR1:
;   btfss RCSTA,OERR
;   goto NO_ERR2
;   bcf RCSTA,CREN
;   bsf RS_ERROR
;   movlw 0x4f
;   movwf Err_Symbol
;   bsf RCSTA,CREN
;NO_ERR2

   movfw RCREG     ; Load data to W
   sublw 5         ; teste frame ID
   btfss STATUS,Z  ; if data != frame ID
   return          ; abort 
   incf RS_sz,F    ; else increment frame size
   btfss PIR1,RCIF ; wait RX buffer full 
   goto $-1
   goto UsartRX


   ; cmd HI: 0 -> Read addr
   ;         1 -> Write addr

   ; cmd LO -> Mem/Bank
   ; Bank bit 0:
   ;            0 -> bank 0,1
   ;            1 -> bank 2,3
   ;      bit 1: ----------
   ;      bit 2,3:
   ;            00xx -> ROM
   ;            01xx -> RAM
   ;            10xx -> EEPROM

check_cmd

   clrf RS_sz
   btfsc RS_cmd,4    ; if cmd bit 4 == 1
   goto write_loop   ; goto write operation
                     ; else do read operation   
   ; do read
   clrf RS_chkSum
read_loop;
    btfss RS_cmd,3   ; if cmd bit 3 == 0
   goto RRam         ; read_ram
                     ; else read_eeprom
   ; eeprom
   movfw RS_addr
   addwf RS_sz,W
   call read_eeprom
   goto SendToUART   ; while len > 0 send requested data byte
RRam:
   movfw RS_addr     ; get frame buffer addr
   addwf RS_sz,W     ; add current offset
   movwf FSR         ; update pointer to read from
   movfw INDF        ; get the data
SendToUART:
   banksel TXSTA
   btfss TXSTA,TRMT
   goto $-1          ; wait buffer empty
   banksel TXREG
   movwf TXREG         ; save requested data to output buffer
   addwf RS_chkSum,F   ; update checksum
   incf RS_sz,F        ; increment current offset
   decfsz RS_len,F
   goto read_loop      ; loop while len > 0 
   clrf RS_sz
     banksel TXSTA
     btfss TXSTA,TRMT  ; wait TX buffer empty
     goto $-1          
     banksel TXREG
   movfw RS_chkSum
   movwf TXREG         ; move RS_chkSum to TX buffer
   clrf RS_chkSum      ; reset RS_chkSum as a empty frame  
   return
write_loop:
   bsf CTS_PIN
   btfss PIR1,RCIF   ; wait RX buffer full
   goto $-1
  ; check error
   btfss RCSTA,FERR
   goto NO_ERR
   bsf RS_ERROR
   movlw 0x46
   movwf Err_Symbol
NO_ERR:
;   btfss RCSTA,OERR
;   goto NO_OERR1
;   bcf RCSTA,CREN
;   bsf RS_ERROR
;   movlw 0x4f
;   movwf Err_Symbol
;   bsf RCSTA,CREN
NO_OERR1

   movfw RCREG       ; Load received byte
   movwf RS_chkSum   ; Use RS_chkSum as a temporary buffer

   btfss RS_cmd,3    ; if MemType == RAM
   goto WRam         ; write ram
  ; write_eeprom
   banksel EEDATA
   movwf EEDATA
   banksel RS_addr
   movfw RS_addr
   addwf RS_sz,W
   banksel EEADR
   movwf EEADR
   call write_eeprom
   banksel RS_len
   incf RS_sz,F
   decfsz RS_len,F
   goto write_loop
   clrf RS_sz
   clrf RS_chkSum
   bcf CTS_PIN
   return
WRam:
   movfw RS_addr   ; Load dest address
   addwf RS_sz,W   ; Add current offset
   movwf FSR       ; Update pointer address
   movfw RS_chkSum ; Load data stored at temp buffer
   movwf INDF      ; Write data to destination address
   incf RS_sz,F    ; Update current offset
   decfsz RS_len,F ; Loop while bytes length
   goto write_loop
   clrf RS_sz
   clrf RS_chkSum   ; reset RS_chkSum so we can use it to signaled 
   bcf CTS_PIN      ; a received frame
   return

;**********************************


   ; cmd HI  == 0 -> Read addr
   ; cmd HI  == 1 -> Write addr
   ; cmd LO -> Mem/Bank
   ; Bank bit 0:1 = bank 0,1,2,3
   ;          3:2 = 00xx -> ROM
   ;          3:2 = 01xx -> RAM
   ;            3 = 1xxx -> EEPROM




;*************************************************



Monitor:
   banksel MonList
   movfw ListSz
   btfsc STATUS,Z
   return
   clrf nList
     btfss TXSTA,TRMT
     goto $-1    ; wait buffer empty
     banksel TXREG
     movlw 0xaa
     movwf TXREG  ; Send frame ident DataMonitor
    banksel nData
   movwf Sum
mon_loop
   ; Ptr = ListAddr+nList*2
   clrf nData
   bcf STATUS,C
   rlf nList,W
   movwf t4
   movlw MonList
   addwf t4,W
   movwf FSR
   movfw INDF
   movwf dataLen
   incf FSR
   movfw INDF
   movwf FSR
next_byte
   movfw INDF
     btfss TXSTA,TRMT
     goto $-1    ; wait buffer empty
     banksel TXREG
   movwf TXREG
   banksel Sum
   addwf Sum
   incf FSR
;   banksel nData
   incf nData
   movfw nData
   subwf dataLen,W
   btfss STATUS,Z
   goto next_byte
   incf nList,F
   decfsz ListSz
   goto mon_loop
   movfw nList
   movwf ListSz
     btfss TXSTA,TRMT
     goto $-1    ; wait buffer empty
   movfw Sum
   banksel TXREG
   movwf TXREG

   return


;*****************************************




;*************************************************




;*****************************************************





;*****************************************************




ShowData

   movlw b'11000000' ; LCDL2
   call LCDcomd
   movlw 'U'
   call LCDdata
   movlw '='
   call LCDdata
   movf U_out,W
   movwf  LO
   movf (U_out+1),W
   movwf  HI
   call LCDval16
   movlw 'V'
   call LCDdata
   movlw '/'
   call LCDdata

   movlw 'I'
   call LCDdata
   movlw '='
   call LCDdata

   movf  I_out,W
;   addwf I_cal, W
   movwf  LO
   movf (I_out+1),W
   movwf  HI
   call LCDval16
  return

;**************************************************

PressOK
    WAIT .100
    btfss PORTB,5
    goto $-1


    retlw 0

;**********************************************************

RBInt_isr
;    banksel INTCON
;    bcf INTCON, RBIF
    btfss PORTB,5
    call PressOK
    btfsc PORTB,7   ; UP
    return
    btfsc PORTB,6   ; Down
    return
    WAIT .100
    btfss PORTB,6
    goto $-1
    btfss PORTB,7
    goto $-1
    WAIT .100
 ;     Calibrate
   bcf INTCON,TMR0IE
    movlw 1
    call LCDcomd     ; Clear LCD
   movlw b'10000000'
   call LCDcomd    ; Set LINE 1
    movlw 3         ; Print "CALIBRAR"
    call OutText

    call set_Vadjust
    call set_Iadjust

   movlw b'10000000'
   call LCDcomd    ; Set LINE 1
   movf charge_status, W
   addlw 3
   call OutText
   call ShowData
   bsf INTCON,TMR0IE
    return


;*****************************************************

print_err
  movwf Old
  movlw 0x80
  call LCDcomd
  movlw .12    ; ALARM
  call OutText
  movlw 0xC0
  call LCDcomd
  movfw Old    ; Error msg
  call OutText

ErrLoop
   movlw 0x80 ;
   call Beep
   movlw 0xff
   call delay_ms
   movlw 0xff
   call delay_ms
   goto ErrLoop




;*****************************************************



LCDval08
	movwf	LO
	movwf	LO_TEMP		; LO -> LO_TEMP
	bcf	BCflag		; blank checker for preceeding zeros

	movlw	d'100'		; check amount of 100s
	movwf	TEMP2		; ==> Decimal Range 0 - 255 <=> 8 bit
	call	_VALcnv08	; call conversion sub-routine
    btfsc   Digit2
	LCDw			; call LCD sub-routine with value stored in w

	movlw	d'10'		; check amount of 10s
	movwf	TEMP2
	call	_VALcnv08	; call conversion sub-routine
    btfsc   Digit1
	LCDw			; call LCD sub-routine with value stored in w

	movlw	d'1'		; check amount of 1s
	movwf	TEMP2
	bsf	BCflag		; remove blank checker in case of zero
	call	_VALcnv08	; call conversion sub-routine
    btfsc   Digit0
	LCDw			; call LCD sub-routine with value stored in w
	RETURN


;****************************************



_VALcnv08
	clrf	TEMP1		; counter
	movfw	TEMP2		; decrement-value
_V08_1	subwf	LO_TEMP,W	; TEST: LO_TEMP-TEMP2 >= 0 ?
	skpc    	; btfss STATUS,C skip, if true
	goto	_V08_LCD	; result negativ, exit
	incf	TEMP1,F		; count
	movfw	TEMP2		; decrement-value
	subwf	LO_TEMP,F	; STORE: LO_TEMP = LO_TEMP - TEMP2
	bsf	BCflag		; invalidate flag
	goto	_V08_1		; repeat
_V08_LCD
	movlw	'0'		; writes Number to LCD
	addwf	TEMP1,W		; '0' is ascii offset, add counter
    btfsc LJustify
    return
	btfss	BCflag		; check flag
	movlw ' '		; clear preceeding zeros
	; return with data in w
	RETURN


;*****************************************************


LCDval16
	movfw	LO		; LO -> LO_TEMP
	movwf	LO_TEMP
	movfw	HI		; HI -> HI_TEMP
	movwf	HI_TEMP
	bcf	BCflag		; Blank checker for preceeding zeros

	movlw	b'00010000'	; check amount of 10000s
	movwf	TEMP2		; Sub-LO
	movlw	b'00100111'
	movwf	TEMP3		; Sub-HI
	call	_VALcnv16	; call conversion sub-routine
	LCDw			; call LCD sub-routine with value stored in w

	movlw	b'11101000'	; check amount of 1000s
	movwf	TEMP2		; Sub-LO
	movlw	b'00000011'
	movwf	TEMP3		; Sub-HI
	call	_VALcnv16	; call conversion sub-routine
	LCDw			; call LCD sub-routine with value stored in w

	movlw	b'01100100'	; check amount of 100s
	movwf	TEMP2		; Sub-LO
	clrf	TEMP3		; Sub-HI is zero
	call	_VALcnv16	; call conversion sub-routine
	LCDw			; call LCD sub-routine with value stored in w

	movlw	b'00001010'	; check amount of 10s
	movwf	TEMP2		; Sub-LO
	clrf	TEMP3		; Sub-HI is zero
	call	_VALcnv16	; call conversion sub-routine
	LCDw			; call LCD sub-routine with value stored in w

	movlw	b'00000001'	; check amount of 1s
	movwf	TEMP2		; Sub-LO
	clrf	TEMP3		; Sub-HI is zero
	bsf	BCflag		; remove blank checker in case of zero
	call	_VALcnv16	; call conversion sub-routine
	LCDw			; call LCD sub-routine with value stored in w
	RETURN


;*****************************************************



_VALcnv16
	clrf	TEMP1		; clear counter
_V16_1	movfw	TEMP3
	subwf	HI_TEMP,w	; TEST: HI_TEMP-TEMP3 >= 0 ?
	skpc			; skip, if true
	goto	_V16_LCD	; result negativ, exit
	bnz	_V16_2		; test zero, jump if result > 0
	movfw	TEMP2		; Precondition: HI-TEST is zero
	subwf	LO_TEMP,w	; TEST: LO_TEMP-TEMP2 >= 0 ?
	skpc			; skip, if true
	goto	_V16_LCD	; result negativ, exit
_V16_2
	movfw	TEMP3
	subwf	HI_TEMP,f	; STORE: HI_TEMP = HI_TEMP - TEMP3
	movfw	TEMP2
	subwf	LO_TEMP,f	; STORE: LO_TEMP = LO_TEMP - TEMP2
	skpc			; skip, if true
	decf	HI_TEMP,f	; decrement HI
	incf	TEMP1,f		; increment counter
	bsf	BCflag		; invalidate flag
	goto	_V16_1
_V16_LCD
	movlw	'0'		; writes number to LCD
	addwf	TEMP1,w		; '0' is ascii offset, add counter
	btfss	BCflag		; check flag
	movlw	' '		; clear preceeding zeros
	; return with data in w
	RETURN

;********************************************************************


LCDval32
   bcf BCflag
   movlw 0x80
   movwf TEMP2
   movlw 0x96
   movwf TEMP3
   movlw 0x98
   movwf TEMP4
   clrf TEMP5
   call _Valcnv32
   call LCDdata
   bsf BCflag

   movlw 0x40
   movwf TEMP2
   movlw 0x42
   movwf TEMP3
   movlw 0x0f
   movwf TEMP4
   clrf TEMP5
   call _Valcnv32
   call LCDdata

   movlw ','
   call LCDdata

   movlw 0xa0
   movwf TEMP2
   movlw 0x86
   movwf TEMP3
   movlw 0x01
   movwf TEMP4
   clrf TEMP5
   call _Valcnv32
   call LCDdata

   movlw 0x10
   movwf TEMP2
   movlw 0x27
   movwf TEMP3
   clrf TEMP4
   clrf TEMP5
   call _Valcnv32
   call LCDdata

 ;  movlw 0xe8
 ;  movwf TEMP2
 ;  movlw 0x03
 ;  movwf TEMP3
 ;  clrf TEMP4
 ;  clrf TEMP5
 ;  call _Valcnv32
 ;  call LCDdata

 ;  movlw 0x64
 ;  movwf TEMP2
 ;  clrf TEMP3
 ;  clrf TEMP4
 ;  clrf TEMP5
 ;  call _Valcnv32
 ;  call LCDdata

 ;  movlw 0x0a
 ;  movwf TEMP2
 ;  clrf TEMP3
 ;  clrf TEMP4
 ;  clrf TEMP5
 ;  call _Valcnv32
 ;  call LCDdata

 ;  movlw 0x01
 ;  movwf TEMP2
 ;  clrf TEMP3
 ;  clrf TEMP4
 ;  clrf TEMP5
 ;  call _Valcnv32
 ;  call LCDdata
   return


_Valcnv32
   clrf TEMP1
_V32_1
   movfw TEMP5
   subwf HI_TEMP,w
   skpc
   goto _V32_LCD
   bnz _V32_2

   movfw TEMP4
   subwf LO_TEMP,w
   skpc
   goto _V32_LCD
   bnz _V32_2

   movfw TEMP3
   subwf HI,w
   skpc
   goto _V32_LCD
   bnz _V32_2

   movfw TEMP2
   subwf LO,w
   skpc
   goto _V32_LCD
_V32_2
   movfw TEMP5
   subwf HI_TEMP,f
   movfw TEMP4
   subwf LO_TEMP,f
   skpc
   decf HI_TEMP
   movfw TEMP3
   subwf HI,f
   skpc
   decf LO_TEMP
   movfw TEMP2
   subwf LO,f
   skpc     ; skeep if carry
   decf HI,f
   incf TEMP1,f
   bsf BCflag
   goto _V32_1
_V32_LCD
   movlw '0'
   addwf TEMP1,w
   btfss BCflag
   movlw ' '
   return


;*********************************************************************************************
;   Preserv W


PrintV8
   movwf Temp
   movlw b'11000000' ; LCDL2
   call LCDcomd
   movlw ' '
   call LCDdata
   movf typ,W
   call LCDdata
   movlw ' '
   call LCDdata
   movlw '-'
   call LCDdata
   movlw '>'
   call LCDdata
   movlw ' '
   call LCDdata
   movf Temp,W
   call LCDval08
   movf uni,W
   call LCDdata
   movf Temp, W
   return


;--------------------------------------------------------------------------



;***** SUBROUTINES *****

	; transmit only lower nibble of w
 LCDxmit	movwf	LCDbuf		; store command/data nibble
	; first, clear LCD data lines
	clrLCDport
	; second, move data out to LCD data lines
	movf	LCDbuf,w	; get data
	andlw	b'00001111'	; extract only valid part
	iorwf	LCDport,f	; put to LCD data lines
	RETURN

;*****************************************************


	; clocks LCD data/command
LCDclk	;WAIT	LCDWAIT
    movlw LCDWAIT
    call delay_ms
    bsf     LCD_EN		; set LCD enable
	; insert LCDSPEED x nops to comply with manufacturer
	; specifications for clock rates above 9 MHz
	VARIABLE CNT_V		; declare intermediate variable
	CNT_V = LCDSPEED	; assign pre-defined constant
	WHILE (CNT_V > 0x0)	; perform while loop to insert 'nops'
	  nop			; insert 'nop'
	  CNT_V -= 1		; decrement
	ENDW
	bcf     LCD_EN
	WAIT	LCDWAIT		; clocks LCD data/command
	RETURN


;*****************************************************


	; transmit command to LCD
LCDcomd
    bcf	LCD_RS		; select command registers
	goto	_LCD_wr

	; transmit data to LCD
LCDdata	bsf	LCD_RS		; select data registers
_LCD_wr	 
	movwf	LCDtemp		; store command/data to send
	; send hi-nibble
	movfw	LCDtemp		; get data
	swapf	LCDtemp,w	; swap hi- and lo-nibble, store in w
	call	LCDxmit		; transmit nibble
	call	LCDclk
	; send lo-nibble
	movfw	LCDtemp		; get data
	call	LCDxmit		; transmit nibble
	call	LCDclk
	; reset LCD controls
	clrLCDport		; reset LCD data lines
	bcf	LCD_RS		; reset command/data register
	RETURN

;*****************************************************

LCDinit
	bcf	LCD_EN		; clear LCD clock line
	bcf	LCD_RS		; clear command/data line
	clrLCDport		; reset LCD data lines
	WAIT	4*LCDWAIT	; >= 4 ms @ 4 MHz

	; LCD INITIALIZATION STARTS HERE
	; start in 8 bit mode
	movlw	LCDEM8		; send b'0011' on [DB7:DB4]
	call	LCDxmit		; start in 8 bit mode
	call	LCDclk		; That's while:
	WAIT	LCDWAIT 	; On POWER UP, the LCD will initialize itself,
				; but after a RESET of the microcontroller without
				; POWER OFF, the 8 bit function mode will reboot
				; the LCD to 4 bit mode safely.

	movlw	LCDDZ		; set DDRAM to zero
	call	LCDxmit
	call	LCDclk
	WAIT	LCDWAIT		; ~1 ms @ 4 MHz

	movlw	LCDEM4		; send b'0010' on [DB7:DB4]
	call	LCDxmit		; change to 4 bit mode
	call	LCDclk
	WAIT	LCDWAIT		; ~1 ms @ 4 MHz

	; now in 4 bit mode, sending two nibbles
	IF LCDTYPE == 0x00
	  LCDcmd LCD2L		; function set: 4 bit mode, 2 lines, 5x7 dots
	  LCDcmd LCDCONT	; display control: display on, cursor off, blinking off
	  LCDcmd LCDCLR		; clear display, address counter to zero
	  WAIT LCDCLRWAIT	; wait after LCDCLR until LCD is ready again
	ELSE
	  IF LCDTYPE == 0x01
	    ; for LCD EA DIP204-4 (white chars, blue backlight)
	    LCDcmd LCD2L_A	; switch on extended function set
	    LCDcmd LCDEXT	; 4 lines
	    LCDcmd LCD2L_B	; switch off extended function set
	    LCDcmd LCDCONT	; display control: display on, cursor off, blinking off
	    LCDcmd LCDCLR	; clear display, address counter to zero
	    WAIT LCDCLRWAIT	; wait after LCDCLR until LCD is ready again
	  ELSE
	    ERROR "Unsupported parameter"
	  ENDIF
	ENDIF
    return






;**********************************************************************************************

;   Sistema grupo de baterias
;            12V/24v


initdata
;Read batt U 12V/24V
   movlw 'U'
   movwf typ
   movlw 'V'
   movwf uni
   movlw U_MODE
   call read_eeprom
   banksel V_batt
   movwf V_batt
   sublw 0xff
   btfsc STATUS,Z
   call set_voltage


;-------------------------------------------

; 12Ah / 40Ah / 62Ah / 90Ah / 160Ah
;  Read battery capacity last option

   movlw 1  ; LCDClear
   call LCDcomd

   movlw 'P'
   movwf typ
   movlw 'A'
   movwf uni
   movlw W_CAP
   call read_eeprom
   banksel W_batt
   movwf W_batt
   sublw 0xff
   btfsc STATUS,Z
   call set_capacity



;-----------------------------------------------------

   call calc_V24
   movf V_batt,W
   sublw 0x0c
   btfsc STATUS,Z   ; if(V_batt == .12)
   call calc_V12    ;      call calc_V12
   movlw 0xa0       ;
   subwf W_batt,W   ;
   btfsc STATUS,Z   ; if( W_batt == 160A)
   call calc_I160
   movlw 0x5a
   subwf W_batt,W
   btfsc STATUS,Z   ; if( W_batt == 90A)
   call calc_I90
   movlw 0x3E
   subwf W_batt,W
   btfsc STATUS,Z   ; if(W_batt == 62A)
   call calc_I62
   movlw 0x28
   subwf W_batt,W
   btfsc STATUS,Z   ; if(W_batt == 40A)
   call calc_I40
   movlw 0x0c
   subwf W_batt,W
   btfsc STATUS,Z   ; if(W_batt == 12A)
   call calc_I12



  ; Check if user as modify some config stuff
  ; Check Read loop counter
   movlw U_ADJ
   call read_eeprom
   banksel V_cal
   movwf V_cal

   movlw R_CNT
   call read_eeprom
   banksel ReadCnt
   movwf ReadCnt
   sublw 0xff
   btfss STATUS,Z
   goto cfg_done
   movlw 1
   movwf ReadCnt

cfg_done:
   return







;*****************************************************************************



refresh
    call read_ad
    banksel ADRESL
    movfw ADRESL
    movwf INDF
    incf FSR
    banksel ADRESH
    movfw ADRESH
    movwf INDF
    return
 






;***********************************************************************************************



read_eeprom
    banksel EEADR                ; bank2
    movwf EEADR          ; address to read
    banksel EECON1
    bcf EECON1, EEPGD    ; point to data mem
    bsf EECON1, RD       ; EE begin read
    btfsc EECON1,RD
    goto $-1
    banksel EEDATA
    movf EEDATA, w       ; EEDATA -> w
    return


;*******************************************************************************

; ATENÇÃO! as interrupções terão de
; estar desligadas para não interromper o processo

write_eeprom
    banksel EECON1
    bcf EECON1,EEPGD    ; Point to data
    bsf EECON1, WREN    ; Enable writes
    movlw 0x55          ;
    movwf EECON2
    movlw 0xaa
    movwf EECON2
    bsf EECON1, WR      ; Set WR bit to
    nop                 ; begin write
    btfsc EECON1,WR
    goto $-1
    bcf EECON1, WREN
    return


;*************************************************************************************

Wavail addwf PCL,F
      retlw  d'12' ; 0x0c
      retlw  d'40' ; 0x28
      retlw  d'62' ; 0x3e
      retlw  d'90' ; 0x5a
      retlw d'160' ; 0xa0



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


getParam
   movwf save_tmp
   movlw high Wavail
   movwf PCLATH
   movlw 0
nxt
   movwf t1
   movf t1, W
   call Wavail
   subwf save_tmp, W
   btfsc STATUS, Z
   goto done_ok
   incf t1,W
   goto nxt
done_ok
   movf t1,W
  clrf PCLATH
   return


set_capacity
   addlw 0
   btfss STATUS, Z
   call getParam
   movwf val
   movlw b'10000000' ; LCDL1
   pagesel LCDcomd
   call LCDcomd
   movlw d'1'
   call OutText
   movf val, W
   call doit
   movf W_batt, W
   call PrintV8
   movlw 'h'
   call LCDdata
confirmW
;   clrwdt
   btfss PORTB,7  ; Up
   call addW
   btfss PORTB,6  ; Down
   call decW
   btfsc PORTB,5  ; Ok
   goto confirmW
   WAIT .100
   btfss PORTB, 5
   goto $-1
   WAIT .100
;  save batt capacity
   movf W_batt, W   ; W -> capacity
   banksel EEDATA
   movwf EEDATA     ; W -> EEDATA
   movlw W_CAP      ; eeprom addr
   movwf EEADR
   pagesel write_eeprom
   call write_eeprom
   banksel W_batt
   return


doit
   movwf val
   movf PCLATH, W
   movwf save_pclath
   movlw high Wavail
   movwf PCLATH
   movf val,W
   call Wavail
   movwf W_batt
   movf save_pclath, W
   movwf PCLATH
   return

addW
   WAIT .100
   btfss PORTB, 7
   goto $-1
   WAIT .100

   movf val,W
   addlw d'1'
   andlw b'00000011'
   btfsc STATUS,Z
   movlw 4  ;  max index to items in Wavail list
   call doit

   movf W_batt, W
   call PrintV8
   movlw 'h'
   call LCDdata
   return

decW
   WAIT .100
   btfss PORTB, 6
   goto $-1
   WAIT .100

   movf val, W
   btfss STATUS,Z
   decf val,F
   movf val, W
   call doit

   movf W_batt, W
   call PrintV8
   movlw 'h'
   call LCDdata
   return


;*********************************************************************************



set_voltage
   addlw 0
   btfsc STATUS,Z
   movlw 0x0c
   movwf V_batt
   movlw b'10000000' ; LCDL1
   pagesel LCDcomd
   call LCDcomd
   movlw d'0'
   call OutText
   movf V_batt, W
   call PrintV8
confirmV
;   clrwdt
   btfss PORTB,7 ; Up
   call addV
   btfss PORTB,6 ;Down
   call subV
   btfsc PORTB,5 ; Ok
   goto confirmV
   WAIT .100
   btfss PORTB, 5
   goto $-1
   WAIT .100
   pagesel set_voltage
    movf V_batt, W
   banksel EEDATA
   movwf EEDATA
   movlw U_MODE
   movwf EEADR
   call write_eeprom
   banksel V_batt

   return



;**********************************************************************************


addV
   WAIT .100
   btfss PORTB, 7
   goto $-1
   WAIT .100

   movf V_batt, W
   sublw  0x18
   btfss STATUS,Z
   addwf V_batt,f ; 0x0c
   movf V_batt,w  ;  V_batt -> w
   call PrintV8
   return



;***********************************************************************************



subV
   WAIT .100
   btfss PORTB, 6
   goto $-1
   WAIT .100

   movf V_batt, W
   sublw 0x18
   btfss STATUS,Z
   goto done1
   movlw 0x0c
   subwf  V_batt,f  ;1
done1
   movf V_batt,W
   call PrintV8
   return



;*******************************************************************************




set_Vadjust
   movlw U_ADJ
   call read_eeprom
   banksel V_cal
   movwf V_cal

     ; prepare params to PrintVal16

;xx   movf U_out+1,W
   movlw 0
   movwf HI
   movwf LO

   movlw 0xc0
   call LCDcomd
   movlw ' '
   call LCDdata
   movlw 'U'
   call LCDdata
   movlw '-'
   call LCDdata
   movlw '>'
   call LCDdata
   movlw ' '
   call LCDdata
   pagesel LCDval08
   call LCDval08
   pagesel LCDdata
   movlw 'V'
   call LCDdata

confirmAdj
;   clrwdt
   btfsc PORTB,7  ; Up
   goto B1
   WAIT .100
   btfss PORTB, 7
   goto $-1
   WAIT .100
   incf V_cal,F
     ; prepare params to PrintVal16
   movf V_cal,W
   movwf LO
     movlw 0xc5
   call LCDcomd
   call LCDval16
   movlw 'V'
   call LCDdata
B1
   btfsc PORTB,6  ; Down
   goto B2
   WAIT .100
   btfss PORTB, 6
   goto $-1
   WAIT .100

   decf V_cal,F
     ; prepare params to PrintVal16
;x   movf U_out+1,W
;x   movwf HI
   movf V_cal,W
;x   addwf U_out,W

   movwf LO
   movlw 0xc5
   call LCDcomd
   call LCDval16
   movlw 'V'
   call LCDdata

B2
   btfsc PORTB,5  ; Ok
   goto confirmAdj
   WAIT .100
   btfss PORTB, 5
   goto $-1
   WAIT .100
   movf V_cal,W
   banksel EEDATA
   movwf EEDATA
   movlw U_ADJ
   movwf EEADR
   call write_eeprom
   banksel V_cal

   return


;*****************************************************************************



set_Iadjust

   movlw I_ADJ
   call read_eeprom
   banksel I_cal
   movwf I_cal
     ; prepare params to PrintVal16

   movf I_out+1,W
   movwf HI
   movf I_out,W
   addwf I_cal,W
   movwf LO
   movlw 0xc0
   call LCDcomd
   movlw ' '
   call LCDdata
   movlw 'I'
   call LCDdata
   movlw '-'
   call LCDdata
   movlw '>'
   call LCDdata
   movlw ' '
   call LCDdata
   call LCDval16
   movlw 'A'
   call LCDdata
confirmIAdj
;   clrwdt
   btfsc PORTB,7  ; Up
   goto C1
   WAIT .100
   btfss PORTB, 7
   goto $-1
   WAIT .100

   incf I_cal,F
     ; prepare params to PrintVal16
   movf I_out+1,W
   movwf HI
   movf I_cal,W
   addwf I_out,W
   movwf LO
   movlw 0xc5
   call LCDcomd
   call LCDval16
   movlw 'A'
   call LCDdata
C1
   btfsc PORTB,6  ; Down
   goto C2
   WAIT .100
   btfss PORTB, 6
   goto $-1
   WAIT .100

   decf I_cal,F
   movf I_cal,W
     ; prepare params to PrintVal16
   movf I_out+1,W
   movwf HI
   movf I_cal,W
   addwf I_out,W
   movwf LO
   movlw 0xc5
   call LCDcomd
   call LCDval16
   movlw 'A'
   call LCDdata
C2
   btfsc PORTB,5  ; Ok
   goto confirmIAdj
   WAIT .100
   btfss PORTB, 5
   goto $-1
   WAIT .100
   movf I_cal,W
   banksel EEDATA
   movwf EEDATA
   movlw I_ADJ
   movwf EEADR
   call write_eeprom
   banksel I_cal
   return





;****************************************************************************


;                                                                     *
;   Descarregada !           <      1.75        V_off                 *
;   Flutuação (normal)            2.15/2.20     V_float               *
;   Equalização (recarga)         2.36/2.40     V_oct                 *
;   Sobretensão (sobrecarga  >      2.70                              *
;                                                                     *
;   Equalização (recarga)    10% da capacidade   I_blk                *
;   Flutuaçao                 1%  "       "      I_tric               *
;   Retenção                 I_blk/5             I_oct                *
;                                                                     *


;********************************************************


;    U = 6.7353 * (5*AD/1023)

calc_V12
    movlw 0x01
    movwf V_off+1     ; limite inferior
    movlw 0x3f        ; 10.50   (6*1.75)
    movwf V_off       ; 0x013E (AD319)
    movlw 0x01
    movwf V_float+1   ; 12.9 -> 13,2
    movlw 0x90        ; AD400
    movwf V_float
    movlw 0x01        ;
    movwf V_oct+1     ; limite superior
    movlw 0xb5        ; 14.16 -> 14.40
    movwf V_oct       ; AD 437
    movlw 0x01
    movwf Charge_Triger+1
    movlw 0x75
    movwf Charge_Triger
   return



;****************************************************************************************

calc_V24
    movlw 0x02
    movwf V_off+1     ;  limite inferior
    movlw 0x7d        ;  12 * 1.75 = 21v
    movwf V_off       ;  0x027d (AD637)
    movlw 0x03        ;
    movwf V_float+1   ;
    movlw 0x21        ; 12 * 2.17 = 26.04v
    movwf V_float     ; 0x0321  (AD801)
    movlw 0x03
    movwf V_oct+1     ; limite superior
    movlw 0x63        ; 12 * 2.38 = 28.56v
    movwf V_oct       ; 0x0363  (AD867)
    movlw 0x02
    movwf Charge_Triger+1
    movlw 0xeb
    movwf Charge_Triger
   return



;****************************************************************************************

 ;  Batt de 12Ah  I_tric =120mA
 ;  I = (5 / 1023) * AD / R


calc_I12
    ;W_bat 12A (0x0c)

    movlw 0x05   ; AD8  (120mA) carga lenta
    movwf I_tric ; T< V_off
    movlw 0
    movwf I_tric+1
    movlw 0x31  ; AD82 (1,2A)  equalização
    movwf I_blk  ; de V_off -> V_float
    movlw 0
    movwf I_blk+1
    movlw 0x09   ; AD16  (240mA) flutuação
    movwf I_oct  ; de V_float -> V_oct
    movlw 0
    movwf I_oct+1
    return


calc_I40
    ;W_bat 40A (0x27)
    movlw 0x10   ; AD41  (400mA) carga lenta
    movwf I_tric ; T< V_off
    movlw 0
    movwf I_tric+1
    movlw 0xa3   ; AD270 (4A)  equalização
    movwf I_blk  ; de V_off -> V_float
    movlw 0
    movwf I_blk+1
    movlw 0x20   ; AD54  (800mA) flutuação
    movwf I_oct  ; de V_float -> V_oct
    movlw 0
    movwf I_oct+1
    return


calc_I62
    ;W_bat 62Ah (0x3e)
    movlw 0x19   ; AD41  (620mA) carga lenta
    movwf I_tric ; T< V_off
    movlw 0
    movwf I_tric+1
    movlw 0xfd  ; AD418 (6,2A)  equalização
    movwf I_blk  ; de V_off -> V_float
    movlw 0
    movwf I_blk+1
    movlw 0x31   ; AD83  (1210mA) flutuação
    movwf I_oct  ; de V_float -> V_oct
    movlw 0
    movwf I_oct+1
    return



calc_I90
    ;W_bat 90A (0x5a)
    movlw 0x24   ; AD60  (900mA) carga lenta
    movwf I_tric ; T< V_off
    movlw 0
    movwf I_tric+1
    movlw 0x70  ; AD607 (9A)  equalização
    movwf I_blk  ; de V_off -> V_oct
    movlw 0x01
    movwf I_blk+1
    movlw 0x49   ; AD121  (1800mA) flutuação
    movwf I_oct  ; de V_float -> V_oct
    movlw 0
    movwf I_oct+1
    return

calc_I160
    ;W_bat 160A (0xa0)
    movlw 0x41   ; AD108  (1600mA) carga lenta
    movwf I_tric ; T< V_off
    movlw 0
    movwf I_tric+1
    movlw 0x8e    ;  AD1023 (16A)  equalização
    movwf I_blk  ; de V_off -> V_oct
    movlw 0x02
    movwf I_blk+1
    movlw 0x82   ; AD216  (3200mA) flutuação
    movwf I_oct  ; de V_float -> V_oct
    movlw 0
    movwf I_oct+1
  return

;******************************************************


read_ad
    bcf INTCON,GIE
    movwf ADCON0
    bsf ADCON0,ADON
    movlw 0x14   ; 20us
     movwf t3
     decfsz t3,F
     goto $-1   ; aquisition time 20us

    bsf ADCON0, GO
    btfsc ADCON0,GO
    goto $-1

    bcf ADCON0,ADON
    bsf INTCON,GIE
    return



;******************************************************************************



;*********************************************************************************




read_temp
    banksel ADCON1
    bcf ADCON1,ADFM   ; left justify (8 bits)
    banksel ADCON0
;read temp AN3
    movlw b'11011001'
    movwf ADCON0
    movlw 0x14
    movwf t2
    decfsz t2, F        ; wait 20us
    goto $-1
    bsf ADCON0, GO
    btfsc ADCON0,GO
    goto $-1
    bcf ADCON0,ADON
    movf ADRESH,w
    movwf t_environ
    banksel ADCON1
    bsf ADCON1,ADFM   ; right justify (10 bits)
    banksel t_environ


    return



;*****************************************************************************

;;;;; Calculate  RI
; return  U_out - u1



;******************************************************









     udata


delay_mult   equ 0x35
delay_k50    equ 0x36
delay_k200   equ 0x37
Temp          equ 0x38     ; PrintV8

w_save       equ 0x3a      ; + bank1
fsr_save     equ 0x3b
status_save  equ 0x3c      ; + bank1
pclath_save  equ 0x3d
option_save  equ 0x3e

t_cnt        equ 0x3f


charge_status  equ 0x41
t_environ      equ 0x42

save_pclath   equ 0x43
save_tmp      equ 0x44

err_flag       equ 0x45


val          equ 0x46       ; setCapacity
uni          equ 0x47
typ          equ 0x48

W_batt        equ 0x49
V_batt        equ 0x4a

t3            equ 0x4c
ReadCnt       equ 0x4d
Require        equ 0x4e  ; Store difference bettwen ( V_xx - U_out)


V_off         equ 0x50
V_float       equ 0x52
V_oct         equ 0x54
I_tric        equ 0x56
I_blk         equ 0x58
I_oct         equ 0x5a
Amp_H         equ 0x5c

U_in          equ 0x60
U_out         equ 0x62
I_out         equ 0x64

V_cal         equ 0x66 ; U calbration
I_cal         equ 0x68  ; I calibration

Old           equ 0x6a      ; GoUp , GoDown
Refresh_rate  equ 0x6c

t1         equ 0x6e
t2         equ 0x6f

RS_sz       equ 0x70
RS_cmd     equ 0x71
RS_addr     equ 0x72
RS_len     equ 0x73
RS_chkSum   equ 0x74



SecCount   equ 0x75
T_AmpH     equ 0x77
Amp_Sec    equ 0x78

Err_Symbol equ 0x7c
Charge_Triger equ 0x7d

; bank1

ListSz     equ 0xd9
MonList    equ 0xda   ; 32 bytes len
nList      equ 0xfb
dataLen    equ 0xfc
t4        equ  0xfd
nData     equ 0xfe
Sum       equ 0xff
	END
