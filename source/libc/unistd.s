.global syscall
.type syscall, @function
syscall:
	mov x8, x0

	mov x0, x1
	mov x1, x2
	mov x2, x3
	mov x3, x4
	mov x4, x5
	mov x5, x6

	svc 0
	ret

.global close
.type close, @function
close:
	mov x8, 57

	svc 0
	ret

.global read
.type read, @function
read:
	mov x8, 63

	svc 0
	ret

.global write
.type write, @function
write:
	mov x8, 64

	svc 0
	ret
