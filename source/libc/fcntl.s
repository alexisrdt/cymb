.global openat
.type openat, @function
openat:
	MOV X8, #56

	SVC #0

	RET
