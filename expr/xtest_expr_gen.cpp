#include <iostream>

#include "gen/constant.hpp"
#include "gen/function.hpp"
#include "gen/gen.hpp"
#include "gen/gen.hpp"
#include "gen/instruction.hpp"
#include "gen/print.hpp"
#include "gen/variable.hpp"
#include "type/integertype.hpp"
#include "type/functiontype.hpp"

#include "binaryexpr.hpp"
#include "identifier.hpp"
#include "integerliteral.hpp"

void
defineGlobalVariable()
{
    using namespace abc;
    using namespace gen;

    auto intType = IntegerType::createSigned(32)->getAlias("int");

    // extern declaration
    gen::globalVariableDefinition("foo_global",
				  intType, 
				  nullptr,
				  true);

    // definition with initializer
    gen::globalVariableDefinition("foo_global",
				  intType, 
				  getConstantInt("43", intType, 10), 
				  false);
}

void
someInstructions()
{
    using namespace abc;
    auto intType = IntegerType::createSigned(32)->getAlias("int");

    auto intExpr = IntegerLiteral::create(-1, intType);
    auto idExpr = Identifier::create(UStr::create("foo_global"),
				     UStr::create("foo_global"),
				     intType);
    auto addExpr = BinaryExpr::create(BinaryExpr::ADD,
				      std::move(intExpr),
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

    /*
    auto intExpr = IntegerLiteral::create(-1);

    std::cerr << intExpr << "\n";
    std::cerr << "signed val   = " << intExpr->getSignedIntValue() << "\n";
    std::cerr << "unsigned val = " << intExpr->getUnsignedIntValue() << "\n";

    auto idExpr = Identifier::create(UStr::create("a"),
				     IntegerType::createSigned(32));
    std::cerr << idExpr << "\n";

    auto addExpr = BinaryExpr::create(BinaryExpr::ADD,
				      std::move(intExpr),
				      std::move(idExpr));
    std::cerr << addExpr << "\n";
    */

    std::cerr << "generating expr.bc\n";
    gen::print("expr");
}
