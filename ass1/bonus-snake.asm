. graphical screen settings:
. Address: 0x00A000
. Columns: 16
. Rows: 16
. Pixel size: 24
.
. keyboard settings: 
. Address: 0x00C000
.
. frequency: 4000

SNAKE   START   0

FIRST   LDA     #0x44
        STCH    DIREC
        LDT     #3
        
        JSUB    RAN015          .generate new food
        STA     FOODX
        JSUB    RAN015
        STA     FOODY

LOOP    JSUB    RENDER

        JSUB    WAIT

        LDS     #0x100
        JSUB    RENDER 
        CLEAR   S

        LDA     @SNAKEX         .if (food was eaten)
        COMP    FOODX
        JEQ     L14
        J       L20

L14     LDA     @SNAKEY
        COMP     FOODY
        JEQ     L16
        J       L20

L16     LDA     #1
        STA     EATEN
        LDA     LEN             .temp = snake[LEN - 1]
        SUB     #1
        RMO     A, X
        LDA     SNAKEX
        MULR    T, X
        ADDR    X, A
        STA     ADR
        LDA     @ADR
        STA     TEMPX

        LDA     LEN
        SUB     #1
        RMO     A, X
        LDA     SNAKEY
        MULR    T, X
        ADDR    X, A
        STA     ADR
        LDA     @ADR
        STA     TEMPY

        JSUB    RAN015          .generate new food
        STA     FOODX
        JSUB    RAN015
        STA     FOODY

        LDA     LEN
        RMO     A, X
        ADD     #1
        STA     LEN

        .propagate position through the snake
        .for (i = LEN - 2, i >= 0 ) snakex/y[i + 1] = snakex/y[i] ???
L20     LDA     LEN
        SUB     #2
        RMO     A,X 

L18     RMO     X, A
        COMP    #0
        .LDA     #0
        .COMPR   X, A
        JLT     L19

        LDA     SNAKEX
        MULR    T, X
        ADDR    X, A
        STA     ADR
        LDS     @ADR            .S = snakex[i]

        ADD     #3
        STA     ADR             
        STS     @ADR            .snakex[i + 1] = snakex[i]

        LDA     SNAKEY
        .MULR    T, X
        ADDR    X, A
        STA     ADR
        LDS     @ADR            .S = snakey[i]

        ADD     #3
        STA     ADR             
        STS     @ADR            .snakey[i + 1] = snakey[i]

        DIVR    T, X
        RMO     X, A            .X--
        SUB     #1
        RMO     A, X

        J       L18

L19     .if (eaten) ..LEN++, snake[LEN - 1] = temp    
        LDA     EATEN
        COMP    #1
        JEQ     L17
        J       L15 

L17     LDA     LEN
        SUB     #1
        RMO     A, X
        LDA     SNAKEX
        MULR    T, X
        ADDR    X, A
        STA     ADR
        LDA     TEMPX
        STA     @ADR

        LDA     SNAKEY
        .MULR    T, X
        ADDR    X, A
        STA     ADR
        LDA     TEMPY
        STA     @ADR

L15     CLEAR   A
        STA     EATEN

        .LDS     #0x100
        .JSUB    RENDER 
        .CLEAR   S

        CLEAR   A
        LDCH    @KEY_IN

L10     COMP    #0x41           .A
        JEQ     L3
        COMP    #0x57           .W
        JEQ     L4
        COMP    #0x44           .D
        JEQ     L5
        COMP    #0x53           .S
        JEQ     L6

        LDCH    DIREC
        J       L10

L3      STCH    DIREC
        LDA     @SNAKEX         .move left
        SUB     #1
        STA     @SNAKEX
        J       L7

L4      STCH    DIREC
        LDA     @SNAKEY         .move up
        SUB     #1       
        STA     @SNAKEY
        J       L7

L5      STCH    DIREC
        LDA     @SNAKEX         .move right
        ADD     #1        
        STA     @SNAKEX
        J       L7

L6      STCH    DIREC
        LDA     @SNAKEY         .move down
        ADD     #1        
        STA     @SNAKEY

