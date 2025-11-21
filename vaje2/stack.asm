STACK   START   0

HALT    J       

SINIT   LDA     #STK
        ADD     #3000
        RMO     A, B
        LDA     #STK

LOOP    COMPR   A, B
        JEQ     END

        CLEAR   T
        STA     STKP
        STT     @STKP
        ADD     #3
        J       LOOP

END     RSUB

SPUSH   STA     @STKP
        LDA     STKP
        ADD     #3
        STA     STKP
        RSUB

SPOP    LDA     STKP
        SUB     #3
        STA     STKP
        LDA     @STKP
        RSUB

STKP    WORD    0
STK     RESW    1000

END     STACK