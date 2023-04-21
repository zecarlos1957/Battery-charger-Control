; adition 16+16




;***** COMPILATION MESSAGES & WARNINGS *****

	ERRORLEVEL -207 	; Found label after column 1.
	ERRORLEVEL -302 	; Register in operand not in bank 0.



	#include "p16f876a.inc"



; STATUS bit definitions
  #define _C STATUS,0
  #define _DC STATUS,1
  #define _Z STATUS,2
  #define _OV STATUS,3

 
   

    global D_add   ;16b+16b=32b
    global D_sub
    global D_divS
    global D_mul
     
    global Sqrt
    global Pow 
 
    global FXM1616U
    global FXD2416U
    global FXM2416U

    global FXD3216U
    global FXM2424U
    global FXADD32
    global FXSUB24

    global ADtoVolts
    global b2bcd
    global bcd2a
    global Update_PWM
 
 
    #define U_in 0x60
    #define U_out 0x62
    #define Require 0x4e

    #define REMB0 AARGB4
    #define REMB1 AARGB5
    #define REMB2 AARGB6
    #define REMB3 AARGB7

   
 
  
    


 

  CODE

       
init
   movlw LupCnt
   movwf LOOPCOUNT
   movf NumHi,W
   movwf SqrtHi
   movf NumLo,W ; set initial guess root = NUM/2
   movwf SqrtLo
   bcf STATUS,C
   rrf SqrtHi, F
   rrf SqrtLo, F
   retlw 0
;
div2 bcf STATUS,C
   rrf ACCbHI,W
   movwf SqrtHi
   rrf ACCbLO,W
   movwf SqrtLo
   retlw 0   



setup movlw .16 ; for 16 shifts
   banksel TEMP
   movwf TEMP
   movf ACCbHI,W ; move ACCb to ACCd
   movwf ACCdHI
   movf ACCbLO,W
   movwf ACCdLO
   clrf ACCbHI
   clrf ACCbLO
   retlw 0

;**********************************************************


;*******************************************************************
D_sub
   comf ACCaLO, F ; negate ACCa ( -ACCa -> ACCa )
   incf ACCaLO, F
   btfsc STATUS,Z
   decf ACCaHI, F
   comf ACCaHI, F 
D_add 
   movf ACCaLO,W ; Addition ( ACCb + ACCa -> ACCb )
   addwf ACCbLO, F ;add lsb
   btfsc STATUS,C ;add in carry
   incf ACCbHI, F
   movf ACCaHI,W
   addwf ACCbHI, F ;add msb
   retlw 0



;*******************************************************************
; Assemble this section only if Signed Arithmetic Needed
;
  IF SIGNED
;
S_SIGN movf ACCaHI,W
   xorwf ACCbHI,W
   movwf sign
   btfss ACCbHI,MSB ; if MSB set go & negate ACCb
   goto chek_A
;
   comf ACCbLO ; negate ACCb
   incf ACCbLO
   btfsc STATUS,Z
   decf ACCbHI
   comf ACCbHI
;
chek_A btfss ACCaHI,MSB ; if MSB set go & negate ACCa
   retlw 0
   goto neg_A
;
ENDIF
;
;
Sqrt call init
sloop movf NumLo,W
   movwf ACCbLO
   movf NumHi,W
   movwf ACCbHI
;
   call D_divS ; double precision division
   call D_add ; double precision addition
; ; the above 2 routines are listed
; ; as seperate routines
   call div2
   decfsz LOOPCOUNT, F
   goto sloop
  retlw 0 ; all iterations done
; ; branch back to desired location
;





; ACCa * ACCb = ACCb:ACCc  

D_mul ;results in ACCb(16 msb’s) and ACCc(16 lsb)

   call setup
mloop rrf ACCdHI, F ;rotate d right
   rrf ACCdLO, F
   btfsc STATUS,C ;need to add?
   call D_add
   rrf ACCbHI, F
   rrf ACCbLO, F
   rrf ACCcHI, F
   rrf ACCcLO, F
   decfsz TEMP, F ;loop until all bits checked
   goto mloop
   IF SIGNED
      btfss SIGN,MSB
      retlw 0
      comf ACCcLO, F ; negate ACCa ( -ACCa -> ACCa )
      incf ACCcLO, F
      btfsc STATUS,Z
      decf ACCcHI, F
      comf ACCcHI, F
      btfsc STATUS,Z
neg_B comf ACCbLO, F ; negate ACCb
      incf ACCbLO, F
      btfsc STATUS,Z
      decf ACCbHI, F
      comf ACCbHI, F
      retlw 0
   ELSE
      retlw 0
   ENDIF 
    


; ; for Multiplication & Division needs
; ; to be assembled as Signed Integer
; ; Routines. If ‘FALSE’ the above two
; ; routines ( D_mpy & D_div ) use
; ; unsigned arithmetic.
;*******************************************************************
; Double Precision Divide ( 16/16 -> 16 )
;
; (ACCb/ACCa -> ACCb with remainder in ACCc) : 16 bit output
; with Quotiont in ACCb (ACCbHI,ACCbLO) and Remainder in ACCc
; (ACCcHI,ACCcLO).
; NOTE: Before calling this routine, the user should make sure that
; the Numerator(ACCb) is greater than Denominator(ACCa). If
; the case is not true, the user should scale either Numerator
; or Denominator or both such that Numerator is greater than
; the Denominator.
;
;
D_divS
;
   IF SIGNED
      CALL S_SIGN
   ENDIF
      call setup
      clrf ACCcHI
      clrf ACCcLO
