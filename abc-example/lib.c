#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static bool getErrCode;

uint64_t
getErr(void)
{
    return getErrCode;
}

uint64_t
getU64(void)
{
    uint64_t val;
    getErrCode = scanf("%" SCNd64, &val) != 1;
    return val;
}

void
printU64(uint64_t val)
{
    printf("%" PRId64, val);
}

void
printNl(void)
{
    printf("\n");
}

void
printLn(const char *s)
{
    printf("%s\n", s);
}


void
print(char *s)
{
    printf("%s", s);
}

uint16_t
getChar()
{
    return getchar();
}

void
putChar(uint16_t ch)
{
    putchar(ch);
}

void
putCharHex(uint16_t ch)
{
    int c = ch;
    printf("0x%02X", c);
}
