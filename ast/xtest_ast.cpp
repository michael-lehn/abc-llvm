#include <iostream>

#include "expr/binaryexpr.hpp"
#include "expr/identifier.hpp"
#include "expr/integerliteral.hpp"
#include "gen/gen.hpp"
#include "gen/print.hpp"
#include "gen/variable.hpp"
#include "lexer/token.hpp"
#include "symtab/symtab.hpp"
#include "type/functiontype.hpp"
#include "type/integertype.hpp"

#include "ast.hpp"

auto
makeIdentifierToken(const char *name)
{
    return abc::lexer::Token{abc::lexer::Loc{},
			     abc::lexer::TokenKind::IDENTIFIER,
			     abc::UStr::create(name)};
}

auto
makeExternVarDecl()
{
    auto varDeclList = std::make_unique<abc::AstList>();

    auto varName = makeIdentifierToken("foo");
    auto varType = abc::IntegerType::createSigned(32);
    auto varDecl = std::make_unique<abc::AstVar>(varName, abc::lexer::Loc{},
						 varType);
    varDeclList->append(std::move(varDecl));
    return std::make_unique<abc::AstExternVar>(std::move(varDeclList));
}

auto
makeMainDecl()
{
    auto intType = abc::IntegerType::createSigned(32)->getAlias("int");

    auto fnName = makeIdentifierToken("main");
    auto fnRetType = intType;
    auto fnParamType = std::vector<const abc::Type *>{intType};
    auto fnType = abc::FunctionType::create(fnRetType,
					    std::move(fnParamType),
					    false);
    auto fnParamName = std::vector<abc::lexer::Token>{};
    auto fnMainDecl = std::make_unique<abc::AstFuncDecl>(fnName,
							 fnType,
							 std::move(fnParamName),
							 true);
    return fnMainDecl;
}

auto
makeExpr()
{
    auto name = abc::UStr::create("argc");
    auto entry = abc::Symtab::find(name, abc::Symtab::AnyScope);
    abc::Symtab::print(std::cerr);
    if (!entry) {
	std::cerr << "Undeclared identifier " << name << "\n";
	assert(0);
    }
    std::cerr << "argcExpr with id = " << entry->id << "\n";
    auto argcExpr = abc::Identifier::create(name, entry->id, entry->type);

    auto intType = abc::IntegerType::createSigned(32)->getAlias("int");
    auto intExpr = abc::IntegerLiteral::create(-1, intType);

    auto addExpr = abc::BinaryExpr::create(abc::BinaryExpr::ADD,
					   std::move(argcExpr),
					   std::move(intExpr));
    return addExpr; 
}

auto
makeMainDef()
{
    auto intType = abc::IntegerType::createSigned(32)->getAlias("int");

    auto fnName = makeIdentifierToken("main");
    auto fnRetType = intType;
    auto fnParamType = std::vector<const abc::Type *>{intType};
    auto fnType = abc::FunctionType::create(fnRetType,
					    std::move(fnParamType),
					    false);

    auto fnMainDef = std::make_unique<abc::AstFuncDef>(fnName, fnType);
    abc::Symtab guard(abc::UStr::create("main"));

    fnMainDef->appendParamName({makeIdentifierToken("argc")});


    auto body = std::make_unique<abc::AstList>();
    body->append(std::make_unique<abc::AstReturn>(abc::lexer::Loc{},
						  makeExpr()));
    fnMainDef->appendBody(std::move(body));

    return fnMainDef;
}

int
main()
{
    gen::init();
    abc::Symtab guard;

    auto topLevel = std::make_unique<abc::AstList>();
    topLevel->append(makeExternVarDecl());
    topLevel->append(makeMainDecl());
    topLevel->append(makeMainDef());

    std::cerr << "AST:\n\n";
    topLevel->print();
    topLevel->codegen();

    std::cerr << "\n";
    gen::printGlobalVariableList();
    std::cerr << "Functions in code generator:\n";
    gen::printFunctionList();
    std::cerr << "generating hello.bc\n";
    gen::print("hello");
}