dloop bcf STATUS,C
      rlf ACCdLO, F
      rlf ACCdHI, F
      rlf ACCcLO, F
      rlf ACCcHI, F
      movf ACCaHI,W
      subwf ACCcHI,W ; check if a>c
      btfss STATUS,Z
      goto nochk
      movf ACCaLO,W
      subwf ACCcLO,W ; if msb equal then check lsb
nochk btfss STATUS,C ; carry set if c>a
      goto nogo
      movf ACCaLO,W ; c-a into c
      subwf ACCcLO, F
      btfss STATUS,C
      decf ACCcHI, F
      movf ACCaHI,W
      subwf ACCcHI, F
      bsf STATUS,C ; shift a 1 into b (result)
nogo rlf ACCbLO, F
      rlf ACCbHI, F
      decfsz TEMP, F ; loop untill all bits checked
      goto dloop
   IF SIGNED
      btfss SIGN,MSB ; check sign if negative
      retlw 0
      goto neg_B ; negate ACCa ( -ACCa -> ACCa )
   ELSE
      retlw 0
   ENDIF
  
  
;****************************************************************


 
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




;*************************************************************
;*************************************************************
;*************************************************************
;*************************************************************
;*************************************************************
;*************************************************************
;*************************************************************
;*************************************************************
;*************************************************************
;*************************************************************


; call Pow(x,y)
; 
; input x AARGB0:AARGB1;AARGB2
;       y BARGB0
;
; output AARGB0:AARGB1:AARGB2;AARGB3:AARGB4:AARGB5
;
;


Pow:
   movfw BARGB0
   movwf TEMPB3
   decf TEMPB3
WPloop
   movfw AARGB2 
   movwf BARGB2
   movfw AARGB1 
   movwf BARGB1
   movfw AARGB0 
   movwf BARGB0
   call FXM2424U   
   decfsz TEMPB3
   goto WPloop
   retlw 0





FXSUB24
 
   movf BARGB2,W  ;ACCaLO,W ; Subtraction ( ACCb - ACCa -> ACCb )
   subwf AARGB2,F  ;ACCbLO, F ;sub lsb
   btfss STATUS,C ;add in carry
   decf AARGB1,F    ;ACCbHI, F
   movf BARGB1,W
   subwf AARGB1,F
   btfss STATUS,C
   decf AARGB0,F
   movf BARGB0,W  ;Subtraction ( ACCb - ACCa -> ACCb )
   subwf AARGB0, F ;sub lsb
  
  
   retlw 0

 
FXADD32
   movf BARGB3,W
   addwf AARGB3,F
   btfsc STATUS,C
   incf AARGB2,F
   movf BARGB2,W   
   addwf AARGB2,F   
   btfsc STATUS,C  
   incf AARGB1,F    
   movf BARGB1,W
   addwf AARGB1,F
   btfsc STATUS,C
   incf AARGB0,F
   movf BARGB0,W  ;Addition ( AARG + BARG -> AARG )
   addwf AARGB0, F ;sub lsb
  
   retlw 0



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


