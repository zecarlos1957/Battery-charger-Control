



;***** COMPILATION MESSAGES & WARNINGS *****

	ERRORLEVEL -207 	; Found label after column 1.
	ERRORLEVEL -302 	; Register in operand not in bank 0.



	#include "p16f876a.inc"


 
   global FXM1616U
   global b2bcd
   global bcd2a
   
   



;-------------------------------------------------


; STATUS bit definitions
  #define _C STATUS,0
  #define _DC STATUS,1
  #define _Z STATUS,2
  #define _OV STATUS,3

 

; binary operation arguments

;AARGB7 equ 0xa0
;AARGB6 equ 0xa1
AARGB5 equ 0xa2
AARGB4 equ 0xa3
AARGB3 equ 0xa4
AARGB2 equ 0xa5
AARGB1 equ 0xa6
AARGB0 equ 0xa7
AARG equ 0xa7 ; most significant byte of argument A
AEXP equ 0xa8 ; 8 bit biased exponent for argument A
EXP equ 0xa8 ; 8 bit biased exponent
;
BARGB3 equ 0xa9
BARGB2 equ 0xaa
BARGB1 equ 0xab
BARGB0 equ 0xac
BARG equ 0xac ; most significant byte of argument B
BEXP equ 0xad ; 8 bit biased exponent for argument B

SIGN    equ 0xae
MFlags  equ 0xaf


TEMPB3 equ 0xb0
TEMPB2 equ 0xb1
TEMPB1 equ 0xb2
TEMPB0 equ 0xb3
TEMP   equ 0xb3    ; temporary storage


SIGNED equ 0 


MSB equ 7
LSB equ 0

;PRECISION equ 0xbc
LOOPCOUNT equ 0xbd

;-----------------------------------------------



   #define bin   0xc0   ; separate exp:fraction ex: 5.735 = 5 735 
   #define bcd   0xc4
   #define pti    0xce
   #define pto    0xcf
   #define ii     0xd0
   #define temp   0xd1
   #define cnt    0xd2

   #define U_in 0x60
   #define U_out 0x62
   #define Requer 0x4e



    code


;******************************************************


UMUL1616L macro
; Max Timing: 2+13+6*15+14+2+7*16+15 = 248 clks
; Min Timing: 2+7*6+5+1+7*6+5+4 = 101 clks
; PM: 51 DM: 9
     MOVLW 0x08
     MOVWF LOOPCOUNT
LOOPUM1616A
     RRF BARGB1, F
     BTFSC _C
     GOTO ALUM1616NAP
     DECFSZ LOOPCOUNT, F
     GOTO LOOPUM1616A
     MOVWF LOOPCOUNT
LOOPUM1616B
     RRF BARGB0, F
     BTFSC _C
     GOTO BLUM1616NAP
     DECFSZ LOOPCOUNT, F
     GOTO LOOPUM1616B
     CLRF AARGB0
     CLRF AARGB1
     RETLW 0x00
BLUM1616NAP
     BCF _C
     GOTO BLUM1616NA
ALUM1616NAP
     BCF _C
     GOTO ALUM1616NA
ALOOPUM1616
     RRF BARGB1, F
     BTFSS _C
     GOTO ALUM1616NA
     MOVF TEMPB1,W
     ADDWF AARGB1, F
     MOVF TEMPB0,W
     BTFSC _C
     INCFSZ TEMPB0,W
     ADDWF AARGB0, F
ALUM1616NA
     RRF AARGB0, F
     RRF AARGB1, F
     RRF AARGB2, F
     DECFSZ LOOPCOUNT, F
     GOTO ALOOPUM1616
     MOVLW 0x08
     MOVWF LOOPCOUNT
BLOOPUM1616
     RRF BARGB0, F
     BTFSS _C
     GOTO BLUM1616NA
     MOVF TEMPB1,W
     ADDWF AARGB1, F
     MOVF TEMPB0,W
     BTFSC _C
     INCFSZ TEMPB0,W
     ADDWF AARGB0, F
BLUM1616NA
     RRF AARGB0, F
     RRF AARGB1, F
     RRF AARGB2, F
     RRF AARGB3, F
     DECFSZ LOOPCOUNT, F
     GOTO BLOOPUM1616
endm





;**********************************************************************************************
;**********************************************************************************************
; 16x16 Bit Unsigned Fixed Point Multiply 16x16 -> 32
; Input: 16 bit unsigned fixed point multiplicand in AARGB0
; 16 bit unsigned fixed point multiplier in BARGB0
; Use: CALL FXM1616U
; Output: 32 bit unsigned fixed point product in AARGB0
; Result: AARG <-- AARG x BARG
; Max Timing: 6+248+2 = 256 clks
; Min Timing: 6+101 = 107 clks
; PM: 6+51+1 = 58 DM: 9
FXM1616U
     CLRF AARGB2 ; clear partial product
     CLRF AARGB3
     MOVF AARGB0,W
     MOVWF TEMPB0
     MOVF AARGB1,W
     MOVWF TEMPB1
     UMUL1616L
     RETLW 0x00





