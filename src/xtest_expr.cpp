#include <iostream>

#include "binaryexpr.hpp"
#include "castexpr.hpp"
#include "expr.hpp"
#include "gen.hpp"
#include "identifier.hpp"
#include "integerliteral.hpp"
#include "proxyexpr.hpp"
#include "stringliteral.hpp"
#include "symtab.hpp"
#include "type.hpp"
#include "unaryexpr.hpp"

int
main(void)
{
    /*
     * generate function 'foo(a: u64, b: 64): u64'
     */

    // parse function in new scope
    Symtab::openScope();

    // parse parameter list
    std::vector<const char *>	fnParamId = {
	"a",
	"b",
    };
    std::vector<const Type *>	argType = {
	Type::getUnsignedInteger(64),
	Type::getUnsignedInteger(64),
    };
    Symtab::addDecl(Token::Loc{}, fnParamId[0], Type::getUnsignedInteger(64));
    Symtab::addDecl(Token::Loc{}, fnParamId[1], Type::getUnsignedInteger(64));

    // parse return type (and create a variable for it)
    const Type *retType = Type::getUnsignedInteger(64);
    Symtab::addDecl(Token::Loc{}, UStr{".retVal"}, retType);

    // create function type and declare it in root scope
    auto fnType = Type::getFunction(retType, argType);
    auto fnDecl = Symtab::addDeclToRootScope(Token::Loc{}, "foo", fnType);
    if (!fnDecl) {
	assert(0 && "symbol already exists");
    }

    // generate definition (fnParamId just needed for expressive code) 
    gen::fnDef(fnDecl->ident.c_str(), fnDecl->type(), fnParamId, false);
 
    /*
     * Create some code from an expression
     */

    auto lit42 = IntegerLiteral::create("42", 10, nullptr);
    auto a = Identifier::create("a");
    auto b = Identifier::create("b");
    auto binary = BinaryExpr::create(
			BinaryExpr::Kind::ADD,
			BinaryExpr::create(
			    BinaryExpr::Kind::SUB,
			    std::move(lit42),
			    std::move(a)),
			UnaryExpr::create(
			    UnaryExpr::MINUS,
			    std::move(b)));

    // for us to see what the expression represents
    binary->print();
    
    // generate code for the expression
    auto r = binary->loadValue();

    // generate code for returning the value of the expression
    gen::ret(r);

    /*
     * end of function
     */
    gen::fnDefEnd();
    Symtab::closeScope();

    std::cerr << "generating 'expr.bc', 'expr0.s', ..., 'expr3.s'" << std::endl;

    gen::dump_bc("expr");
    gen::dump_asm("expr0", 0);
    gen::dump_asm("expr1", 1);
    gen::dump_asm("expr2", 2);
    gen::dump_asm("expr3", 3);
}