; RCS Header $Id: fxm46.a16 2.3 1996/10/16 14:23:23 F.J.Testa Exp $
; $Revision: 2.3 $
; 24x16 PIC16 FIXED POINT MULTIPLY ROUTINES
;
; Input: fixed point arguments in AARG and BARG
;
; Output: product AARGxBARG in AARG
;
; All timings are worst case cycle counts
;
; It is useful to note that the additional unsigned routines requiring a non-power of two
; argument can be called in a signed multiply application where it is known that the
; respective argument is nonnegative, thereby offering some improvement in
; performance.
;
; Routine Clocks Function
;
; FXM2416S 346 24x16 -> 40 bit signed fixed point multiply
;
; FXM2416U 334 24x16 -> 40 bit unsigned fixed point multiply
;
; FXM2315U 319 23x15 -> 38 bit unsigned fixed point multiply
;
; The above timings are based on the looped macros. If space permits,
; approximately 36-62 clocks can be saved by using the unrolled macros.
;
;**********************************************************************************************
;**********************************************************************************************
; 24x16 Bit Multiplication Macros
;SMUL2416L macro
;; Max Timing: 2+12+6*21+20+2+6*22+21+6 = 321 clks
;; Min Timing: 2+7*6+5+2+6*6+5+5 = 97 clks
;; PM: 19+20+2+21+6 = 68 DM: 12
;   MOVLW 0x80
;   MOVWF LOOPCOUNT
;LOOPSM2416A
;   RRF BARGB1, F
;   BTFSC _C
;   GOTO ALSM2416NA
;   DECFSZ LOOPCOUNT, F
;   GOTO LOOPSM2416A
;   MOVLW 0x7
;   MOVWF LOOPCOUNT
;LOOPSM2416B
;   RRF BARGB0, F
;   BTFSC _C
;   GOTO BLSM2416NA
;   DECFSZ LOOPCOUNT, F
;   GOTO LOOPSM2416B
;   CLRF AARGB0
;   CLRF AARGB1
;   CLRF AARGB2
;   RETLW 0x00
;ALOOPSM2416
;   RRF BARGB1, F
;   BTFSS _C
;   GOTO ALSM2416NA
;   MOVF TEMPB2,W
;   ADDWF AARGB2, F
;   MOVF TEMPB1,W
;   BTFSC _C
;   INCFSZ TEMPB1,W
;   ADDWF AARGB1, F
;   MOVF TEMPB0,W
;   BTFSC _C
;   INCFSZ TEMPB0,W
;   ADDWF AARGB0, F
;ALSM2416NA RLF SIGN,W
;   RRF AARGB0, F
;   RRF AARGB1, F
;   RRF AARGB2, F
;   RRF AARGB3, F
;   DECFSZ LOOPCOUNT, F
;   GOTO ALOOPSM2416
;   MOVLW 0x7
;   MOVWF LOOPCOUNT
;BLOOPSM2416
;   RRF BARGB0, F
;   BTFSS _C
;   GOTO BLSM2416NA
;   MOVF TEMPB2,W
;   ADDWF AARGB2, F
;   MOVF TEMPB1,W
;   BTFSC _C
;   INCFSZ TEMPB1,W
;   ADDWF AARGB1, F
;   MOVF TEMPB0,W
;   BTFSC _C
;   INCFSZ TEMPB0,W
;   ADDWF AARGB0, F
;BLSM2416NA RLF SIGN,W
;   RRF AARGB0, F
;   RRF AARGB1, F
;   RRF AARGB2, F
;   RRF AARGB3, F
;   RRF AARGB4, F
;   DECFSZ LOOPCOUNT, F
;   GOTO BLOOPSM2416
;   RLF SIGN,W
;   RRF AARGB0, F
;   RRF AARGB1, F
;   RRF AARGB2, F
;   RRF AARGB3, F
;   RRF AARGB4, F
;   endm  
;


UMUL2416L macro
; Max Timing: 2+14+6*20+19+2+7*21+20 = 324 clks
; Min Timing: 2+7*6+5+1+7*6+5+5 = 102 clks
; PM: 18+20+2+21 = 61 DM: 12
   MOVLW 0x08
   MOVWF LOOPCOUNT
LOOPUM2416A
   RRF BARGB1, F
   BTFSC _C
   GOTO ALUM2416NAP
   DECFSZ LOOPCOUNT, F
   GOTO LOOPUM2416A
   MOVWF LOOPCOUNT
LOOPUM2416B
   RRF BARGB0, F
   BTFSC _C
   GOTO BLUM2416NAP
   DECFSZ LOOPCOUNT, F
   GOTO LOOPUM2416B
   CLRF AARGB0
   CLRF AARGB1
   CLRF AARGB2
   RETLW 0x00
BLUM2416NAP
   BCF _C
   GOTO BLUM2416NA
ALUM2416NAP
   BCF _C
   GOTO ALUM2416NA
ALOOPUM2416
   RRF BARGB1, F
   BTFSS _C
   GOTO ALUM2416NA
   MOVF TEMPB2,W
   ADDWF AARGB2, F
   MOVF TEMPB1,W
   BTFSC _C
   INCFSZ TEMPB1,W
   ADDWF AARGB1, F
   MOVF TEMPB0,W
   BTFSC _C
   INCFSZ TEMPB0,W
   ADDWF AARGB0, F
ALUM2416NA
   RRF AARGB0, F
   RRF AARGB1, F
   RRF AARGB2, F
   RRF AARGB3, F
   DECFSZ LOOPCOUNT, F
   GOTO ALOOPUM2416
   MOVLW 0x08
   MOVWF LOOPCOUNT
BLOOPUM2416
   RRF BARGB0, F
   BTFSS _C
   GOTO BLUM2416NA
   MOVF TEMPB2,W
   ADDWF AARGB2, F
   MOVF TEMPB1,W
   BTFSC _C
   INCFSZ TEMPB1,W
   ADDWF AARGB1, F
   MOVF TEMPB0,W
   BTFSC _C
   INCFSZ TEMPB0,W
   ADDWF AARGB0, F
BLUM2416NA
   RRF AARGB0, F
   RRF AARGB1, F
   RRF AARGB2, F
   RRF AARGB3, F
   RRF AARGB4, F
   DECFSZ LOOPCOUNT, F
   GOTO BLOOPUM2416
   endm



