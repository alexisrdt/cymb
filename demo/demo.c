int some_func()
{
	int a;
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
	}

	return 0;
}
