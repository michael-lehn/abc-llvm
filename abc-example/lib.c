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
