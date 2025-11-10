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
        STCH     DIREC
LOOP    JSUB    RENDER

        JSUB    WAIT

        .if (food was eaten) temp = snake[LEN - 1], generate new food:

        .propagate position through the snake
        .for (i = LEN - 2, i >= 0 ) snakex/y[i + 1] = snakex/y[i] 

        .if (eaten) LEN++, snake[LEN - 1] = temp    

        LDS     #0x100
        JSUB    RENDER 
        CLEAR   S

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
        .check snake
        .for (i = 1, i < LEN) snakex/y[0] != snakex/y[i]

        J       LOOP

HALT    J       HALT  

.waits some time before reading input so it doesnt feel laggy
.no arguements, no return
.changes A, X
WAIT    LDA     #0
        LDX     #0xA0
L8      ADD     #1
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

        CLEAR   X               .draw head
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
        STA     @ADR

        J       L1

L2      JSUB    POP             .epilouge
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
SCREEN  WORD    X'00A000'       .screen pointer
KEY_IN  WORD    X'00C000'       .input pointer
ST_PTR  WORD    X'000200'       .stack pointer, 2kB of space
SNAKEX  WORD    X'000A00'       .pointer to an array of snake x coords, first input is head, SNAKEX + LEN is tail, 3 byte elements, upper left corner is 0, 0
SNAKEY  WORD    X'000C00'       .pointer to an array of snake y coords, offset 0x200 from SNAKEX
ADR     WORD    0               .actual address of the x / y coord since sic/xe doesnt support indirect and indexed addressing
DIREC   BYTE    0               .movement direction, gets added to snake's head 

        END     FIRST
