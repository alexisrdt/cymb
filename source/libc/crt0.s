.global main
.type main, @function

.global _start
.type _start, @function
_start:
	MOV FP, #0
	MOV LR, #0

	LDR X0, [SP]

	ADD X1, SP, #8

	BL main

	MOV X8, #93
	SVC #0
