/* C part of byval_tail_recursion_inline.abc: reads the caller's object
 * through a pointer, opaque to the optimizer of the ABC program. */

struct Vec
{
    long long v[4];
    int n;
};

long long
readFirst(const struct Vec *s)
{
    return s->v[0];
}

int
readCount(const struct Vec *s)
{
    return s->n;
}
