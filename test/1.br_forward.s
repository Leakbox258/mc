.bss
.data
.text
.globl main
main: 
main_0:
	BEQ x1, x2, main_1
	BNE x1, x2, main_2
	BLT x1, x2, main_3
	BGE x1, x2, main_4
	BLTU x1, x2, main_5
	BGEU x1, x2, main_6
main_1:
	addi x0, x0, 114
main_2:
	addi x0, x0, 115
main_3:
	addi x0, x0, 116
main_4:
	addi x0, x0, 117
main_5:
	addi x0, x0, 118
main_6:
	addi x0, x0, 119
	