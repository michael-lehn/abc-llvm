#include "expr/assertexpr.hpp"
#include "gen/function.hpp"
#include "type/functiontype.hpp"
#include "type/integertype.hpp"
#include "type/pointertype.hpp"

#include "defaultdecl.hpp"

namespace abc {

void
initDefaultDecl()
{
    std::vector<const Type *> param;
    param.push_back(PointerType::create(IntegerType::createChar()));
    param.push_back(PointerType::create(IntegerType::createChar()));
    param.push_back(IntegerType::createInt());

    auto assertType =
      FunctionType::create(IntegerType::createBool(), std::move(param), false);

    auto assertName = UStr::create("__assert");
    AssertExpr::setFunction(assertName, assertType);

    gen::functionDeclaration(assertName.c_str(), assertType, true);
}

} // namespace abc