;**********************************************************************************************
;**********************************************************************************************
; 24x16 Bit Unsigned Fixed Point Multiply 24x16 -> 40
; Input: 24 bit unsigned fixed point multiplicand in AARGB0
; 16 bit unsigned fixed point multiplier in BARGB0
; Use: CALL FXM2416U
; Output: 40 bit unsigned fixed point product in AARGB0
; Result: AARG <-- AARG x BARG
; Max Timing: 8+324+2 = 334 clks
; Min Timing: 8+102 = 110 clks
; PM: 8+61+1 = 70 DM: 12
FXM2416U
   CLRF AARGB3 ; clear partial product
   CLRF AARGB4
   MOVF AARGB0,W
   MOVWF TEMPB0
   MOVF AARGB1,W
   MOVWF TEMPB1
   MOVF AARGB2,W
   MOVWF TEMPB2
   UMUL2416L
   RETLW 0x00






;*********************************************************************************
;*********************************************************************************
;*********************************************************************************
;*********************************************************************************
;*********************************************************************************






UMUL2424L macro
; Max Timing: 2+14+6*20+19+2+7*21+20+2+7*22+21 = 501 clks
; Min Timing: 2+7*6+5+1+7*6+5+1+7*6+5+5 = 150 clks
; PM: 23+20+2+21+2+22 = 88 DM: 13
   MOVLW 0x08
   MOVWF LOOPCOUNT
LOOPUM2424A
   RRF BARGB2, F
   BTFSC _C
   GOTO ALUM2424NAP
   DECFSZ LOOPCOUNT, F
   GOTO LOOPUM2424A
   MOVWF LOOPCOUNT
LOOPUM2424B
   RRF BARGB1, F
   BTFSC _C
   GOTO BLUM2424NAP
   DECFSZ LOOPCOUNT, F
   GOTO LOOPUM2424B
   MOVWF LOOPCOUNT
LOOPUM2424C
   RRF BARGB0, F
   BTFSC _C
   GOTO CLUM2424NAP
   DECFSZ LOOPCOUNT, F
   GOTO LOOPUM2424C
   CLRF AARGB0
   CLRF AARGB1
   CLRF AARGB2
   RETLW 0x00
CLUM2424NAP
   BCF _C
   GOTO CLUM2424NA
BLUM2424NAP
   BCF _C
   GOTO BLUM2424NA
ALUM2424NAP
   BCF _C
   GOTO ALUM2424NA
ALOOPUM2424
   RRF BARGB2, F
   BTFSS _C
   GOTO ALUM2424NA
   MOVF TEMPB2,W
   ADDWF AARGB2, F
   MOVF TEMPB1,W
   BTFSC _C
   INCFSZ TEMPB1,W
   ADDWF AARGB1, F
   MOVF TEMPB0,W
   BTFSC _C
   INCFSZ TEMPB0,W
   ADDWF AARGB0, F
ALUM2424NA
   RRF AARGB0, F
   RRF AARGB1, F
   RRF AARGB2, F
   RRF AARGB3, F
   DECFSZ LOOPCOUNT, F
   GOTO ALOOPUM2424
   MOVLW 0x08
   MOVWF LOOPCOUNT
BLOOPUM2424
   RRF BARGB1, F
   BTFSS _C
   GOTO BLUM2424NA
   MOVF TEMPB2,W
   ADDWF AARGB2, F
   MOVF TEMPB1,W
   BTFSC _C
   INCFSZ TEMPB1,W
   ADDWF AARGB1, F
   MOVF TEMPB0,W
   BTFSC _C
   INCFSZ TEMPB0,W
   ADDWF AARGB0, F
BLUM2424NA
   RRF AARGB0, F
   RRF AARGB1, F
   RRF AARGB2, F
   RRF AARGB3, F
   RRF AARGB4, F
   DECFSZ LOOPCOUNT, F
   GOTO BLOOPUM2424
   MOVLW 0x08
   MOVWF LOOPCOUNT
CLOOPUM2424
   RRF BARGB0, F
   BTFSS _C
   GOTO CLUM2424NA
   MOVF TEMPB2,W
   ADDWF AARGB2, F
   MOVF TEMPB1,W
   BTFSC _C
   INCFSZ TEMPB1,W
   ADDWF AARGB1, F
   MOVF TEMPB0,W
   BTFSC _C
   INCFSZ TEMPB0,W
   ADDWF AARGB0, F
CLUM2424NA
   RRF AARGB0, F
   RRF AARGB1, F
   RRF AARGB2, F
   RRF AARGB3, F
   RRF AARGB4, F
   RRF AARGB5, F
   DECFSZ LOOPCOUNT, F
   GOTO CLOOPUM2424
   endm


;**********************************************************************************************
; 24x24 Bit Unsigned Fixed Point Multiply 24x24 -> 48
; Input: 24 bit unsigned fixed point multiplicand in AARGB0
; 24 bit unsigned fixed point multiplier in BARGB0
; Use: CALL FXM2424U
; Output: 48 bit unsigned fixed point product in AARGB0
; Result: AARG <-- AARG x BARG
; Max Timing: 9+501+2 = 512 clks
; Min Timing: 9+150 = 159 clks
; PM: 9+88+1 = 98 DM: 13
FXM2424U
   CLRF AARGB3 ; clear partial product
   CLRF AARGB4
   CLRF AARGB5
   MOVF AARGB0,W
   MOVWF TEMPB0
   MOVF AARGB1,W
   MOVWF TEMPB1
   MOVF AARGB2,W
   MOVWF TEMPB2
   UMUL2424L
   RETLW 0x00