;***********************************************************************





   ;Inputs:
    ;   Dividend - AARGB0:AARGB1:AARGB2 (0 - most significant!)
    ;   Divisor  - BARGB0:BARGB1
    ;Temporary:
    ;   Counter  - LOOPCOUNT
    ;   Remainder- REMB0:REMB1
    ;Output:
    ;   Quotient - AARGB0:AARGB1:AARGB2
    ;
    ;       Size: 28
    ; Max timing: 4+24*(6+6+4+3+6)-1+3+2=608 cycles (with return)
    ; Min timing: 4+24*(6+6+5+6)-1+3+2=560 cycles (with return)
    ;          

    FXD2416U:
            CLRF TEMPB0
            CLRF TEMPB1
            MOVLW .24
            MOVWF LOOPCOUNT
    LOOPU2416
            RLF AARGB2, F           ;shift left divider to pass next bit to remainder
            RLF AARGB1, F           ;and shift in next bit of result
            RLF AARGB0, F

            RLF TEMPB1, F            ;shift carry into remainder
            RLF TEMPB0, F

            RLF LOOPCOUNT, F        ;save carry in counter
             
            MOVF BARGB1, W          ;substract divisor from remainder
            SUBWF TEMPB1, F
            MOVF BARGB0, W
            BTFSS _C
            INCFSZ BARGB0, W
            SUBWF TEMPB0, W          ;keep that byte in W untill we make sure about borrow

            SKPNC                   ;if no borrow
             BSF LOOPCOUNT, 0       ;set bit 0 of counter (saved carry)

            BTFSC LOOPCOUNT, 0      ;if no borrow
             GOTO UOK46LL           ;jump

            MOVF BARGB1, W          ;restore remainder if borrow
            ADDWF TEMPB1, F
            MOVF TEMPB0, W           ;read high byte of remainder to W
                                    ;to not change it by next instruction
    UOK46LL
            MOVWF TEMPB0             ;store high byte of remainder
            CLRC                    ;copy bit 0 to carry
            RRF LOOPCOUNT, F        ;and restore counter
            DECFSZ LOOPCOUNT, f     ;decrement counter
             GOTO LOOPU2416         ;and repeat loop if not zero
             
            RLF AARGB2, F           ;shift in last bit of result
            RLF AARGB1, F   
            RLF AARGB0, F
  
            RETURN



;******************************************************************
; Convert the 10 binary coded digits (5 bytes) starting at 
; <bcd> into an ascii string also starting at <bcd>. Original
; bcd digits are lost.

bcd2a	movlw	bcd+9
	movwf	pto		; destination pointer
	movlw	bcd+4
 	movwf	pti		; source pointer
	movlw	5		; 5 bytes to process
	movwf	cnt

bcd2a1	movf	pti,w		; get current input pointer
	movwf	FSR
	decf	pti,f		; prepare for next
	movf	INDF,w		; get 2 bcds
	movwf	temp		; save for later
	movf	pto,w		; get current output pointer
	movwf	FSR
	decf	pto,f		; prepare for next
	decf	pto,f
	movf	temp,w		; get digits back
	andlw	0x0f		; process lsd
	addlw	"0"
	movwf	INDF		; to output
	decf	FSR,f
	swapf	temp,w		; process msd
	andlw	0x0f
	addlw	"0"
	movwf	INDF		; to output
	decfsz	cnt		; all digits?
	goto	bcd2a1	
    return			; yes





 
 
;****************************************
;******************************************************************
; Convert 32-bit binary number at <bin> into a bcd number
; at <bcd>. Uses Mike Keitz's procedure for handling bcd 
; adjust; Modified Microchip AN526 for 32-bits.

b2bcd	movlw	.32		; 32-bits
    banksel ii
	movwf	ii		; make cycle counter
	clrf	bcd		; clear result area
	clrf	bcd+1
	clrf	bcd+2
	clrf	bcd+3
	clrf	bcd+4
	
b2bcd2	movlw	bcd		; make pointer
	movwf	FSR
	movlw	5
	movwf	cnt

; Mike's routine:

b2bcd3	movlw	0x33		
	addwf	INDF,f		; add to both nybbles
	btfsc	INDF,3		; test if low result > 7
	andlw	0xf0		; low result >7 so take the 3 out
	btfsc	INDF,7		; test if high result > 7
	andlw	0x0f		; high result > 7 so ok
	subwf	INDF,f		; any results <= 7, subtract back
	incf	FSR,f		; point to next
	decfsz	cnt
	goto	b2bcd3
	
	rlf	bin+3,f		; get another bit
	rlf	bin+2,f
	rlf	bin+1,f
	rlf	bin+0,f
	rlf	bcd+4,f		; put it into bcd
	rlf	bcd+3,f
	rlf	bcd+2,f
	rlf	bcd+1,f
	rlf	bcd+0,f
	decfsz	ii,f		; all done?
	goto	b2bcd2		; no, loop
  	return			; yes

    END
