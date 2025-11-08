.global syscall
.type syscall, @function
syscall:
	MOV X8, X0

	MOV X0, X1
	MOV X1, X2
	MOV X2, X3
	MOV X3, X4
	MOV X4, X5
	MOV X5, X6

	SVC #0
	RET

.global close
.type close, @function
close:
	MOV X8, #57

	SVC #0
	RET

.global read
.type read, @function
read:
	MOV X8, #63

	SVC #0
	RET

.global write
.type write, @function
write:
	MOV X8, #64

	SVC #0
	RET
