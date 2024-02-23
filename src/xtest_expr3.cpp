#include "binaryexpr.hpp"
#include "castexpr.hpp"
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
    gen::init();
    Symtab::openScope();

    /*
     * Create some code from an expression
     */

    auto lit42 = IntegerLiteral::create(42);
    lit42->print();
    Symtab::addDecl(Token::Loc{},
		    UStr::create("a"),
		    Type::getUnsignedInteger(64));
    auto a = Identifier::create(UStr::create("a"));
    a->print();
    auto proxy_a = ProxyExpr::create(a.get());
    auto cast_a = CastExpr::create(std::move(a), Type::getUnsignedInteger(16));
    cast_a->print();
    auto not_cast_a = UnaryExpr::create(UnaryExpr::LOGICAL_NOT,
					std::move(cast_a));
    not_cast_a->print();
    auto binary = BinaryExpr::create(BinaryExpr::Kind::ADD,
				     std::move(not_cast_a),
				     std::move(lit42));
    binary->print();
    binary = BinaryExpr::create(BinaryExpr::Kind::SUB,
				 std::move(binary),
				 std::move(proxy_a));
    binary->print();

    auto str = StringLiteral::create(UStr::create("hello, world!"),
				     UStr::create("hello, world!"));
    str->print();

    /*
    auto a = Expr::createIdentifier("a");
    auto b = Expr::createIdentifier("b");
    auto binary = Expr::createBinary(
			Binary::Kind::ADD,
			Expr::createBinary(
			    Binary::Kind::SUB,
			    std::move(lit42),
			    std::move(a)),
			Expr::createUnaryMinus(
			    std::move(b)));

    // for us to see what the expression represents
    binary->print();
    */
    Symtab::closeScope();
}

