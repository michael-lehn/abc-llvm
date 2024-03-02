#include <algorithm>
#include <iostream>

#include "lexer/error.hpp"
#include "type/integertype.hpp"

#include "implicitcast.hpp"
#include "promotion.hpp"

namespace abc { namespace promotion {

/*
 * Rules for call expressions
 */
CallResult
call(ExprPtr &&fn, std::vector<ExprPtr> &&arg, lexer::Loc *loc)
{
    return std::make_tuple(std::move(fn), std::move(arg), fn->type->retType());
}

/*
 * Rules for binary expressions
 */
static BinaryResult binaryErr(BinaryExpr::Kind kind, ExprPtr &&left,
			      ExprPtr &&right, lexer::Loc *loc);
static BinaryResult binaryInt(BinaryExpr::Kind kind, ExprPtr &&left,
			      ExprPtr &&right, lexer::Loc *loc);

BinaryResult
binary(BinaryExpr::Kind kind, ExprPtr &&left, ExprPtr &&right, lexer::Loc *loc)
{
    if (left->type->isInteger() && right->type->isInteger()) {
	return binaryInt(kind, std::move(left), std::move(right), loc);
    } else {
	return binaryErr(kind, std::move(left), std::move(right), loc);
    }
}

static BinaryResult
binaryErr(BinaryExpr::Kind kind, ExprPtr &&left, ExprPtr &&right,
	  lexer::Loc *loc)
{
    if (loc) {
	error::out() << *loc << ": operator can not be applied to"
	    << left->loc << " left operand of type '" << left->type
	    << "' and '"
	    << right->loc << " right operand of type '" << right->type
	    << "'" << std::endl;
	error::fatal();
    }
    return std::make_tuple(std::move(left), std::move(right), nullptr);
}

static BinaryResult
binaryInt(BinaryExpr::Kind kind, ExprPtr &&left, ExprPtr &&right,
	  lexer::Loc *loc)
{
    assert(left->type->isInteger());
    assert(right->type->isInteger());
    auto size = std::max(left->type->numBits(), right->type->numBits());

    // when mixing signed and unsigned: unsigned wins 
    auto commonType = left->type->isUnsignedInteger()
		   || right->type->isUnsignedInteger()
	? IntegerType::createUnsigned(size)
	: IntegerType::createSigned(size);
    if (left->type->hasConstFlag() || right->type->hasConstFlag()) {
	commonType = commonType->getConst();
    }

    const Type *type = nullptr;
    const Type *newLeftType = nullptr;
    const Type *newRightType = nullptr;

    switch (kind) {
	case BinaryExpr::Kind::ASSIGN:
	case BinaryExpr::Kind::ADD_ASSIGN:
	case BinaryExpr::Kind::SUB_ASSIGN:
	case BinaryExpr::Kind::MUL_ASSIGN:
	case BinaryExpr::Kind::DIV_ASSIGN:
	case BinaryExpr::Kind::MOD_ASSIGN:
	    if (left->isLValue()) {
		type = newLeftType = newRightType = left->type;
	    }
	    break;
	case BinaryExpr::Kind::ADD:
	case BinaryExpr::Kind::SUB:
	case BinaryExpr::Kind::MUL:
	case BinaryExpr::Kind::DIV:
	case BinaryExpr::Kind::MOD:
	    type = newLeftType = newRightType = commonType;
	    break;
	case BinaryExpr::Kind::EQUAL:
	case BinaryExpr::Kind::NOT_EQUAL:
	case BinaryExpr::Kind::GREATER:
	case BinaryExpr::Kind::GREATER_EQUAL:
	case BinaryExpr::Kind::LESS:
	case BinaryExpr::Kind::LESS_EQUAL:
	    type = IntegerType::createBool();
	    newLeftType = newRightType = commonType;
	    break;
	case BinaryExpr::Kind::LOGICAL_AND:
	case BinaryExpr::Kind::LOGICAL_OR:
	    type = newLeftType = newRightType = IntegerType::createBool();
	    break;
	default:
	    type = nullptr;
	    break;
    }
    if (!type || !newLeftType || !newRightType) { 
	return binaryErr(kind, std::move(left), std::move(right), loc);
    }
    left = ImplicitCast::create(std::move(left), newLeftType);
    right = ImplicitCast::create(std::move(right), newRightType);
    return std::make_tuple(std::move(left), std::move(right), type);
}

} } // namespace promotion, abc
