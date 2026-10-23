// TEST: T0154
#include <algorithm>
#include <string>

#include "lexer/loc.hpp"
#include "symtab/symtab.hpp"
#include "type/arraytype.hpp"
#include "type/floattype.hpp"
#include "type/inittypesystem.hpp"
#include "type/integertype.hpp"

#include "check.hpp"

using namespace abc;

static UStr
str(const char *s)
{
    return UStr::create(s);
}

static std::string
id(const symtab::Entry *entry)
{
    return entry ? entry->getId().c_str() : "<no entry>";
}

//------------------------------------------------------------------------------

static void
testLookupByKind()
{
    Symtab root;
    auto i32 = IntegerType::createSigned(32);

    auto var = Symtab::addDeclaration(lexer::Loc{}, str("x"), i32);
    CHECK(var.first && var.second);
    auto type = Symtab::addType(lexer::Loc{}, str("T"), i32);
    CHECK(type.first && type.second);

    CHECK(Symtab::find(str("x"), Symtab::AnyScope) == var.first);
    CHECK(Symtab::variable(str("x"), Symtab::AnyScope) == var.first);
    CHECK(!Symtab::type(str("x"), Symtab::AnyScope));
    CHECK(!Symtab::constant(str("x"), Symtab::AnyScope));

    CHECK(Symtab::type(str("T"), Symtab::AnyScope) == type.first);
    CHECK(!Symtab::variable(str("T"), Symtab::AnyScope));

    CHECK(!Symtab::find(str("y"), Symtab::AnyScope));
    // names in the root scope are not decorated
    CHECK_EQ(id(var.first), "x");
}

static void
testScopesAndShadowing()
{
    Symtab root;
    auto i32 = IntegerType::createSigned(32);
    auto dbl = FloatType::createDouble();

    auto global = Symtab::addDeclaration(lexer::Loc{}, str("x"), i32).first;
    {
	// function scope (requires a prefix used for unique names)
	Symtab fnScope(str("f"));
	CHECK(Symtab::find(str("x"), Symtab::AnyScope) == global);
	CHECK(!Symtab::find(str("x"), Symtab::CurrentScope));

	auto local = Symtab::addDeclaration(lexer::Loc{}, str("x"), dbl);
	CHECK(local.second);
	CHECK(local.first != global);
	CHECK(Symtab::find(str("x"), Symtab::AnyScope) == local.first);
	CHECK(Symtab::find(str("x"), Symtab::CurrentScope)->type == dbl);
	{
	    // nested block
	    Symtab block;
	    CHECK(Symtab::find(str("x"), Symtab::AnyScope) == local.first);
	    auto inner = Symtab::addDeclaration(lexer::Loc{}, str("x"), i32);
	    CHECK(Symtab::find(str("x"), Symtab::AnyScope) == inner.first);
	    // locals get unique names derived from the function name
	    CHECK(id(local.first) != id(inner.first));
	    CHECK(id(inner.first).find("f") != std::string::npos);
	}
	// leaving the block makes the shadowed declaration visible again
	CHECK(Symtab::find(str("x"), Symtab::AnyScope) == local.first);
    }
    CHECK(Symtab::find(str("x"), Symtab::AnyScope) == global);
}

static void
testRedeclaration()
{
    Symtab root;
    auto i32 = IntegerType::createSigned(32);

    // a compatible redeclaration in the same scope returns the existing entry
    auto first = Symtab::addDeclaration(lexer::Loc{}, str("f"), i32);
    auto second = Symtab::addDeclaration(lexer::Loc{}, str("f"), i32);
    CHECK(first.second);
    CHECK(!second.second);
    CHECK(first.first == second.first);

    // an unbound array can be completed by a later declaration
    auto unbound = ArrayType::create(i32, 0);
    auto bound = ArrayType::create(i32, 3);
    auto a1 = Symtab::addDeclaration(lexer::Loc{}, str("a"), unbound);
    auto a2 = Symtab::addDeclaration(lexer::Loc{}, str("a"), bound);
    CHECK(a1.first == a2.first);
    CHECK(a2.second);
    CHECK(a2.first->type == bound);
}

static void
testDidYouMean()
{
    Symtab root;
    auto i32 = IntegerType::createSigned(32);
    Symtab::addDeclaration(lexer::Loc{}, str("counter"), i32);
    Symtab::addDeclaration(lexer::Loc{}, str("unrelated"), i32);
    Symtab::addType(lexer::Loc{}, str("countr"), i32);

    auto list = Symtab::didYouMean(str("countre"));
    auto contains = [&](const char *s) {
	return std::find(list.begin(), list.end(), s) != list.end();
    };
    CHECK(contains("counter"));
    CHECK(!contains("unrelated"));
    // type names are not suggested for identifiers
    CHECK(!contains("countr"));
}

static void
testEntryLinkageAndDefinition()
{
    auto i32 = IntegerType::createSigned(32);

    auto entry = symtab::Entry::createVarEntry(lexer::Loc{}, str("v"), i32);
    CHECK(entry.variableDeclaration());
    CHECK(!entry.hasLinkage());
    CHECK(entry.setDefinitionFlag());
    // a second definition is rejected
    CHECK(!entry.setDefinitionFlag());

    // internal linkage makes the name local to the module
    CHECK(entry.setInternalLinkage());
    CHECK(entry.hasLinkage());
    CHECK_EQ(id(&entry), ".v");
    // external declaration can not follow a static declaration
    CHECK(!entry.setExternalLinkage());

    auto ext = symtab::Entry::createVarEntry(lexer::Loc{}, str("e"), i32);
    CHECK(ext.setExternalLinkage());
    CHECK_EQ(id(&ext), "e");
    // static declaration can not follow an external declaration
    CHECK(!ext.setInternalLinkage());

    auto type = symtab::Entry::createTypeEntry(lexer::Loc{}, str("T"), i32);
    CHECK(type.typeDeclaration() && !type.variableDeclaration());
}

int
main(int argc, char *argv[])
{
    initTypeSystem();
    testLookupByKind();
    testScopesAndShadowing();
    testRedeclaration();
    testDidYouMean();
    testEntryLinkageAndDefinition();
    return check::summary(argv[0]);
}
