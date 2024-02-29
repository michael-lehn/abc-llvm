#include <iostream>

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
makeMainDecl()
{
    auto fnName = makeIdentifierToken("main");
    auto fnRetType = abc::IntegerType::createSigned(8);
    auto fnArgType = std::vector<const abc::Type *>{};
    auto fnType = abc::FunctionType::create(fnRetType,
					    std::move(fnArgType),
					    false);
    auto fnArgName = std::vector<abc::lexer::Token>{};
    auto fnMainDecl = std::make_unique<abc::AstFuncDecl>(fnName,
							 fnType,
							 std::move(fnArgName),
							 true);
    return fnMainDecl;
}

auto
makeMainDef()
{
    auto fnName = makeIdentifierToken("main");
    auto fnRetType = abc::IntegerType::createSigned(8);
    auto fnArgType = std::vector<const abc::Type *>{};
    auto fnType = abc::FunctionType::create(fnRetType,
					    std::move(fnArgType),
					    false);
    auto fnArgName = std::vector<abc::lexer::Token>{};
    auto fnMainDef = std::make_unique<abc::AstFuncDef>(fnName,
						       fnType,
						       std::move(fnArgName));
    return fnMainDef;
}

int
main()
{
    gen::init();
    abc::Symtab guard;

    auto topLevel = std::make_unique<abc::AstList>();
    topLevel->append(makeMainDecl());
    topLevel->append(makeMainDef());
    topLevel->print();
    topLevel->codegen();

    gen::printGlobalVariableList();
    gen::printFunctionList();
    std::cerr << "generating hello.bc\n";
    gen::print("hello");
}
