#include <iostream>

#include "expr/integerliteral.hpp"
#include "lexer/loc.hpp"
#include "type/integertype.hpp"

#include "symtab.hpp"

const char *
getEntryKind(const abc::symtab::Entry *entry)
{
    if (entry->typeDeclaration()) {
	return "Type Declaration";
    } else if (entry->variableDeclaration()) {
	return "Variable Declaration";
    } else {
	return "Constant Declaration";
    }
}

void
addVar(const char *name)
{
    using namespace abc;
    auto addDecl = Symtab::addDeclaration(lexer::Loc{},
					  UStr::create(name),
					  IntegerType::createSigned(32));

    std::cerr << "add: var = " << name << "\n";
    std::cerr << "added = " << addDecl.second << "\n";
    std::cerr << "id = " << addDecl.first->getId() << "\n";
    std::cerr << "type = " << addDecl.first->type << "\n";
    std::cerr << "kind = " << getEntryKind(addDecl.first) << "\n";
    std::cerr << "\n";
}

void
addType(const char *name)
{
    using namespace abc;
    auto addDecl = Symtab::addType(lexer::Loc{},
				   UStr::create(name),
				   IntegerType::createSigned(32));

    std::cerr << "add: type = " << name << "\n";
    std::cerr << "added = " << addDecl.second << "\n";
    std::cerr << "id = " << addDecl.first->getId() << "\n";
    std::cerr << "type = " << addDecl.first->type << "\n";
    std::cerr << "kind = " << getEntryKind(addDecl.first) << "\n";
    std::cerr << "\n";
}

void
addExpr(const char *name, const abc::Expr *expr)
{
    using namespace abc;
    auto addDecl = Symtab::addExpression(lexer::Loc{},
					 UStr::create(name),
					 expr);

    std::cerr << "add: type = " << name << "\n";
    std::cerr << "added = " << addDecl.second << "\n";
    std::cerr << "id = " << addDecl.first->getId() << "\n";
    std::cerr << "expr = " << addDecl.first->expr << "\n";
    std::cerr << "kind = " << getEntryKind(addDecl.first) << "\n";
    std::cerr << "\n";
}

int
main()
{
    abc::Symtab guard;
    addVar("foo");
    addVar("foo");
    addType("int");
    addVar("int");

    auto someExpr = abc::IntegerLiteral::create(42);
    addExpr("FOO", someExpr.get());

    abc::Symtab::print(std::cerr);
}
