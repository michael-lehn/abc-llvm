#include <iostream>

#include "callexpr.hpp"
#include "castexpr.hpp"
#include "error.hpp"
#include "promotion.hpp"

namespace promotion {

static const Type *getCommonType(const Type *left, const Type *right,
				 Token::Loc *loc);


/*
 * Rules for call expressions
 */

CallResult
call(ExprPtr &&fn, std::vector<ExprPtr> &&param, Token::Loc *loc)
{
    auto fnType = fn->type->isPointer() ? fn->type->getRefType() : fn->type;
    if (!fnType->isFunction()) {
	if (loc) {
	    error::out() << fn->loc << ": Not a function or function ppointer."
		<< " Operand has type '" << fn->type << "'" << std::endl;
	    error::fatal();
	}
	return std::make_tuple(std::move(fn), std::move(param), nullptr);
    }
    bool hasVarg = fnType->hasVarg();
    const auto &argType = fnType->getArgType();

    if (param.size() < argType.size()) {
	if (loc) {
	    error::out() << fn->loc
		<< ": too few arguments to function" << std::endl;
	    error::fatal();
	}
    } else if (!hasVarg && param.size() > argType.size()) {
	if (loc) {
	    error::out() << fn->loc
		<< ": too many arguments to function" << std::endl;
	    error::fatal();
	}
    }

    for (std::size_t i = 0; i < param.size(); ++i) {
	auto loc = param[i]->loc;
	if (i < argType.size()) {
	    param[i] = CastExpr::create(std::move(param[i]), argType[i], loc);
	} else {
	    auto ty = Type::convertArrayOrFunctionToPointer(param[i]->type);
	    param[i] = CastExpr::create(std::move(param[i]), ty, loc);
	}
    }
    return std::make_tuple(std::move(fn), std::move(param),
			   fnType->getRetType());
}

/*
 * Rules for unary expressions
 */

static UnaryResult unaryErr(UnaryExpr::Kind kind, ExprPtr &&child,
			    Token::Loc *loc);

UnaryResult
unary(UnaryExpr::Kind kind, ExprPtr &&child, Token::Loc *loc)
{
    if (!child->type) {
	return unaryErr(kind, std::move(child), loc);
    }

    const Type *type = nullptr;
    const Type *childType = nullptr;

    switch (kind) {
	case UnaryExpr::ADDRESS:
	    if (child->hasAddr()) {
		type = Type::getPointer(child->type);
		childType = child->type;
	    }
	    break;
	case UnaryExpr::DEREF:
	    if (child->type->isPointer() && !child->type->isNullPointer()) {
		type = child->type->getRefType();
		childType = child->type;
	    }
	    break;
	case UnaryExpr::LOGICAL_NOT:
	    if (child->type->isInteger() || child->type->isPointer()) {
		type = Type::getBool();
		childType = child->type;
	    } else if (loc && child->type->isArray()) {
		type = Type::getBool();
		childType = Type::getPointer(child->type);
		error::out() << *loc
		    << ": Warning: Address of array will "
		    << "always evaluate to 'true'" << std::endl;
		error::warning();
	    } else if (loc && child->type->isFunction()) {
		type = Type::getBool();
		childType = Type::getPointer(child->type);
		error::out() << *loc
		    << ": Warning: Address of function will "
		    << "always evaluate to 'true'" << std::endl;
	    }
	    break;
	case UnaryExpr::POSTFIX_INC:
	case UnaryExpr::POSTFIX_DEC:
	    if (child->type->isInteger() || child->type->isPointer()) {
		type = childType = child->type;
	    }
	    break;
	case UnaryExpr::MINUS:
	    if (child->type->isInteger()) {
		type = childType = child->type;
	    }
	    break;
	default:
	    ;
    }
    if (!type) {
	return unaryErr(kind, std::move(child), loc);
    }
    if (*child->type != *childType) {
	child = CastExpr::create(std::move(child), childType);
    }
    return std::make_pair(std::move(child), type);
}

static UnaryResult
unaryErr(UnaryExpr::Kind kind, ExprPtr &&child, Token::Loc *loc)
{
    if (loc) {
	error::out() << *loc
	    << ": operator can not be applied to operand of type '" 
	    << child->type << "'" << std::endl;
	error::fatal();
    }
    return std::make_pair(std::move(child), nullptr);
}

/*
 * Rules for binary expressions
 */

static BinaryResult binaryErr(BinaryExpr::Kind kind, ExprPtr &&left,
			      ExprPtr &&right, Token::Loc *loc);

static BinaryResult binaryFn(BinaryExpr::Kind kind, ExprPtr &&left,
			     ExprPtr &&right, Token::Loc *loc);

static BinaryResult binaryPtr(BinaryExpr::Kind kind, ExprPtr &&left,
			      ExprPtr &&right, Token::Loc *loc);

static BinaryResult binaryArray(BinaryExpr::Kind kind, ExprPtr &&left,
			        ExprPtr &&right, Token::Loc *loc);

static BinaryResult binaryStruct(BinaryExpr::Kind kind, ExprPtr &&left,
				 ExprPtr &&right, Token::Loc *loc);

static BinaryResult binaryInt(BinaryExpr::Kind kind, ExprPtr &&left,
			      ExprPtr &&right, Token::Loc *loc);

BinaryResult
binary(BinaryExpr::Kind kind, ExprPtr &&left, ExprPtr &&right, Token::Loc *loc)
{
    if (!left->type || !right->type) {
	return binaryErr(kind, std::move(left), std::move(right), loc);
    } else if (left->type->isFunction() || right->type->isFunction()) {
	return binaryFn(kind, std::move(left), std::move(right), loc);
    } else if (left->type->isArray() || right->type->isArray()) {
	return binaryArray(kind, std::move(left), std::move(right), loc);
    } else if (left->type->isPointer() || right->type->isPointer()) {
	return binaryPtr(kind, std::move(left), std::move(right), loc);
    } else if (left->type->isStruct() || right->type->isStruct()) {
	return binaryStruct(kind, std::move(left), std::move(right), loc);
    } else if (left->type->isInteger() || right->type->isInteger()) {
	return binaryInt(kind, std::move(left), std::move(right), loc);
    } else {
	return binaryErr(kind, std::move(left), std::move(right), loc);
    }
}

static BinaryResult
binaryErr(BinaryExpr::Kind kind, ExprPtr &&left, ExprPtr &&right,
	  Token::Loc *loc)
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
binaryFn(BinaryExpr::Kind kind, ExprPtr &&left, ExprPtr &&right,
	 Token::Loc *loc)
{
    if (left->type->isFunction()) {
	left = CastExpr::create(std::move(left),
				Type::getPointer(left->type));
    }
    if (right->type->isFunction()) {
	right = CastExpr::create(std::move(right),
				 Type::getPointer(right->type));
    }
    return binary(kind, std::move(left), std::move(right), loc);
}
 
static BinaryResult
binaryPtr(BinaryExpr::Kind kind, ExprPtr &&left, ExprPtr &&right,
	  Token::Loc *loc)
{
    if (kind == BinaryExpr::ADD && !left->type->isPointer()) {
	left.swap(right);
    }
    const Type *type = nullptr;
    switch (kind) {
	case BinaryExpr::ASSIGN:
	    type = left->type;
	    right = CastExpr::create(std::move(right), type);
	    break;
	case BinaryExpr::ADD:
	    type = right->type->isInteger()
		? left->type
		: nullptr;
	    break;
	case BinaryExpr::SUB:
	    type = right->type->isPointer()
		? Type::getSignedInteger(64)
		: nullptr;
	    break;
	case BinaryExpr::Kind::EQUAL:
	case BinaryExpr::Kind::NOT_EQUAL:
	case BinaryExpr::Kind::GREATER:
	case BinaryExpr::Kind::GREATER_EQUAL:
	case BinaryExpr::Kind::LESS:
	case BinaryExpr::Kind::LESS_EQUAL:
	    {
		type = Type::getBool();
		auto common = getCommonType(left->type, right->type, loc);
		left = CastExpr::create(std::move(left), common);
		right = CastExpr::create(std::move(right), common);
	    }
	    break;
	case BinaryExpr::Kind::LOGICAL_AND:
	case BinaryExpr::Kind::LOGICAL_OR:
	    {
		type = Type::getBool();
		left = CastExpr::create(std::move(left), type);
		right = CastExpr::create(std::move(right), type);
	    }
	    break;
	default:
	    ;
    }
    if (!type) {
	return binaryErr(kind, std::move(left), std::move(right), loc);
    }
    return std::make_tuple(std::move(left), std::move(right), type);
}

static BinaryResult
binaryInt(BinaryExpr::Kind kind, ExprPtr &&left, ExprPtr &&right,
	  Token::Loc *loc)
{
    auto size = std::max(left->type->getIntegerNumBits(),
			 right->type->getIntegerNumBits());

    // when mixing signed and unsigned: unsigned wins 
    auto commonType = left->type->getIntegerKind() == Type::UNSIGNED
		   || right->type->getIntegerKind() == Type::UNSIGNED
	? Type::getUnsignedInteger(size)
	: Type::getSignedInteger(size);

    const Type *type = nullptr;
    const Type *leftType = nullptr;
    const Type *rightType = nullptr;

    switch (kind) {
	case BinaryExpr::Kind::ASSIGN:
	    if (left->isLValue()) {
		type = leftType = rightType = left->type;
	    }
	    break;
	case BinaryExpr::Kind::ADD:
	case BinaryExpr::Kind::SUB:
	case BinaryExpr::Kind::MUL:
	case BinaryExpr::Kind::DIV:
	case BinaryExpr::Kind::MOD:
	    type = leftType = rightType = commonType;
	    break;
	case BinaryExpr::Kind::EQUAL:
	case BinaryExpr::Kind::NOT_EQUAL:
	case BinaryExpr::Kind::GREATER:
	case BinaryExpr::Kind::GREATER_EQUAL:
	case BinaryExpr::Kind::LESS:
	case BinaryExpr::Kind::LESS_EQUAL:
	    type = Type::getBool();
	    leftType = rightType = commonType;
	    break;
	case BinaryExpr::Kind::LOGICAL_AND:
	case BinaryExpr::Kind::LOGICAL_OR:
	    type = leftType = rightType = Type::getBool();
	    break;
	default:
	    type = nullptr;
	    break;
    }
    if (!type || !leftType || !rightType) { 
	return binaryErr(kind, std::move(left), std::move(right), loc);
    }
    if (*left->type != *leftType) {
	left = CastExpr::create(std::move(left), leftType);
    }
    if (*right->type != *rightType) {
	right = CastExpr::create(std::move(right), rightType);
    }
    return std::make_tuple(std::move(left), std::move(right), type);
}

static BinaryResult
binaryArray(BinaryExpr::Kind kind, ExprPtr &&left, ExprPtr &&right,
	    Token::Loc *loc)
{
    const Type *leftType = nullptr;
    const Type *rightType = nullptr;

    switch (kind) {
	case BinaryExpr::Kind::ASSIGN:
	    if (left->type->isArray() && right->type->isArray()) {
		if (*left->type->getRefType() != *right->type->getRefType()
			|| left->type->getDim() < right->type->getDim())
		{
		    return binaryErr(kind, std::move(left), std::move(right),
				     loc);
		}
		return std::make_tuple(std::move(left), std::move(right),
				       left->type);	
	    } else if (left->type->isPointer()) {
		leftType = Type::getPointer(left->type->getRefType());
		rightType = right->type;
	    }
	    break;
	case BinaryExpr::Kind::ADD:
	case BinaryExpr::Kind::SUB:
	case BinaryExpr::Kind::MUL:
	case BinaryExpr::Kind::DIV:
	case BinaryExpr::Kind::MOD:
	case BinaryExpr::Kind::EQUAL:
	case BinaryExpr::Kind::NOT_EQUAL:
	case BinaryExpr::Kind::GREATER:
	case BinaryExpr::Kind::GREATER_EQUAL:
	case BinaryExpr::Kind::LESS:
	case BinaryExpr::Kind::LESS_EQUAL:
	case BinaryExpr::Kind::LOGICAL_AND:
	case BinaryExpr::Kind::LOGICAL_OR:
	    leftType = Type::convertArrayOrFunctionToPointer(left->type);
	    rightType = Type::convertArrayOrFunctionToPointer(right->type);
	    break;
	default:
	    ;
    }
    if (leftType && rightType) {
	if (*left->type != *leftType) {
	    left = CastExpr::create(std::move(left), leftType);
	}
	if (*right->type != *rightType) {
	    right = CastExpr::create(std::move(right), rightType);
	}
	return binary(kind, std::move(left), std::move(right), loc);
    }
    return binaryErr(kind, std::move(left), std::move(right), loc);
}

static BinaryResult
binaryStruct(BinaryExpr::Kind kind, ExprPtr &&left, ExprPtr &&right,
	     Token::Loc *loc)
{
    switch (kind) {
	case BinaryExpr::Kind::ASSIGN:
	    if (*left->type == *Type::getConstRemoved(right->type)) {
		return std::make_tuple(std::move(left), std::move(right),
				       left->type);
	    }
	    break;

	case BinaryExpr::Kind::MEMBER:
	    assert(0 && "case should be handled in BinaryExpr::createMember");
	    break;
	default:
	    ;
    }
    return binaryErr(kind, std::move(left), std::move(right), loc);
}

/*
 * Rules for conditional expressions
 */

ConditionalResult
conditional(ExprPtr &&cond, ExprPtr &&thenExpr, ExprPtr &&elseExpr,
	    Token::Loc *loc)
{
    const Type *type = getCommonType(thenExpr->type, elseExpr->type, loc);
    if (!type) {
	if (loc) {
	    error::out() << *loc << ": operator can not be applied to"
		<< thenExpr->loc << " operand of type '" << thenExpr->type
		<< "' and '"
		<< elseExpr->loc << " operand of type '" << elseExpr->type
		<< "'" << std::endl;
	    error::fatal();
	}
	return std::make_tuple(std::move(cond), std::move(thenExpr),
			       std::move(elseExpr), type);
    }
    if (*thenExpr->type != *type) {
	thenExpr = CastExpr::create(std::move(thenExpr), type);
    }
    if (*elseExpr->type != *type) {
	elseExpr = CastExpr::create(std::move(elseExpr), type);
    }
    return std::make_tuple(std::move(cond), std::move(thenExpr),
			   std::move(elseExpr), type);
}

static const Type *
getCommonType(const Type *left, const Type *right, Token::Loc *loc)
{
    left = Type::convertArrayOrFunctionToPointer(left);
    right = Type::convertArrayOrFunctionToPointer(right);

    if (*left == *right) {
	return left;
    } else if (*left != *right && (left->isVoid() || right->isVoid())) {
	return nullptr;
    } else if (left->isInteger() && right->isInteger()) {
	auto size = std::max(left->getIntegerNumBits(),
			     right->getIntegerNumBits());
	// when mixing signed and unsigned: unsigned wins 
	return left->getIntegerKind() == Type::UNSIGNED
	    || right->getIntegerKind() == Type::UNSIGNED
	    ? Type::getUnsignedInteger(size)
	    : Type::getSignedInteger(size);
    } else if (left->isNullPointer() || right->isNullPointer()) {
	return nullptr;
    } else {
	if (loc) {
	    error::out() << *loc
		<< ": Error: no common type for type '" << left
		<< "' and type '" << right << "'" << std::endl;
	    error::fatal();
	}
	return nullptr;
    }
}


} // namespace promotion
