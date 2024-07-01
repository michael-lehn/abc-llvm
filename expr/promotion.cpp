#include <iostream>

#include "lexer/error.hpp"
#include "type/arraytype.hpp"
#include "type/integertype.hpp"
#include "type/floattype.hpp"
#include "type/pointertype.hpp"

#include "implicitcast.hpp"
#include "promotion.hpp"

namespace abc { namespace promotion {

/*
 * Rules for call expressions
 */
CallResult
call(ExprPtr &&fn, std::vector<ExprPtr> &&arg, lexer::Loc *loc)
{
    auto fnType = fn->type;
    if (!fnType->isFunction()) {
	if (loc) {
	    error::location(fn->loc);
	    error::out() << fn->loc
		<< error::setColor(error::BOLD_RED) << ": "
		<< error::setColor(error::BOLD)
		<< "error: Not a function or function pointer."
		<< " Operand has type '" << fn->type << "'\n"
		<< error::setColor(error::NORMAL);
	    error::fatal();
	}
	return std::make_tuple(std::move(fn), std::move(arg), nullptr);
    }
    bool hasVarg = fnType->hasVarg();
    const auto &paramType = fnType->paramType();

    if (arg.size() < paramType.size()) {
	if (loc) {
	    error::location(fn->loc);
	    error::out() << fn->loc
		<< error::setColor(error::BOLD_RED) << ": "
		<< error::setColor(error::BOLD)
		<< "error: too few arguments to function\n"
		<< error::setColor(error::NORMAL);
	    error::fatal();
	}
    } else if (!hasVarg && arg.size() > paramType.size()) {
	if (loc) {
	    error::location(fn->loc);
	    error::out() << fn->loc
		<< error::setColor(error::BOLD_RED) << ": "
		<< error::setColor(error::BOLD)
		<< "error: too many arguments to function\n"
		<< error::setColor(error::NORMAL);
	    error::fatal();
	}
    }

    for (std::size_t i = 0; i < arg.size(); ++i) {
	if (i < paramType.size()) {
	    arg[i] = ImplicitCast::create(std::move(arg[i]), paramType[i]);
	} else {
	    // Rules for converting vargs:
	    // - If an array is passed as varg it will always converted to a
	    //	 pointer (required to interface with C)
	    // - float is casted to double	 
	    if (arg[i]->type->isArray()) {
		auto argType = PointerType::create(arg[i]->type->refType());
		arg[i] = ImplicitCast::create(std::move(arg[i]), argType);
	    } else if (arg[i]->type->isFloat()) {
		auto argType = FloatType::createDouble();
		arg[i] = ImplicitCast::create(std::move(arg[i]), argType);
	    }
	}
    }
    


    return std::make_tuple(std::move(fn), std::move(arg), fn->type->retType());
}

/*
 * Rules for binary expressions
 */
static BinaryResult binaryErr(BinaryExpr::Kind kind, ExprPtr &&left,
			      ExprPtr &&right, lexer::Loc *loc);
static BinaryResult binaryInt(BinaryExpr::Kind kind, ExprPtr &&left,
			      ExprPtr &&right, lexer::Loc *loc);
static BinaryResult binaryFlt(BinaryExpr::Kind kind, ExprPtr &&left,
			      ExprPtr &&right, lexer::Loc *loc);
static BinaryResult binaryPtr(BinaryExpr::Kind kind, ExprPtr &&left,
			      ExprPtr &&right, lexer::Loc *loc);
static BinaryResult binaryArray(BinaryExpr::Kind kind, ExprPtr &&left,
			        ExprPtr &&right, lexer::Loc *loc);
static BinaryResult binaryStruct(BinaryExpr::Kind kind, ExprPtr &&left,
				 ExprPtr &&right, lexer::Loc *loc);

BinaryResult
binary(BinaryExpr::Kind kind, ExprPtr &&left, ExprPtr &&right, lexer::Loc *loc)
{
    if (left->type->isStruct() || right->type->isStruct()) {
	return binaryStruct(kind, std::move(left), std::move(right), loc);
    } else if (left->type->isArray() || right->type->isArray()) {
	return binaryArray(kind, std::move(left), std::move(right), loc);
    } else if (left->type->isPointer() || right->type->isPointer()) {
	return binaryPtr(kind, std::move(left), std::move(right), loc);
    } else if (left->type->isFloatType() || right->type->isFloatType()) {
	return binaryFlt(kind, std::move(left), std::move(right), loc);
    } else if (left->type->isInteger() && right->type->isInteger()) {
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
	error::location(*loc);
	error::out() << error::setColor(error::BOLD) << *loc
	    << ": " << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "operator can not be applied to operand '"
	    << left << "' of type '" << left->type
	    << "' and  operand '" << right << "' of type '" << right->type
	    << "'\n"
	    << error::setColor(error::NORMAL);
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

    // when mixing signed and unsigned: unsigned wins 
    auto commonType = Type::common(left->type, right->type);


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
	case BinaryExpr::Kind::BITWISE_AND_ASSIGN:
	case BinaryExpr::Kind::BITWISE_OR_ASSIGN:
	case BinaryExpr::Kind::BITWISE_XOR_ASSIGN:
	case BinaryExpr::Kind::BITWISE_LEFT_SHIFT_ASSIGN:
	case BinaryExpr::Kind::BITWISE_RIGHT_SHIFT_ASSIGN:
	    if (!Type::assignable(left->type)) {
		error::out() << error::setColor(error::BOLD) << left->loc
		    << ": " << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << ": assignment of read-only variable '"
		    << left << "'\n"
		    << error::setColor(error::NORMAL);
	    } else if (!left->isLValue()) {
		error::out() << error::setColor(error::BOLD) << left->loc
		    << ": " << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << " not an LValue\n"
		    << error::setColor(error::NORMAL);
	    } else {
		type = newLeftType = left->type;
		if (kind != BinaryExpr::Kind::BITWISE_LEFT_SHIFT_ASSIGN
		 && kind != BinaryExpr::Kind::BITWISE_RIGHT_SHIFT_ASSIGN)
		{
		    newRightType = left->type;
		} else {
		    newRightType = right->type;
		}
	    }
	    break;
	case BinaryExpr::Kind::ADD:
	case BinaryExpr::Kind::SUB:
	case BinaryExpr::Kind::MUL:
	case BinaryExpr::Kind::DIV:
	case BinaryExpr::Kind::MOD:
	case BinaryExpr::Kind::BITWISE_AND:
	case BinaryExpr::Kind::BITWISE_OR:
	case BinaryExpr::Kind::BITWISE_XOR:
	    type = newLeftType = newRightType = commonType;
	    break;
	case BinaryExpr::Kind::BITWISE_LEFT_SHIFT:
	case BinaryExpr::Kind::BITWISE_RIGHT_SHIFT:
	    type = newLeftType = left->type;
	    newRightType = right->type;
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

static BinaryResult
binaryFlt(BinaryExpr::Kind kind, ExprPtr &&left, ExprPtr &&right,
	  lexer::Loc *loc)
{
    assert(left->type->isInteger() || left->type->isFloatType());
    assert(right->type->isInteger() || right->type->isFloatType());

    // when mixing integer and floating point: floating point wins
    auto commonType = Type::common(left->type, right->type);

    const Type *type = nullptr;
    const Type *newLeftType = nullptr;
    const Type *newRightType = nullptr;

    switch (kind) {
	case BinaryExpr::Kind::ASSIGN:
	case BinaryExpr::Kind::ADD_ASSIGN:
	case BinaryExpr::Kind::SUB_ASSIGN:
	case BinaryExpr::Kind::MUL_ASSIGN:
	case BinaryExpr::Kind::DIV_ASSIGN:
	    if (!Type::assignable(left->type)) {
		error::out() << error::setColor(error::BOLD) << left->loc
		    << ": " << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << ": assignment of read-only variable '"
		    << left << "'\n"
		    << error::setColor(error::NORMAL);
	    } else if (!left->isLValue()) {
		error::out() << error::setColor(error::BOLD) << left->loc
		    << ": " << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << " not an LValue\n"
		    << error::setColor(error::NORMAL);
	    } else {
		type = newLeftType = newRightType = left->type;
	    }
	    break;
	case BinaryExpr::Kind::ADD:
	case BinaryExpr::Kind::SUB:
	case BinaryExpr::Kind::MUL:
	case BinaryExpr::Kind::DIV:
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

static BinaryResult
binaryPtr(BinaryExpr::Kind kind, ExprPtr &&left, ExprPtr &&right,
	  lexer::Loc *loc)
{
    if (kind == BinaryExpr::ADD && !left->type->isPointer()) {
	left.swap(right);
    }
    const Type *type = nullptr;
    const Type *newLeftType = nullptr;
    const Type *newRightType = nullptr;

    switch (kind) {
	case BinaryExpr::ASSIGN:
	    if (!Type::assignable(left->type)) {
		error::out() << error::setColor(error::BOLD) << left->loc
		    << ": " << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << "assignment of read-only variable '"
		    << left << "'\n"
		    << error::setColor(error::NORMAL);
		break;
	    } else if (!left->isLValue()) {
		error::out() << error::setColor(error::BOLD) << left->loc
		    << ": " << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << "not an LValue\n"
		    << error::setColor(error::NORMAL);
	    } else {
		type = newLeftType = left->type;
		newRightType = Type::convert(right->type, type);
	    }
	    break;
	case BinaryExpr::Kind::INDEX:
	    if (!right->type->isInteger()) {
		error::out() << error::setColor(error::BOLD) << right->loc 
		    << ": " << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << "integer expression expected\n"
		    << error::setColor(error::NORMAL);
		error::fatal();
		return binaryErr(kind, std::move(left), std::move(right), loc);
	    } else if (!left->type->isNullptr()) {
		auto elementType = left->type->refType();

		right = ImplicitCast::create(std::move(right),
					    IntegerType::createSizeType());
		return std::make_tuple(std::move(left), std::move(right),
				       elementType);
	    } else {
		error::out() << error::setColor(error::BOLD) << left->loc
		    << ": " << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << ": dereferencing nullptr\n"
		    << error::setColor(error::NORMAL);
		error::fatal();
		return binaryErr(kind, std::move(left), std::move(right), loc);
	    }
	case BinaryExpr::Kind::ADD_ASSIGN:
	    if (!Type::assignable(left->type)) {
		error::out() << error::setColor(error::BOLD) << left->loc
		    << ": " << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << "assignment of read-only variable '"
		    << left << "'\n"
		    << error::setColor(error::NORMAL);
		break;
	    } else if (right->type->isInteger()) {
		if (!left->isLValue()) {
		    error::out() << error::setColor(error::BOLD) << left->loc
			<< ": " << error::setColor(error::BOLD_RED) << "error: "
			<< error::setColor(error::BOLD)
			<< ": not an LValue\n"
			<< error::setColor(error::NORMAL);
		} else {
		    type = newLeftType = left->type;
		    newRightType = right->type;
		}
	    }
	    break;
	case BinaryExpr::Kind::SUB_ASSIGN:
	    if (!Type::assignable(left->type)) {
		error::out() << error::setColor(error::BOLD) << left->loc
		    << ": " << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << "assignment of read-only variable '"
		    << left << "'\n"
		    << error::setColor(error::NORMAL);
		break;
	    } else if (right->type->isPointer()) {
		if (!left->isLValue()) {
		    error::out() << error::setColor(error::BOLD) << left->loc
			<< ": " << error::setColor(error::BOLD_RED) << "error: "
			<< error::setColor(error::BOLD)
			<< "not an LValue\n"
			<< error::setColor(error::NORMAL);
		} else {
		    type = IntegerType::createSigned(64);
		    newLeftType = left->type;
		    newRightType = right->type;
		}
	    }
	    break;
	case BinaryExpr::ADD:
	    if (right->type->isInteger()) {
		type = newLeftType = left->type;
		newRightType = right->type;
	    }
	    break;
	case BinaryExpr::SUB:
	    if (left->type->isPointer() && right->type->isPointer()) {
		type = IntegerType::createSigned(64);
		newLeftType = newRightType
		    = Type::common(left->type, right->type);
	    } else if (left->type->isPointer() && right->type->isInteger()) {
		type = newLeftType = left->type;
		newRightType = right->type;
	    }
	    break;
	case BinaryExpr::Kind::EQUAL:
	case BinaryExpr::Kind::NOT_EQUAL:
	case BinaryExpr::Kind::GREATER:
	case BinaryExpr::Kind::GREATER_EQUAL:
	case BinaryExpr::Kind::LESS:
	case BinaryExpr::Kind::LESS_EQUAL:
	    {
		type = IntegerType::createBool();
		newLeftType = newRightType
		    = Type::common(left->type, right->type);
	    }
	    break;
	case BinaryExpr::Kind::LOGICAL_AND:
	case BinaryExpr::Kind::LOGICAL_OR:
	    {
		type = newLeftType = newRightType = IntegerType::createBool();
	    }
	    break;
	default:
	    ;
    }
    if (!type || !newLeftType || !newRightType) { 
	return binaryErr(kind, std::move(left), std::move(right), loc);
    }
    left = ImplicitCast::create(std::move(left), newLeftType);
    right = ImplicitCast::create(std::move(right), newRightType);
    return std::make_tuple(std::move(left), std::move(right), type);
}

static BinaryResult
binaryArray(BinaryExpr::Kind kind, ExprPtr &&left, ExprPtr &&right,
	    lexer::Loc *loc)
{
    //
    // Like in C: Arrays in an expression are treated as pointers.
    // One exception: We do allow array assignments if types are equal!
    //
    switch (kind) {
	case BinaryExpr::Kind::INDEX:
	    if (!right->type->isInteger()) {
		error::out() << error::setColor(error::BOLD) << right->loc
		    << ": " << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << ": integer expression expected\n"
		    << error::setColor(error::NORMAL);
		error::fatal();
		return binaryErr(kind, std::move(left), std::move(right), loc);
	    } else {
		auto elementType = left->type->refType();
		right = ImplicitCast::create(std::move(right),
					    IntegerType::createSizeType());
		return std::make_tuple(std::move(left), std::move(right),
				       elementType);
	    }
	case BinaryExpr::Kind::ASSIGN:
	    if (!Type::assignable(left->type)) {
		error::location(left->loc);
		error::out() << error::setColor(error::BOLD) << left->loc
		    << ": " << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << "assignment of read-only variable '"
		    << left << "'\n"
		    << error::setColor(error::NORMAL);
		break;
	    } else if (!left->isLValue()) {
		error::location(left->loc);
		error::out() << error::setColor(error::BOLD) << left->loc
		    << ": " << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << "not an LValue\n"
		    << error::setColor(error::NORMAL);
		break;
	    } else {
		right = ImplicitCast::create(std::move(right), left->type);
		return std::make_tuple(std::move(left), std::move(right),
				       left->type);	
	    }
	default:
	    if (left->type->isArray()) {
		auto newLeftType = PointerType::create(left->type->refType());
		left = ImplicitCast::create(std::move(left), newLeftType);
	    }
	    if (right->type->isArray()) {
		auto newRightType = PointerType::create(right->type->refType());
		right = ImplicitCast::create(std::move(right), newRightType);
	    }
	    return binary(kind, std::move(left), std::move(right), loc);
    }
    return binaryErr(kind, std::move(left), std::move(right), loc);
}

static BinaryResult
binaryStruct(BinaryExpr::Kind kind, ExprPtr &&left, ExprPtr &&right,
	     lexer::Loc *loc)
{
    const Type *newRightType = nullptr;

    switch (kind) {
	default:
	    break;
	case BinaryExpr::Kind::ASSIGN:
	    if (!Type::assignable(left->type)) {
		error::location(left->loc);
		error::out() << error::setColor(error::BOLD) << left->loc
		    << ": " << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << "assignment of read-only variable '"
		    << left << "'\n"
		    << error::setColor(error::NORMAL);
	    } else if (!left->isLValue()) {
		error::location(left->loc);
		error::out() << error::setColor(error::BOLD) << left->loc
		    << ": " << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << "not an LValue\n"
		    << error::setColor(error::NORMAL);
	    } else {
		newRightType = left->type;
	    }
	    break;
    }
    if (!newRightType) { 
	return binaryErr(kind, std::move(left), std::move(right), loc);
    }
    right = ImplicitCast::create(std::move(right), newRightType);
    return std::make_tuple(std::move(left), std::move(right), left->type);
}

/*
 * Rules for unary expressions
 */

using UnaryResult = std::pair<ExprPtr, const Type *>;

static UnaryResult unaryErr(UnaryExpr::Kind kind, ExprPtr &&child,
			    lexer::Loc *loc);

UnaryResult
unary(UnaryExpr::Kind kind, ExprPtr &&child, lexer::Loc *loc)
{
    const Type *type = nullptr;
    const Type *newChildType = nullptr;
    switch (kind) {
	default:
	    break;
	case UnaryExpr::ADDRESS:
	    if (child->hasAddress()) {
		type = PointerType::create(child->type);
		newChildType = child->type;
	    }
	    break;
	case UnaryExpr::ASTERISK_DEREF:
	case UnaryExpr::ARROW_DEREF:
	    if (child->type->isPointer() && !child->type->isNullptr()) {
		type = child->type->refType();
		newChildType = child->type;
	    }
	    break;
	case UnaryExpr::PREFIX_INC:
	case UnaryExpr::POSTFIX_INC:
	    if (child->type->hasConstFlag()) {
		error::location(child->loc);
		error::out() << error::setColor(error::BOLD) << child->loc
		    << ": " << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << "increment of read-only variable '" << child << "'\n"
		    << error::setColor(error::NORMAL);
		break;
	    }
	    // fall through
	case UnaryExpr::PREFIX_DEC:
	case UnaryExpr::POSTFIX_DEC:
	    if (child->type->hasConstFlag()) {
		error::location(child->loc);
		error::out() << error::setColor(error::BOLD) << child->loc
		    << ": " << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << "decrement of read-only variable '" << child << "'\n"
		    << error::setColor(error::NORMAL);
		break;
	    }
	    if (child->isLValue()) {
		if (child->type->isInteger() || child->type->isPointer()) {
		    type = newChildType = child->type;
		}
	    }
	    break;
	case UnaryExpr::LOGICAL_NOT:
	    if (child->type->isInteger() || child->type->isPointer()) {
		newChildType = child->type;
		type = IntegerType::createBool();
	    }
	    break;
	case UnaryExpr::MINUS:
	    if (child->type->isInteger() || child->type->isFloatType()) {
		type = newChildType = child->type;
	    }
	    break;
    }
    if (!type || !newChildType) { 
	return unaryErr(kind, std::move(child), loc);
    }
    child = ImplicitCast::create(std::move(child), newChildType);
    return std::make_pair(std::move(child), type);
    
}

static UnaryResult
unaryErr(UnaryExpr::Kind kind, ExprPtr &&child, lexer::Loc *loc)
{
    if (loc) {
	error::location(*loc);
	error::out() << error::setColor(error::BOLD) << *loc << ": "
	    << ": " << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "operator can not be applied to operand " << child
	    << " of type '" << child->type << "'\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
    }
    return std::make_pair(std::move(child), nullptr);
}

} } // namespace promotion, abc
