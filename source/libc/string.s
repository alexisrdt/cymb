.global strlen
.type strlen, @function
strlen:
	MOV X1, XZR

	unaligned:

	EOR X2, X0, #0xF
	CBNZ X2, loop

	LDRB W2, [X0], #1
	CBZ X2, end

	ADD X1, X1, #1

	B unaligned

	loop:

	LDR Q0, [X0], #16
	CMEQ V0.16B, V0.16B, #0

	MOV X2, V0.D[0]
	CBNZ X2, found
	ADD X1, X1, #8

	MOV X2, V0.D[1]
	CBNZ X2, found
	ADD X1, X1, #8

	B loop

	found:

	RBIT X2, X2
	CLZ X2, X2
	LSR X2, X2, #3

	ADD X1, X1, X2

	end:
	MOV X0, X1
	RET
