#include <iostream>

#include "gen/constant.hpp"
#include "gen/function.hpp"
#include "gen/gen.hpp"
#include "gen/gen.hpp"
#include "gen/instruction.hpp"
#include "gen/print.hpp"
#include "gen/variable.hpp"
#include "type/floattype.hpp"
#include "type/integertype.hpp"
#include "type/functiontype.hpp"

#include "binaryexpr.hpp"
#include "identifier.hpp"
#include "floatliteral.hpp"

void
defineGlobalVariable()
{
    using namespace abc;
    using namespace gen;

    auto fltType = FloatType::createDouble()->getAlias("double");

    // extern declaration
    gen::externalVariableDeclaration("foo_global", fltType);

    // definition with initializer
    gen::globalVariableDefinition("foo_global",
				  fltType, 
				  getConstantFloat("1.25", fltType));
}

void
someInstructions()
{
    using namespace abc;
    auto fltType = FloatType::createDouble()->getAlias("double");

    auto fltExpr = FloatLiteral::create(1.25, fltType);
    auto idExpr = Identifier::create(UStr::create("foo_global"),
				     UStr::create("foo_global"),
				     fltType);
    auto addExpr = BinaryExpr::create(BinaryExpr::ADD,
				      std::move(fltExpr),
				      std::move(idExpr));

    gen::returnInstruction(addExpr->loadValue());
}

void
defineMain()
{
    using namespace abc;

    auto intType = IntegerType::createSigned(32)->getAlias("int");
    auto fnType =  FunctionType::create(intType,
					std::vector<const Type *>{});
    gen::functionDefinitionBegin("main",
				 fnType,
				 std::vector<const char *>{},
				 false);
    someInstructions();
    gen::functionDefinitionEnd();
}

int
main()
{
    gen::init();

    using namespace abc;

    defineGlobalVariable();
    defineMain();

    std::cerr << "generating expr.bc\n";
    gen::print("expr.bc");
}