;**********************************************************************************************





 ;**********************************************************************************************
;**********************************************************************************************
; 32x32 Bit Signed Fixed Point Multiply 32x32 -> 64
; Input: 32 bit signed fixed point multiplicand in AARGB0
; 32 bit signed fixed point multiplier in BARGB0
; Use: CALL FXM3232S
; Output: 64 bit signed fixed point product in AARGB0
; Result: AARG <-- AARG x BARG
; Max Timing: 15+851+2 = 868 clks B > 0
; 36+851+2 = 889 clks B < 0
; Min Timing: 15+192 = 207 clks
; PM: 36+152+1 = 189 DM: 17
;FXM3232S CLRF AARGB4 ; clear partial product
;   CLRF AARGB5
;   CLRF AARGB6
;   CLRF AARGB7
;   CLRF SIGN
;   MOVF AARGB0,W
;   IORWF AARGB1,W
;   IORWF AARGB2,W
;   IORWF AARGB3,W
;   BTFSC _Z
;   RETLW 0x00
;   MOVF AARGB0,W
;   XORWF BARGB0,W
;   MOVWF TEMPB0
;   BTFSC TEMPB0,MSB
;   COMF SIGN,F
;   BTFSS BARGB0,MSB
;   GOTO M3232SOK
;   COMF BARGB3, F
;   COMF BARGB2, F
;   COMF BARGB1, F
;   COMF BARGB0, F
;   INCF BARGB3, F
;   BTFSC _Z
;   INCF BARGB2, F
;   BTFSC _Z
;   INCF BARGB1, F
;   BTFSC _Z
;   INCF BARGB0, F
;   COMF AARGB3, F
;   COMF AARGB2, F
;   COMF AARGB1, F
;   COMF AARGB0, F
;   INCF AARGB3, F
;   BTFSC _Z
;   INCF AARGB2, F
;   BTFSC _Z
;   INCF AARGB1, F
;   BTFSC _Z
;   INCF AARGB0, F
;   BTFSC BARGB0,MSB
;   GOTO M3232SX
;M3232SOK MOVF AARGB0,W
;   MOVWF TEMPB0
;   MOVF AARGB1,W
;   MOVWF TEMPB1
;   MOVF AARGB2,W
;   MOVWF TEMPB2
;   MOVF AARGB3,W
;   MOVWF TEMPB3
;SMUL3232L
;   RETLW 0x00
;M3232SX CLRF AARGB4
;   CLRF AARGB5
;   CLRF AARGB6
;   CLRF AARGB7
;   RLF SIGN,W
;   RRF AARGB0,F
;   RRF AARGB1,F
;   RRF AARGB2,F
;   RRF AARGB3,F
;   RETLW 0x00
;   
;   
;   
;;*************************************************************************************************************
   
;SDIV3216L macro
UDIV3216L macro
; Max Timing: 16+6*22+21+21+6*22+21+21+6*22+21+21+6*22+21+8 = 699 clks
; Min Timing: 16+6*21+20+20+6*21+20+20+6*21+20+20+6*21+20+3 = 663 clks
; PM: 240 DM: 9
   CLRF TEMP
   RLF AARGB0,W
   RLF REMB1, F
   MOVF BARGB1,W
   SUBWF REMB1, F
   MOVF BARGB0,W
   BTFSS _C
   INCFSZ BARGB0,W
   SUBWF REMB0, F
   CLRW
   BTFSS _C
   MOVLW 1
   SUBWF TEMP, F
   RLF AARGB0, F
   MOVLW 7
   MOVWF LOOPCOUNT
LOOPU3216A RLF AARGB0,W
   RLF REMB1, F
   RLF REMB0, F
   RLF TEMP, F
   MOVF BARGB1,W
   BTFSS AARGB0,LSB
   GOTO UADD26LA
   SUBWF REMB1, F
   MOVF BARGB0,W
   BTFSS _C
   INCFSZ BARGB0,W
   SUBWF REMB0, F
   CLRW
   BTFSS _C
   MOVLW 1
   SUBWF TEMP, F
   GOTO UOK26LA
UADD26LA ADDWF REMB1, F
   MOVF BARGB0,W
   BTFSC _C
   INCFSZ BARGB0,W
   ADDWF REMB0, F
   CLRW
   BTFSC _C
   MOVLW 1
   ADDWF TEMP, F
UOK26LA RLF AARGB0, F
   DECFSZ LOOPCOUNT, F
   GOTO LOOPU3216A
   RLF AARGB1,W
   RLF REMB1, F
   RLF REMB0, F
   RLF TEMP, F
   MOVF BARGB1,W
   BTFSS AARGB0,LSB
   GOTO UADD26L8
   SUBWF REMB1, F
   MOVF BARGB0,W
   BTFSS _C
   INCFSZ BARGB0,W
   SUBWF REMB0, F
   CLRW
   BTFSS _C
   MOVLW 1
   SUBWF TEMP, F
   GOTO UOK26L8
