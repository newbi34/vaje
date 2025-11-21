REC     START   0

FIRST   LDT     #10
        RD      DEVICE
        COMP    #13
        JEQ     SKIP    .read \r
        AND     MASK1
        COMP    #0        
        JEQ     END
        SUB     #48     .convert to int from char
        ADDR    A, B
        MULR    T, B
        J       FIRST   .read next digit

SKIP    RD      DEVICE  .read \n
        CLEAR   A
        ADD     #1
        RMO     A, X        
        RMO     B, A
        DIV     #10

        COMP    #0        
        JEQ     END
   
        JSUB    FAK
        RMO     X, A
        JSUB    PRINT

        CLEAR   B
        J       FIRST

END     CLEAR   A
        ADD     #48
        WD      STDOUT
HALT    J       HALT

FAK     RMO     A, T
        RMO     L, A
        JSUB    PUSH    .push return address
        RMO     T, A

        COMP    #1
        JEQ     RET

        MULR    A, X
        SUB     #1
        JSUB    FAK

RET     RMO     A, T
        JSUB    POP     .get return address
        RMO     A, L
        RMO     T, A

        RSUB

PUSH    STA     @S_PTR
        LDA     S_PTR
        ADD     #3
        STA     S_PTR
        RSUB

POP     LDA     S_PTR
        SUB     #3
        STA     S_PTR
        LDA     @S_PTR
        RSUB

PRINT   RMO     A, B
        LDS     #0

LOOP1   AND     MASK
        SHIFTL  A, 4
        COMP    #10
        JLT     NUMBER
        ADD     #55     .hex numbers
        J       SKIP1
NUMBER  ADD     #48     .dec numbers
SKIP1   WD      STDOUT
        SHIFTL  B, 4
        RMO     S, A
        COMP    #5
        ADD     #1
        RMO     A, S
        RMO     B, A
        JLT     LOOP1
        LDA     #10
        WD      STDOUT
        RSUB

MASK1   WORD    X'0000FF'
MASK    WORD    X'F00000'
STDOUT	BYTE	X'01'
DEVICE  BYTE    X'FA'
S_PTR   WORD    X'001000'

	END 	FIRST