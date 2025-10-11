int some_func(unsigned char a)
{
	int b = 0;

	a = (b + 3) * 5;
	b += a % 2;

	return b + 1;
}

int main(void)
{
	unsigned char c = 100;

	while(c > 0)
	{
		--c;
		c = ++some_func(doit(a++, thing(5, b * (4 + 3)), c--));
	}

	a += array[5][other[1](6, 7)];

	return my_struct.field->other();
}
