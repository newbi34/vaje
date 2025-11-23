HANOI   START   10

        LDA     #5
        JSUB    SOLVE


HALT    J        HALT

SOLVE   RMO     A, B
        RMO     L, A
        JSUB    PUSH
        RMO     T, A
        JSUB    PUSH
        RMO     B, A 

        JSUB    POP
        RMO     A, T
        JSUB    POP
        RMO     A, L
        

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

S_PTR   WORD    X'001000'

        END     HANOI