#include <iostream>

#include "functiontype.hpp"
#include "integertype.hpp"
#include "floattype.hpp"

void
intExample(bool signedInt, std::size_t numBits, const char *alias)
{
    using namespace abc;

    auto ty = signedInt
	? IntegerType::createSigned(numBits)
	: IntegerType::createUnsigned(numBits);
    auto tyConst = ty->getConst();
    auto tyCheck = tyConst->getConstRemoved();

    auto tyAlias = ty->getAlias(UStr::create(alias));
    auto tyAliasConst = tyAlias->getConst();
    auto tyAliasCheck = tyAliasConst->getConstRemoved();

    std::cerr << (signedInt? "Signed" : "Unsigned")
	<< " Integer with " << numBits << " bits\n";

    std::cerr << "  ty = " << ty << ", (void *)ty = " << (void *)ty << "\n";
    std::cerr << "  tyConst = " << tyConst
	<< ", (void *)tyConst = " << (void *)tyConst << "\n";
    std::cerr << "  tyCheck = " << tyCheck
	<< ", (void *)tyCheck = " << (void *)tyCheck << "\n";

    std::cerr << "Type alias " << alias << "\n";
    std::cerr << "  tyAlias = " << tyAlias
	<< ", (void *)tyAlias = " << (void *)tyAlias << "\n";
    std::cerr << "  tyAliasConst = " << tyAliasConst
	<< ", (void *)tyAliasConst = " << (void *)tyAliasConst << "\n";
    std::cerr << "  tyAliasCheck = " << tyAliasCheck
	<< ", (void *)tyAliasCheck = " << (void *)tyAliasCheck << "\n";
    std::cerr << "\n";
}

void
floatExample(const char *alias)
{
    using namespace abc;

    auto ty = FloatType::createFloat();
    auto tyConst = ty->getConst();
    auto tyCheck = tyConst->getConstRemoved();

    auto tyAlias = ty->getAlias(UStr::create(alias));
    auto tyAliasConst = tyAlias->getConst();
    auto tyAliasCheck = tyAliasConst->getConstRemoved();

    std::cerr << "float with alias " << alias << "\n";
    std::cerr << "  ty = " << ty << ", (void *)ty = " << (void *)ty << "\n";
    std::cerr << "  tyConst = " << tyConst
	<< ", (void *)tyConst = " << (void *)tyConst << "\n";
    std::cerr << "  tyCheck = " << tyCheck
	<< ", (void *)tyCheck = " << (void *)tyCheck << "\n";

    std::cerr << "Type alias " << alias << "\n";
    std::cerr << "  tyAlias = " << tyAlias
	<< ", (void *)tyAlias = " << (void *)tyAlias << "\n";
    std::cerr << "  tyAliasConst = " << tyAliasConst
	<< ", (void *)tyAliasConst = " << (void *)tyAliasConst << "\n";
    std::cerr << "  tyAliasCheck = " << tyAliasCheck
	<< ", (void *)tyAliasCheck = " << (void *)tyAliasCheck << "\n";
    std::cerr << "\n";
}

void
doubleExample(const char *alias)
{
    using namespace abc;

    auto ty = FloatType::createDouble();
    auto tyConst = ty->getConst();
    auto tyCheck = tyConst->getConstRemoved();

    auto tyAlias = ty->getAlias(UStr::create(alias));
    auto tyAliasConst = tyAlias->getConst();
    auto tyAliasCheck = tyAliasConst->getConstRemoved();

    std::cerr << "double with alias " << alias << "\n";
    std::cerr << "  ty = " << ty << ", (void *)ty = " << (void *)ty << "\n";
    std::cerr << "  tyConst = " << tyConst
	<< ", (void *)tyConst = " << (void *)tyConst << "\n";
    std::cerr << "  tyCheck = " << tyCheck
	<< ", (void *)tyCheck = " << (void *)tyCheck << "\n";

    std::cerr << "Type alias " << alias << "\n";
    std::cerr << "  tyAlias = " << tyAlias
	<< ", (void *)tyAlias = " << (void *)tyAlias << "\n";
    std::cerr << "  tyAliasConst = " << tyAliasConst
	<< ", (void *)tyAliasConst = " << (void *)tyAliasConst << "\n";
    std::cerr << "  tyAliasCheck = " << tyAliasCheck
	<< ", (void *)tyAliasCheck = " << (void *)tyAliasCheck << "\n";
    std::cerr << "\n";
}

void
fnExample()
{
    using namespace abc;

    auto ret =  IntegerType::createSigned(32)->getAlias("int");
    std::vector<const Type *> param = {
	IntegerType::createSigned(32)->getAlias("int"),
	IntegerType::createSigned(16)->getAlias("short"),
	IntegerType::createUnsigned(32)->getAlias("unsigned"),
    };
    auto fn = FunctionType::create(ret, std::move(param), true);
    auto fnAlias = fn->getAlias("foo");

    std::cerr << "Some function type\n";
    std::cerr << "  fn = " << fn << "\n";
    std::cerr << "  fnAlias = " << fnAlias << "\n";
}

int
main()
{
    intExample(true, 32, "int");
    intExample(true, 32, "int");
    intExample(false, 32, "unsigned");

    floatExample("f32");
    doubleExample("f64");

    fnExample();
    fnExample();
    fnExample();
}
