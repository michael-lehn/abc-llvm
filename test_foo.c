#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

uint64_t foo(uint64_t, uint64_t);

int
main(int argc, char *argv[])
{
    uint64_t a = argc >= 2 ? strtoll(argv[1], 0, 10) : 13;
    uint64_t b = argc >= 3 ? strtoll(argv[2], 0, 10) : 31;
    printf("foo(%" PRId64 ", %" PRId64 ") = %" PRId64 "\n", a, b, foo(a, b));
}
