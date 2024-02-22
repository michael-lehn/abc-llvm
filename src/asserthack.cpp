#include <sstream>

#include "asserthack.hpp"
#include "binaryexpr.hpp"
#include "callexpr.hpp"
#include "integerliteral.hpp"
#include "identifier.hpp"
#include "stringliteral.hpp"
#include "symtab.hpp"

namespace asserthack {

UStr assertIdent = "__assert";

void
makeDecl()
{
    // 
    // msg: -> const char, file: -> const char, line: int
    std::vector<const Type *> argType;
    argType.push_back(Type::getPointer(Type::getConst(Type::getChar())));
    argType.push_back(Type::getPointer(Type::getConst(Type::getChar())));
    argType.push_back(Type::getSignedInteger(8 * sizeof(int)));

    auto fnAssertType = Type::getFunction(Type::getBool(), argType, true);
    gen::fnDecl(assertIdent.c_str(), fnAssertType, true);
    Symtab::addDeclToRootScope(Token::Loc{}, assertIdent, fnAssertType);
}

ExprPtr
createCall(ExprPtr &&expr, Token::Loc loc)
{
	std::stringstream ss;
	ss << expr;

	auto fnAssert = Identifier::create(assertIdent, loc);

	std::vector<ExprPtr> param;
	param.push_back(StringLiteral::create(ss.str(), ss.str(), loc));
	param.push_back(
		StringLiteral::create(token.loc.path, token.loc.path, loc));
	param.push_back(
		IntegerLiteral::create(loc.from.line,
				       Type::getSignedInteger(sizeof(int))));

	return BinaryExpr::create(BinaryExpr::LOGICAL_OR,
				  std::move(expr),
				  CallExpr::create(std::move(fnAssert),
						   std::move(param),
						   loc),
				  loc);
}

} // namespace asserthack
