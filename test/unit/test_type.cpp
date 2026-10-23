// TEST: T0155
#include "type/arraytype.hpp"
#include "type/floattype.hpp"
#include "type/functiontype.hpp"
#include "type/inittypesystem.hpp"
#include "type/integertype.hpp"
#include "type/nullptrtype.hpp"
#include "type/pointertype.hpp"
#include "type/structtype.hpp"
#include "type/voidtype.hpp"

#include "check.hpp"

using namespace abc;

static const Type *i8, *i16, *i32, *i64, *u8, *u32, *u64;
static const Type *flt, *dbl, *boolTy, *voidTy, *nullTy;

static void
initTypes()
{
    initTypeSystem();
    i8 = IntegerType::createSigned(8);
    i16 = IntegerType::createSigned(16);
    i32 = IntegerType::createSigned(32);
    i64 = IntegerType::createSigned(64);
    u8 = IntegerType::createUnsigned(8);
    u32 = IntegerType::createUnsigned(32);
    u64 = IntegerType::createUnsigned(64);
    flt = FloatType::createFloat();
    dbl = FloatType::createDouble();
    boolTy = IntegerType::createBool();
    voidTy = VoidType::create();
    nullTy = NullptrType::create();
}

static const Type *
ptr(const Type *refType)
{
    return PointerType::create(refType);
}

static const Type *
arr(const Type *refType, std::size_t dim)
{
    return ArrayType::create(refType, dim);
}

//------------------------------------------------------------------------------

static void
testIntegerTypes()
{
    // types are unique: equal types are represented by the same object
    CHECK(IntegerType::createSigned(32) == i32);
    CHECK(i32 != u32);
    CHECK(i32->isInteger() && i32->isSignedInteger());
    CHECK(u32->isInteger() && u32->isUnsignedInteger());
    CHECK_EQ(i64->numBits(), 64u);
    CHECK(boolTy->isBool() && boolTy->numBits() == 1);
    CHECK(!i8->isBool());
    CHECK(!i32->isFloatType() && !i32->isPointer() && i32->isScalar());
    // char is an unsigned 8 bit integer
    CHECK(Type::equals(IntegerType::createChar(), u8));
}

static void
testConstQualifier()
{
    auto ci32 = i32->getConst();
    CHECK(ci32->hasConstFlag());
    CHECK(!i32->hasConstFlag());
    CHECK(ci32->getConstRemoved() == i32);
    CHECK(ci32->getConst() == ci32);
    CHECK(!Type::equals(i32, ci32));
    CHECK(Type::assignable(i32));
    CHECK(!Type::assignable(ci32));
    // an array of readonly elements is not assignable
    CHECK(!Type::assignable(arr(ci32, 3)));
    CHECK(Type::assignable(arr(i32, 3)));
    // readonly pointer vs pointer to readonly
    CHECK(!Type::assignable(ptr(i32)->getConst()));
    CHECK(Type::assignable(ptr(ci32)));
}

static void
testTypeAlias()
{
    auto myInt = i32->getAlias("MyInt");
    CHECK(myInt != i32);
    CHECK(myInt->isAlias() && myInt->getUnalias() == i32);
    CHECK(Type::equals(myInt, i32));
    CHECK(myInt->isInteger() && myInt->numBits() == 32);
    CHECK_EQ(std::string{myInt->ustr().c_str()}, "MyInt");
    auto ptrAlias = ptr(dbl)->getAlias("DblPtr");
    CHECK(ptrAlias->isPointer() && ptrAlias->refType() == dbl);
}

static void
testPointerAndArrayTypes()
{
    CHECK(ptr(i32) == ptr(i32));
    CHECK(ptr(i32)->isPointer() && ptr(i32)->refType() == i32);
    CHECK(Type::equals(ptr(i32), ptr(i32)));
    CHECK(!Type::equals(ptr(i32), ptr(u32)));
    CHECK(!Type::equals(ptr(i32), ptr(i32->getConst())));
    CHECK(nullTy->isPointer() && nullTy->isNullptr());

    auto a = arr(i16, 10);
    CHECK(a->isArray() && a->dim() == 10 && a->refType() == i16);
    CHECK(!a->isScalar());
    CHECK(!Type::equals(a, arr(i16, 11)));
    CHECK(Type::equals(a, arr(i16, 10)));
    CHECK_EQ(a->aggregateSize(), 10u);
    CHECK(a->aggregateType(3) == i16);

    // array[2][3] of int: array of 2 arrays of 3 ints
    auto m = arr(arr(i32, 3), 2);
    CHECK(m->dim() == 2 && m->refType()->dim() == 3);

    auto unbound = arr(i32, 0);
    CHECK(unbound->isUnboundArray());
    CHECK(!a->isUnboundArray());
    CHECK(Type::patchUnboundArray(unbound, 4) == arr(i32, 4));
    CHECK(Type::patchUnboundArray(a, 4) == a);
}

static void
testFunctionTypes()
{
    auto f = FunctionType::create(i32, {i32, dbl});
    CHECK(f->isFunction() && f->retType() == i32);
    CHECK_EQ(f->paramType().size(), 2u);
    CHECK(!f->hasVarg());
    CHECK(Type::equals(f, FunctionType::create(i32, {i32, dbl})));
    CHECK(!Type::equals(f, FunctionType::create(i32, {i32, dbl}, true)));
    CHECK(!Type::equals(f, FunctionType::create(i64, {i32, dbl})));
    CHECK(!Type::equals(f, FunctionType::create(i32, {dbl, i32})));
    CHECK(!Type::equals(f, FunctionType::create(i32, {i32})));
    auto g = FunctionType::create(voidTy, {}, true);
    CHECK(g->retType()->isVoid() && g->hasVarg() && g->paramType().empty());
    // pointer to function returning pointer to function
    auto h = ptr(FunctionType::create(ptr(f), {i32}));
    CHECK(h->refType()->retType()->refType() == f);
}