UADD26L8 ADDWF REMB1, F
   MOVF BARGB0,W
   BTFSC _C
   INCFSZ BARGB0,W
   ADDWF REMB0, F
   CLRW
   BTFSC _C
   MOVLW 1
   ADDWF TEMP, F
UOK26L8 RLF AARGB1, F
   MOVLW 7
   MOVWF LOOPCOUNT
LOOPU3216B RLF AARGB1,W
   RLF REMB1, F
   RLF REMB0, F
   RLF TEMP, F
   MOVF BARGB1,W
   BTFSS AARGB1,LSB
   GOTO UADD26LB
   SUBWF REMB1, F
   MOVF BARGB0,W
   BTFSS _C
   INCFSZ BARGB0,W
   SUBWF REMB0, F
   CLRW
   BTFSS _C
   MOVLW 1
   SUBWF TEMP, F
   GOTO UOK26LB
UADD26LB ADDWF REMB1, F
   MOVF BARGB0,W
   BTFSC _C
   INCFSZ BARGB0,W
   ADDWF REMB0, F
   CLRW
   BTFSC _C
   MOVLW 1
   ADDWF TEMP, F
UOK26LB RLF AARGB1, F
   DECFSZ LOOPCOUNT, F
   GOTO LOOPU3216B
   RLF AARGB2,W
   RLF REMB1, F
   RLF REMB0, F
   RLF TEMP, F
   MOVF BARGB1,W
   BTFSS AARGB1,LSB
   GOTO UADD26L16
   SUBWF REMB1, F
   MOVF BARGB0,W
   BTFSS _C
   INCFSZ BARGB0,W
   SUBWF REMB0, F
   CLRW
   BTFSS _C
   MOVLW 1
   SUBWF TEMP, F
   GOTO UOK26L16
UADD26L16 ADDWF REMB1, F
   MOVF BARGB0,W
   BTFSC _C
   INCFSZ BARGB0,W
   ADDWF REMB0, F
   CLRW
   BTFSC _C
   MOVLW 1
   ADDWF TEMP, F
UOK26L16 RLF AARGB2, F
   MOVLW 7
   MOVWF LOOPCOUNT
LOOPU3216C RLF AARGB2,W
   RLF REMB1, F
   RLF REMB0, F
   RLF TEMP, F
   MOVF BARGB1,W
   BTFSS AARGB2,LSB
   GOTO UADD26LC
   SUBWF REMB1, F
   MOVF BARGB0,W
   BTFSS _C
   INCFSZ BARGB0,W
   SUBWF REMB0, F
   CLRW
   BTFSS _C
   MOVLW 1
   SUBWF TEMP, F
   GOTO UOK26LC
UADD26LC ADDWF REMB1, F
   MOVF BARGB0,W
   BTFSC _C
   INCFSZ BARGB0,W
   ADDWF REMB0, F
   CLRW
   BTFSC _C
   MOVLW 1
   ADDWF TEMP, F
UOK26LC RLF AARGB2, F
   DECFSZ LOOPCOUNT, F
   GOTO LOOPU3216C
   RLF AARGB3,W
   RLF REMB1, F
   RLF REMB0, F
   RLF TEMP, F
   MOVF BARGB1,W
   BTFSS AARGB2,LSB
   GOTO UADD26L24
   SUBWF REMB1, F
   MOVF BARGB0,W
   BTFSS _C
   INCFSZ BARGB0,W
   SUBWF REMB0, F
   CLRW
   BTFSS _C
   MOVLW 1
   SUBWF TEMP, F
   GOTO UOK26L24
UADD26L24 ADDWF REMB1, F
   MOVF BARGB0,W
   BTFSC _C
   INCFSZ BARGB0,W
   ADDWF REMB0, F
   CLRW
   BTFSC _C
   MOVLW 1
   ADDWF TEMP, F
UOK26L24 RLF AARGB3, F
   MOVLW 7
   MOVWF LOOPCOUNT
LOOPU3216D RLF AARGB3,W
   RLF REMB1, F
   RLF REMB0, F
   RLF TEMP, F
   MOVF BARGB1,W
   BTFSS AARGB3,LSB
   GOTO UADD26LD
   SUBWF REMB1, F
   MOVF BARGB0,W
   BTFSS _C
   INCFSZ BARGB0,W
   SUBWF REMB0, F
   CLRW
   BTFSS _C
   MOVLW 1
   SUBWF TEMP, F
   GOTO UOK26LD
UADD26LD ADDWF REMB1, F
   MOVF BARGB0,W
   BTFSC _C
   INCFSZ BARGB0,W
   ADDWF REMB0, F
   CLRW
   BTFSC _C
   MOVLW 1
   ADDWF TEMP, F
UOK26LD RLF AARGB3, F
   DECFSZ LOOPCOUNT, F
   GOTO LOOPU3216D
   BTFSC AARGB3,LSB
   GOTO UOK26L
   MOVF BARGB1,W
   ADDWF REMB1, F
   MOVF BARGB0,W
   BTFSC _C
   INCFSZ BARGB0,W
   ADDWF REMB0, F
UOK26L
    endm
 



