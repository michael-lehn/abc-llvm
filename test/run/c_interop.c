/* C part of c_interop.abc */

#include <stdint.h>

int abc_triple(int x);
extern int abc_value;

int c_counter = 7;

int
c_add(int a, int b)
{
    return a + b;
}

int64_t
c_mul64(int64_t a, int64_t b)
{
    return a * b;
}

double
c_scale(double x, float f)
{
    return x * f;
}

unsigned char
c_next_u8(unsigned char x)
{
    return x + 1;
}

short
c_negate_i16(short x)
{
    return -x;
}

void
c_fill(int *p, int n)
{
    for (int i = 0; i < n; ++i) {
	p[i] = i * i;
    }
}

const char *
c_greeting(void)
{
    return "C";
}

int
c_apply(int (*f)(int), int x)
{
    return f(x);
}

int
c_call_abc(int x)
{
    return abc_triple(x) + abc_value;
}
