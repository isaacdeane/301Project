        .text
        .globl main

main:
        li   $t0, 0x00F00000     # test value for clz
        clz  $t1, $t0            # count leading zeros

        li   $t2, 0xFFF00000     # test value for clo
        clo  $t3, $t2            # count leading ones

        addi $v0, $0, 10
        syscall