L7      .check bounds
        .snakex/y[0] >= 0, <= 15
        LDA     @SNAKEX
        COMP    #0
        JLT     HALT
        COMP    #15
        JGT     HALT

        LDA @SNAKEY
        COMP    #0
        JLT     HALT
        COMP    #15
        JGT     HALT

        .check snake
        .for (i = 1, i < LEN) snakex/y[0] != snakex/y[i]


        J       LOOP

HALT    J       HALT  

.generate random number from 0 to 15
.no arguments, returns in A 
RAN015  LDA     SEED
        ADDR    B, A
        +AND     #0x0FFFFF
        STA     SEED         

        MUL     #13
        ADD     #17         

        AND     #0x0000FF
        DIV     #16          .between 0 and 15
        RSUB

.waits some time before reading input so it doesnt feel laggy
.no arguements, no return
.changes A, X
WAIT    LDA     #0
        LDX     #0x99
L8      ADD     #1
        ADDR    A, B            .increment B which servers at a timer for randomness
        COMPR   A, X
        JEQ     L9
        J       L8
L9      RSUB

.draws the snake
.no arguements, no return
.changes A
RENDER  RMO     L, A
        JSUB    PUSH            .prolouge
        RMO     X, A
        JSUB    PUSH
        RMO     T, A
        JSUB    PUSH

        RMO     S, A
        COMP    #0
        JEQ     L12

        LDA     LEN             .remove tail
        SUB     #1              .get SNAKEX[LEN - 1]
        RMO     A, X
        JSUB    OFFSET
        ADD     SCREEN
        STA     ADR
        CLEAR   A
        STCH    @ADR

        RMO     S, A
        COMP    #0x100
        JEQ     L2              .remove only

L12     CLEAR   X               .draw head
        JSUB    OFFSET
        ADD     SCREEN
        STA     ADR
        CLEAR   A
        LDCH    #240
        STCH    @ADR           

L1      RMO     X, A            .draw others
        ADD     #1
        RMO     A, X
        
        COMP    LEN
        JEQ     L2

        JSUB    OFFSET          
        ADD     SCREEN
        STA     ADR
        CLEAR   A
        LDCH    #255
        STCH     @ADR

        J       L1

L2      RMO     S, A
        COMP    #0
        JEQ     L13

        LDA     FOODY           .draw food
        MUL     #16
        ADD     FOODX
        ADD     SCREEN
        STA     ADR
        CLEAR   A
        LDCH    #50
        STCH     @ADR

L13     JSUB    POP             .epilouge
        RMO     A, T
        JSUB    POP             
        RMO     A, X
        JSUB    POP
        RMO     A, L
        RSUB

.calculates the offset of the snake cell at index X for rendering
.offset = 16 * snakey[index] + snakex[index]
.arguements: X - index
.return: A - offset
.changes A, T  
OFFSET  RMO     X, A
        MUL     #3          
        ADD     SNAKEY
        STA     ADR
        LDA     @ADR
        MUL     #16             
        RMO     A, T

        RMO     X, A
        MUL     #3          
        ADD     SNAKEX
        STA     ADR
        LDA     @ADR
        ADDR    T, A
        RSUB

.pushes A to stack
.changes A 
PUSH    STA     @ST_PTR         
        LDA     ST_PTR
        ADD     #3
        STA     ST_PTR
        RSUB

.pops top of stack into A
POP     LDA     ST_PTR         
        SUB     #3
        STA     ST_PTR
        LDA     @ST_PTR
        RSUB

LEN     WORD    1               .snake length
SCREEN  WORD    X'00A000'       .screen 
KEY_IN  WORD    X'00C000'       .input 
ST_PTR  WORD    X'002000'       .stack pointer... 20kB space
SNAKEX  WORD    X'000A00'       .pointer to an array of snake x coords, first input is head, SNAKEX + LEN is tail, 3 byte elements, upper left corner is 0, 0
SNAKEY  WORD    X'000C00'       .pointer to an array of snake y coords, offset 0x200 from SNAKEX
ADR     WORD    0               .actual address of the x / y coord since sic/xe doesnt support indirect and indexed addressing
DIREC   BYTE    0               .movement direction, gets added to snake's head 
SEED    WORD    125
FOODX   WORD    17
FOODY   WORD    17
TEMPX   WORD    0
TEMPY   WORD    0
EATEN   WORD    0

        END     FIRST
