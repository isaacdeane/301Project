.data
greeting: .asciiz "Hello"
arr:      .word 10, 20, -1
ptr:      .word greeting
val:      .word Loop

.text
.globl main
main:
    # immediates / moves
    li   $t0, 42              # small immediate
    li   $t1, 0x12345678      # great immediate：lui+ori）
    mov  $t2, $t0

    # logic R / I 
    and  $t3, $t1, $t2
    or   $t4, $t1, $t2
    nor  $t5, $t1, $t2
    xor  $t6, $t1, $t2
    andi $t7, $t2, 0x00ff
    ori  $s0, $t2, 1
    xori $s1, $t2, 0xffff

    lui  $s2, $zero, 0x1234

    # shifts
    sll  $s3, $t2, 2
    srl  $s4, $s3, 1

    # comparators 
    sge  $s5, $t2, $t0
    sgt  $s6, $t2, $t0
    sle  $s7, $t2, $t0
    seq  $t8, $t2, $t0
    sne  $t9, $t2, $t0

    # data address / load / store
    la   $a0, arr
    lw   $a1, 0($a0)
    sw   $a1, 4($a0)

Loop:
    addi $a2, $zero, 3
Back:
    addi $a2, $a2, -1
    bne  $a2, $zero, Back

    beq  $a2, $zero, Fwd
    addi $a2, $a2, 1 
Fwd:
    # pseudoinstructions
    blt  $t0, $t1, AfterLT
    bge  $t0, $t1, AfterLT
AfterLT:
    bgt  $t1, $t0, AfterGT
    ble  $t1, $t0, AfterGT
AfterGT:
    syscall
