.bss
.data
.text
.globl main
main:
main_0:
	JAL x0, main_1
main_1:
	JALR x0, 0(ra)
	ECALL