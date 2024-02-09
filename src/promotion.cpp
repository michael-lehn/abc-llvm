#include "castexpr.hpp"
#include "error.hpp"
#include "promotion.hpp"

namespace promotion {

/*
 * Rules for unary expressions
 */

static UnaryResult unaryErr(UnaryExpr::Kind kind, ExprPtr &&child,
			    Token::Loc *loc);

UnaryResult
unary(UnaryExpr::Kind kind, ExprPtr &&child, Token::Loc *loc)
{
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
	    type = Type::getBool();
	    if (child->type->isInteger()) {
		childType = child->type;
	    } else if (loc && child->type->isArray()) {
		childType = Type::getPointer(child->type);
		error::out() << *loc
		    << ": Warning: Address of array will "
		    << "always evaluate to 'true'" << std::endl;
		error::warning();
	    } else if (loc && child->type->isFunction()) {
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
	default:
	    ;
    }
    if (!type || !childType) {
	unaryErr(kind, std::move(child), loc);
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
	error::out() << *loc << ": operator can not be applied to"
	    << child->loc << " operand of type '" << child->type
	    << "'" << std::endl;
	error::fatal();
    }
    return std::make_pair(std::move(child), nullptr);
}

/*
 * Rules for binary expressions
 */

static BinaryResult binaryErr(BinaryExpr::Kind kind, ExprPtr &&left,
			      ExprPtr &&right, Token::Loc *loc);

static BinaryResult binaryPtr(BinaryExpr::Kind kind, ExprPtr &&left,
			      ExprPtr &&right, Token::Loc *loc);

static BinaryResult binaryInt(BinaryExpr::Kind kind, ExprPtr &&left,
			      ExprPtr &&right, Token::Loc *loc);

static BinaryResult binaryArr(BinaryExpr::Kind kind, ExprPtr &&left,
			      ExprPtr &&right, Token::Loc *loc);

BinaryResult
binary(BinaryExpr::Kind kind, ExprPtr &&left, ExprPtr &&right, Token::Loc *loc)
{
    if (!left->type || !right->type) {
	return binaryErr(kind, std::move(left), std::move(right), loc);
    } else if (left->type->isPointer() || right->type->isPointer()) {
	return binaryPtr(kind, std::move(left), std::move(right), loc);
    } else if (left->type->isArray() || right->type->isArray()) {
	return binaryArr(kind, std::move(left), std::move(right), loc);
    } else if (left->type->isInteger() || right->type->isInteger()) {
	return binaryInt(kind, std::move(left), std::move(right), loc);
    }
    return binaryErr(kind, std::move(left), std::move(right), loc);
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
binaryPtr(BinaryExpr::Kind kind, ExprPtr &&left, ExprPtr &&right,
	  Token::Loc *loc)
{
    if (kind == BinaryExpr::ADD && !left->type->isPointer()) {
	left.swap(right);
    }
    const Type *type = nullptr;
    switch (kind) {
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
	default:
	    break;
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
	    type = leftType = rightType = left->type;
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
binaryArr(BinaryExpr::Kind kind, ExprPtr &&left, ExprPtr &&right,
	  Token::Loc *loc)
{
    const Type *type = nullptr;
    /*
    const Type *leftType = nullptr;
    const Type *rightType = nullptr;

    switch (kind) {
	case BinaryExpr::Kind::ASSIGN:
	    if (left->type->isArray() && right->type->isArray()) {
		// ...
		return 
	    }

	case BinaryExpr::Kind::ADD:
	case BinaryExpr::Kind::SUB:
	default:
    }
    */
    return std::make_tuple(std::move(left), std::move(right), type);
}

/*
 * Rules for conditional expressions
 */

static const Type *getCommonType(const Type *left, const Type *right,
				 Token::Loc *loc);

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
    } else if (left->isPointer() && right->isInteger()) {
	auto ty = Type::getUnsignedInteger(64); // TODO: some size_t type?
	if (loc) {
	    error::out() << loc
		<< ": Warning: Cast of '" << left
		<< "' to '" << ty << "'" << std::endl;
	    error::warning();
	}
	return ty;
    } else if (left->isInteger() && right->isPointer()) {
	auto ty = Type::getUnsignedInteger(64); // TODO: some size_t type?
	if (loc) {
	    error::out() << loc
		<< ": Warning: Cast of '" << right
		<< "' to '" << ty << "'" << std::endl;
	    error::warning();
	}
	return ty;
    } else {
	if (loc) {
	    error::out() << loc
		<< ": Error: no common type for type '" << left
		<< "' and type '" << right << "'" << std::endl;
	    error::fatal();
	}
	return nullptr;
    }
}


} // namespace promotion
