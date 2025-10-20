.bss
.data
.text
.globl main
main: 
	# call extern symbol foo
	auipc t1, %pcrel_hi(foo)
	jalr ra, t1, %pcrel_lo(foo)