static void
testStructTypes()
{
    auto s = StructType::createIncomplete(UStr::create("S"));
    CHECK(s->isStruct() && !s->hasSize());
    s->complete({UStr::create("a"), UStr::create("b")}, {0, 1}, {i32, dbl});
    CHECK(s->hasSize());
    CHECK(s->memberIndex(UStr::create("b")) == 1);
    CHECK(!s->memberIndex(UStr::create("c")));
    CHECK(s->memberType(UStr::create("b")) == dbl);
    CHECK_EQ(s->aggregateSize(), 2u);
    CHECK(s->aggregateType(0) == i32);

    // structs are equal only if they are the same declaration
    auto t = StructType::createIncomplete(UStr::create("T"));
    t->complete({UStr::create("a"), UStr::create("b")}, {0, 1}, {i32, dbl});
    CHECK(Type::equals(s, s));
    CHECK(!Type::equals(s, t));
    CHECK(Type::convert(s, s) == s);
    CHECK(!Type::convert(s, t));

    // members of a readonly struct are readonly
    CHECK(s->getConst()->memberType(UStr::create("a"))->hasConstFlag());
}

static void
testCommonType()
{
    CHECK(Type::common(i32, i32) == i32);
    CHECK(Type::common(i8, i16) == i16);
    // mixing signed and unsigned is an open question (#0028)
    // floating point wins over integer, double over float
    CHECK(Type::common(i32, dbl) == dbl);
    CHECK(Type::common(dbl, i64) == dbl);
    CHECK(Type::common(flt, dbl) == dbl);
    CHECK(Type::common(i64, flt) == flt);
    CHECK(Type::common(ptr(i32), nullTy) == ptr(i32));
    CHECK(Type::common(nullTy, ptr(i32)) == ptr(i32));
    CHECK(Type::common(ptr(i32), ptr(dbl)) == nullptr);
    // arrays with same element type but different dimension: pointer
    CHECK(Type::common(arr(i32, 3), arr(i32, 4)) == ptr(i32));
}

static void
testImplicitConversion()
{
    // arithmetic types convert implicitly to each other
    CHECK(Type::convert(i32, dbl) == dbl);
    CHECK(Type::convert(dbl, i32) == i32);
    CHECK(Type::convert(u64, i8) == i8);
    CHECK(Type::convert(i32, boolTy) == boolTy);
    CHECK(Type::convert(ptr(i32), boolTy) == boolTy);
    CHECK(!Type::convert(dbl, boolTy));

    // no implicit conversion between pointers and integers
    CHECK(!Type::convert(i64, ptr(i32)));
    CHECK(!Type::convert(ptr(i32), i64));

    // pointer conversions
    CHECK(Type::convert(nullTy, ptr(i32)) == ptr(i32));
    CHECK(Type::convert(ptr(i32), ptr(voidTy)) == ptr(voidTy));
    CHECK(Type::convert(ptr(voidTy), ptr(i32)) == ptr(i32));
    CHECK(!Type::convert(ptr(i32), ptr(dbl)));
    CHECK(!Type::convert(ptr(i32), ptr(u32)));
    CHECK(Type::convert(ptr(i32), ptr(i32->getConst())));
    // a readonly qualifier of the referenced type can not be dropped
    CHECK(!Type::convert(ptr(i32->getConst()), ptr(i32)));

    // arrays decay to pointers to their element type
    CHECK(Type::convert(arr(i32, 3), ptr(i32)) == ptr(i32));
    CHECK_KNOWN_BUG("T0157", "#0007", !Type::convert(arr(i32, 3), ptr(dbl)));
    // arrays only convert to arrays with the same dimension (or unbound)
    CHECK(Type::convert(arr(i32, 3), arr(i32, 3)));
    CHECK(!Type::convert(arr(i32, 3), arr(i32, 4)));
    CHECK(Type::convert(arr(i32, 3), arr(i32, 0)));
    CHECK(!Type::convert(arr(i32, 3), i32));
}

static void
testExplicitCast()
{
    // anything that converts implicitly can be cast explicitly
    CHECK(Type::explicitCast(dbl, i32) == i32);
    // explicit casts may drop readonly
    CHECK(Type::explicitCast(ptr(i32->getConst()), ptr(i32)) == ptr(i32));
    // explicit casts between unrelated pointer types and integer to pointer
    CHECK(Type::explicitCast(ptr(i32), ptr(dbl)) == ptr(dbl));
    CHECK(Type::explicitCast(i64, ptr(i32)) == ptr(i32));
    // pointers can not be cast to integers
    CHECK(!Type::explicitCast(ptr(i32), i64));
    CHECK(!Type::explicitCast(i32, arr(i32, 2)));
}

int
main(int argc, char *argv[])
{
    initTypes();
    testIntegerTypes();
    testConstQualifier();
    testTypeAlias();
    testPointerAndArrayTypes();
    testFunctionTypes();
    testStructTypes();
    testCommonType();
    testImplicitConversion();
    testExplicitCast();
    return check::summary(argv[0]);
}
