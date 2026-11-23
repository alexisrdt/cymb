.global strlen
.type strlen, @function
strlen:
	mov x1, xzr

	unaligned:

	eor x2, x0, 0xf
	cbnz x2, loop

	ldrb w2, [x0], 1
	cbz x2, end

	add x1, x1, 1

	b unaligned

	loop:

	ldr q0, [X0], 16
	cmeq v0.16b, v0.16b, 0

	mov x2, v0.d[0]
	cbnz x2, found
	add x1, x1, 8

	mov x2, v0.d[1]
	cbnz x2, found
	add x1, x1, 8

	b loop

	found:

	rbit x2, x2
	clz x2, x2
	lsr x2, x2, 3

	add x1, x1, x2

	end:
	mov x0, x1
	ret
