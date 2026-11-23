.global openat
.type openat, @function
openat:
	mov x8, 56

	svc 0

	ret
