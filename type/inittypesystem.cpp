#include "arraytype.hpp"
#include "enumtype.hpp"
#include "functiontype.hpp"
#include "inittypesystem.hpp"
#include "integertype.hpp"
#include "nullptrtype.hpp"
#include "pointertype.hpp"
#include "structtype.hpp"
#include "typealias.hpp"
#include "voidtype.hpp"

namespace abc {

void
initTypeSystem()
{
    ArrayType::init();
    EnumType::init();
    FunctionType::init();
    IntegerType::init();
    NullptrType::init();
    PointerType::init();
    StructType::init();
    TypeAlias::init();
    VoidType::init();
}

} // namespace abc
