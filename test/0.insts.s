.bss
.data
.text
.globl main
main: 
main_0: 
	AND x1, x2, x3
	OR x1, x2, x3
	ANDI x1, x2, 255 # 0xFF
	ORI x1, x2, 1
	XORI x1, x2, -1
	SLL x1, x2, x3
	SRL x1, x2, x3
	SRA x1, x2, x3
	SLLI x1, x2, 3
	SRLI x1, x2, 4
	SLT x1, x2, x3
	LW x1, 0(sp)
	LBU x1, 0(x2)