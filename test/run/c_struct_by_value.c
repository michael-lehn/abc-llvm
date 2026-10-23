/* C part of c_struct_by_value.abc */

#include <stdint.h>

typedef struct { float x, y; } Vec2;
typedef struct { int64_t a, b; } Pair;
typedef struct { float x, y, w, h; } Rect;

Vec2
c_vec_add(Vec2 a, Vec2 b)
{
    Vec2 r = { a.x + b.x, a.y + b.y };
    return r;
}

int64_t
c_pair_sum(Pair p)
{
    return p.a + p.b;
}

float
c_rect_area(Rect r)
{
    return r.w * r.h;
}
