.global main
.type main, @function

.global _start
.type _start, @function
_start:
	mov fp, 0
	mov lr, 0

	ldr x0, [sp]

	add x1, sp, 8

	bl main

	mov x8, 93
	svc 0
.size _start, . - _start
