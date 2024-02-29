#include <iostream>

#include "type/functiontype.hpp"
#include "type/integertype.hpp"

#include "constant.hpp"
#include "function.hpp"
#include "gen.hpp"
#include "instruction.hpp"
#include "print.hpp"
#include "variable.hpp"

void
defineGlobalVariable()
{
    using namespace abc;
    using namespace gen;

    auto intType = IntegerType::createSigned(32)->getAlias("int");

    // extern declaration
    globalVariableDefinition("foo_global",
			     intType, 
			     nullptr,
			     true);

    // definition with initializer
    globalVariableDefinition("foo_global",
			     intType, 
			     getConstantInt("42", intType, 10), 
			     false);
}

void
someInstructions()
{
    using namespace abc;
    using namespace gen;

    auto intType = IntegerType::createSigned(32)->getAlias("int");

    localVariableDefinition("foo", intType);
    localVariableDefinition("bar", intType);
    localVariableDefinition("foobar", intType);

    auto left = getConstantInt("1", intType, 10);
    auto right = getConstantInt("2", intType, 10);
    returnInstruction(instruction(gen::ADD, left, right));
}

const abc::Type *
getMainFnType()
{
    using namespace abc;

    auto intType = IntegerType::createSigned(32)->getAlias("int");
    return FunctionType::create(intType, std::vector<const Type *>{});
}


void
defineMain()
{
    gen::functionDefinitionBegin("main",
				 getMainFnType(),
				 std::vector<const char *>{},
				 false);
    someInstructions();
    gen::functionDefinitionEnd();
}

int
main()
{
    gen::init();
    std::cerr << "generating hello.bc\n";

    defineMain();
    defineGlobalVariable();

    gen::printGlobalVariableList();
    gen::printFunctionList();

    gen::print("hello");
}