;**********************************************************************************************
;**********************************************************************************************
; 32/16 Bit Unsigned Fixed Point Divide 32/16 -> 32.16
; Input: 32 bit unsigned fixed point dividend in AARGB0, AARGB1,AARGB2,AARGB3
; 16 bit unsigned fixed point divisor in BARGB0, BARGB1
; Use: CALL FXD3216U
; Output: 32 bit unsigned fixed point quotient in AARGB0, AARGB1,AARGB2,AARGB3
; 16 bit unsigned fixed point remainder in REMB0, REMB1
; Result: AARG, REM <-- AARG / BARG
; Max Timing: 2+699+2 = 703 clks
; Max Timing: 2+663+2 = 667 clks
; PM: 2+240+1 = 243 DM: 9
FXD3216U 
     CLRF REMB0
     CLRF REMB1
     UDIV3216L
     RETLW 0x00
 

;*************************************************
;******************************************************



ADtoVolts

    ; Translate   AD to voltage
   ; float result = (5 * AD / 1023) * 6.7353

    ; result = ADdata*5
   
    movlw .5
    movwf  ACCbLO
    clrf  ACCbHI
    clrf  ACCcLO
    clrf  ACCcHI
    clrf  ACCdLO
    clrf  ACCdHI
    pagesel D_mul
    call D_mul   ; ACCa * ACCb =  ACCb:ACCc  
                 ;                  H   L
    ; result *= 1000
    
  
    movf ACCcLO,W
    movwf ACCaLO  ;AARGB2
    movf ACCcHI,W
    movwf ACCaHI   ;AARGB1
    clrf ACCcHI
    clrf ACCdHI
    clrf ACCcLO
    clrf ACCdLO
    movlw 0xe8    ; 0x3e8 = 1000
    movwf ACCbLO   ;BARGB1
    movlw 0x03
    movwf ACCbHI   ;BARGB0
    call D_mul   ;FXM2416U
    
    ; result /= 1023

    movf ACCcLO,W
    movwf AARGB2
    movf ACCcHI,W
    movwf AARGB1
    movf ACCbLO,W
    movwf AARGB0 
    movlw 0xff
    movwf BARGB1
    movlw 0x03
    movwf BARGB0
    call FXD2416U  ; (AARGB0:1:2/BARGB0:1 -> AARGB0:1:2  rem in REMB0:REMB1)

 ; result *= 6735
    
    movlw 0x4f
    movwf BARGB1
    movlw 0x1a   ;0x1a4f 6735
    movwf BARGB0
    call FXM2416U  ; (AARGB0:1:2 * BARGB0:1 -> AARGB0:1:2:3:4   

    return

 

;;*************************************************************************************************************
   
;**************************************************

; input: Require set to value to add  to current U_out

Update_PWM

   ; Convert requered AD to volts 
  
;   movfw Require
;   banksel ACCaLO
;   movwf ACCaLO
;   banksel Require
;   movfw Require+1
;   banksel ACCaLO
;   movwf ACCaHI 

;   call ADtoVolts ; res AARGB0:1:2:3:4
 
   
   ;save result
;   movfw AARGB4
;   movwf BINdata
;   movfw AARGB3
;   movwf BINdata+1
;   movfw AARGB2
;   movwf BINdata+2
;   movfw AARGB1
;   movwf BINdata+3
 

 
; Add Uout to Require
;   banksel U_out
;   movfw U_out
;   addwf Require,F
;   btfsc STATUS,C
;   incf Require+1,F
;   movfw U_out+1
;   addwf Require+1,F


   
   ; Convert  Uout to volts 
   movfw U_out
   banksel ACCaLO
   movwf ACCaLO
   banksel U_out
   movfw U_out+1
   banksel ACCaLO
   movwf ACCaHI
 
   call ADtoVolts ; res AARGB0:1:2:3:4

   ; Add to Bindata
   movfw AARGB4
   addwf BINdata,F
   btfsc STATUS,C
   incf BINdata+1,F
   movfw AARGB3
   addwf BINdata+1,F
   btfsc STATUS,C
   incf BINdata+2
   movfw AARGB2
   addwf BINdata+2,F

 

   call Calc_PWM

  swapf AARGB2,W
   andlw 0x30
   banksel CCP1CON
   iorwf CCP1CON,F
   banksel AARGB0 
   rrf AARGB2
   rrf AARGB2
   movlw 0x3f
   andwf AARGB2,F
   swapf AARGB1,F
   rlf AARGB1
   rlf AARGB1,W
   andlw 0xf0
   iorwf AARGB2,W   
   banksel CCPR1L
   movwf CCPR1L

   return


;****************************************


 
Calc_PWM

   
  
   ; convert AD_Uin to volts
   banksel U_in
   movfw U_in
   banksel ACCaLO
   movwf ACCaLO
   banksel U_in
   movfw U_in+1
   banksel ACCaLO
   movwf ACCaHI
 

   call ADtoVolts ; res AARGB0:1:2:3:4
    

   ; U_in/=1000 for use RMS/Uin
    movfw AARGB1
    movwf AARGB0
    movfw AARGB2
    movwf AARGB1
    movfw AARGB3
    movwf AARGB2
    movfw AARGB4
    movwf AARGB3
   
 
    movlw 0xe8   
    movwf BARGB1
    movlw 0x03
    movwf BARGB0
    call FXD3216U  ;(AARGB0:1:2:3/BARGB0:1 -> AARGB0:1:2:3  rem in REMB0:REMB1)
   

    ; res = rms/ U_in
 
    movfw AARGB3
    movwf BARGB1
    movfw AARGB2
    movwf BARGB0
 

    movf BINdata,W
    movwf AARGB2
    movf BINdata+1,W
    movwf AARGB1
    movf BINdata+2,W
    movwf AARGB0
 
    call FXD2416U ;(AARGB0:1:2/BARGB0:1 -> AARGB0:1:2  rem in REMB0:REMB1)
 
    ; res=pow(res,2)

    movlw 2
    movwf BARGB0
    call Pow  ; output AARGB0:1:2:3:4:5
   
;    ; T1 = res*T  (XT 4Mhz) T=25

    movf AARGB3,W
    movwf AARGB0    
    movf AARGB4,W
    movwf AARGB1  
    movf AARGB5,W
    movwf AARGB2  
    movlw .25
    movwf BARGB1
    clrf BARGB0
    call FXM2416U  ; output AARGB 40 bits  

   
     ;duty =T/25

    movf AARGB1,W
    movwf AARGB0
    movf AARGB2,W
    movwf AARGB1
    movf AARGB3,W
    movwf AARGB2
    movf AARGB4,W
    movwf AARGB3
    movlw .25  ;0xe8
    movwf BARGB1
    movlw 0  ;   x03
    movwf BARGB0
    call FXD3216U     ;out - AARGB0:1:2
 ;   call FXD3216U

   ; /=10000   
  
    movlw 0x10
    movwf BARGB1  
     movlw 0x27
    movwf BARGB0  

    call FXD2416U   ; output  AARGB0:1:2

    ; PWM duty*1023

    movlw 0xff
    movwf BARGB1
    movlw 0x03
    movwf BARGB0
    call FXM2416U ;(AARGB0:1:2/BARGB0:1 
                ; -> 40 bits  rem in REMB0:REMB1)
     
    
    ; PWM/=100
 
    movf AARGB2,W
    movwf AARGB0
    movf AARGB3,W
    movwf AARGB1
    movf AARGB4,W
    movwf AARGB2
    movlw .100
    movwf BARGB1
    movlw 0x00
    movwf BARGB0
    call FXD2416U ;(AARGB0:1:2/BARGB0:1 -> AARGB0:1:2  rem in REMB0:REMB1)

    return


;*************************************************
;******************************************************

   
    ;   UDATA
   



 

; binary operation arguments

AARGB7 equ 0xa0
AARGB6 equ 0xa1
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

 

ACCaHI  equ 0xb4
ACCaLO  equ 0xb5
ACCbHI  equ 0xb6
ACCbLO  equ 0xb7
ACCcHI  equ 0xb8
ACCcLO  equ 0xb9
ACCdHI  equ 0xba
ACCdLO  equ 0xbb




TEMPB3 equ 0xb0
TEMPB2 equ 0xb1
TEMPB1 equ 0xb2
TEMPB0 equ 0xb3
TEMP   equ 0xb3    ; temporary storage

;
;
; Note that AARG and ACC reference the same storage location
;*********************************************************************************************
;
; FLOATING POINT SPECIFIC DEFINITIONS
;
; literal constants
;
EXPBIAS equ D'127'
;
;




; floating point library exception flags
;
FPFLAGS equ 0xaf ; floating point library exception flags
IOV equ 0 ; bit0 = integer overflow flag
FOV equ 1 ; bit1 = floating point overflow flag
FUN equ 2 ; bit2 = floating point underflow flag
FDZ equ 3 ; bit3 = floating point divide by zero flag
NAN equ 4 ; bit4 = not-a-number exception flag
DOM equ 5 ; bit5 = domain error exception flag
RND equ 6 ; bit6 = floating point rounding flag, 0 = truncation
; 1 = unbiased rounding to nearest LSb
SAT equ 7 ; bit7 = floating point saturate flag, 0 = terminate on
; exception without saturation, 1 = terminate on
; exception with saturation to appropriate value
;**********************************************************************************************



SIGNED equ 0 


MSB equ 7
LSB equ 0

PRECISION equ 0xbc
LOOPCOUNT equ 0xbd

SqrtLo equ ACCaLO
SqrtHi equ ACCaHI
;

       ; Store AD convertion to bin
bin  equ 0xc0   ; separate exp:fraction ex: 5.735 = 5 735 
bcd  equ 0xc4
pti  equ 0xce
pto  equ 0xcf
ii   equ 0xd0
temp equ 0xd1
cnt  equ 0xd2


Vinteger  equ 0xc6 ;(no decimal point ex: 5.735 = 5735) 
BINdata   equ 0xca ; same as Vinteger to PWM denominator usage
 

NumLo equ 0xce
NumHi equ 0xcf
 
 

LupCnt equ .10 ; Number of iterations

 ; usart_lib res  0xd0 -> 0xd8

     END