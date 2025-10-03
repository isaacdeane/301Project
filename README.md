# 301Project

# Description
This project implements one or more MIPS files into two raw binaries.

# Build the assembler
make

# How to run
./assemble Testcases/Assembly/<filename>.asm <filename>_static.bin <filename>_inst.bin
./assemble Testcases/Assembly/<file1>.asm <file2>.asm <filename>_static.bin <filename>_inst.bin

# Assumptions
.data: labels are on the same line as .word (e.g., arr: .word 1, 2).
.text: labels are on their own line (e.g., loop:).
