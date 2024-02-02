#include <cstring>
#include <charconv>
#include <iostream>
#include <type_traits>
#include <variant>

#include "expr.hpp"
#include "symtab.hpp"

//------------------------------------------------------------------------------

void
ExprDeleter::operator()(const Expr *expr) const
{
    delete expr;
}
//
//------------------------------------------------------------------------------

/*
 * Class Literal
 */

Literal::Literal(UStr val, const Type *type, std::uint8_t radix,
		 Token::Loc loc)
    : val{val}, type{type}, radix{radix}, loc{loc}
{}
    
/*
 * Class Identifier
 */

Identifier::Identifier(UStr ident, const Type *type, Token::Loc loc)
    : val{ident.c_str()}, type{type}, loc{loc}
{}

//-- handling type conversions and casts ---------------------------------------

static const Type *
getCommonType(Token::Loc loc, const Type *left, const Type *right)
{
    left = Type::convertArrayOrFunctionToPointer(left);
    right = Type::convertArrayOrFunctionToPointer(right);

    if (*left == *right) {
	return left;
    } else if (left->isInteger() && right->isInteger()) {
	auto size = std::max(left->getIntegerNumBits(),
			     right->getIntegerNumBits());
	// when mixing signed and unsigned: unsigned wins 
	return left->getIntegerKind() == Type::UNSIGNED
	    || right->getIntegerKind() == Type::UNSIGNED
	    ? Type::getUnsignedInteger(size)
	    : Type::getSignedInteger(size);
    } else if (left->isNullPointer()) {
	error::out() << loc
	    << ": Error: operation not defined for '" << left
	    << "' and '" << right << "'" << std::endl;
	error::fatal();
    } else if (left->isPointer() && right->isInteger()) {
	auto ty = Type::getUnsignedInteger(64); // TODO: some size_t type?
	error::out() << loc
	    << ": Warning: Cast of '" << left
	    << "' to '" << ty << "'" << std::endl;
	return ty;
    } else if (left->isInteger() && right->isPointer()) {
	auto ty = Type::getUnsignedInteger(64); // TODO: some size_t type?
	error::out() << loc
	    << ": Warning: Cast of '" << right
	    << "' to '" << ty << "'" << std::endl;
	return ty;
    }


    error::out() << loc
	<< ": Error: operation not defined for '" << left
	<< "' and '" << right << "'" << std::endl;
    error::fatal();
    return nullptr;
}

//-- class Unary =--------------------------------------------------------------

void
Unary::setTypeAndCastOperands(void)
{
    switch (kind) {
	case ADDRESS:
	    type = Type::getPointer(child->getType());
	    return;
	case DEREF:
	    if (child->getType()->isNullPointer()) {
		error::out() << opLoc
		    << ": Error: dereferencing null pointer" << std::endl;
		error::fatal();
	    }
	    type = child->getType()->getRefType();
	    return;
	case LOGICAL_NOT:
	    type = Type::getBool();
	    if (child->getType()->isArray()) {
		error::out() << opLoc
		    << ": Warning: Address of array will "
		    << "always evaluate to 'true'" << std::endl;
	    } else if (child->getType()->isFunction()) {
		/*
		auto ty = Type::getPointer(child->getType());
		child = Expr::createCast(std::move(child), ty, opLoc);
		*/
		error::out() << opLoc
		    << ": Warning: Address of function will "
		    << "always evaluate to 'true'" << std::endl;
	    }
	    return;
	default:
	    assert(0 && "case not handled");
    }
}

//-- class Binary --------------------------------------------------------------

void
Binary::setType(void)
{
    auto l = left->getType(),
	 r = right->getType();

    if (kind == Binary::Kind::ADD || kind == Binary::Kind::SUB) {
	l = Type::convertArrayOrFunctionToPointer(l);
	r = Type::convertArrayOrFunctionToPointer(r);
    }

    switch (kind) {
	case Binary::Kind::MEMBER:
	    type = r;
	    return;
	case Binary::Kind::CALL:
	    type = l->getRetType();
	    return;
	case Binary::Kind::ASSIGN:
	    if (l->isArray() || l->isFunction() || l->hasConstFlag()) {
		error::out() << opLoc
		    << " cannot assign to variable with type '"
		    << l << "'" << std::endl;
		error::fatal();
	    }
	    type = Type::getTypeConversion(r, l, opLoc);
	    if (!type) {
		error::out() << opLoc << " can not cast expression of type '"
		    << r << "' to type '" << l << "'" <<std::endl;
		error::fatal();
	    }
	    return;
	case Binary::Kind::POSTFIX_INC:
	case Binary::Kind::POSTFIX_DEC:
	case Binary::Kind::ADD:
	    if (l->isPointer() || r->isPointer()) {
		if (l->isPointer() && r->isInteger()) {
		    type = l;
		} else if (r->isPointer() && l->isInteger()) {
		    // for pointer arithmeitc let right always be the index
		    type = r;
		    left.swap(right);
		    std::swap(l, r);
		} else {
		    type = nullptr;
		}
	    } else {
		type = getCommonType(opLoc, l, r);
	    }
	    return;
	case Binary::Kind::SUB:
	    if (l->isPointer() && r->isPointer()) {
		type = Type::getSignedInteger(64); // TODO: some ptrdiff_t
	    } else if (l->isPointer() && r->isInteger()) {
		type = l;
	    } else {
		type = getCommonType(opLoc, l, r);
	    }
	    return;
	case Binary::Kind::MUL:
	case Binary::Kind::DIV:
	case Binary::Kind::MOD:
	    if (l->isInteger() && r->isInteger()) {
		type = getCommonType(opLoc, l, r);
	    }
	    return;
	case Binary::Kind::EQUAL:
	case Binary::Kind::NOT_EQUAL:
	case Binary::Kind::GREATER:
	case Binary::Kind::GREATER_EQUAL:
	case Binary::Kind::LESS:
	case Binary::Kind::LESS_EQUAL:
	case Binary::Kind::LOGICAL_AND:
	case Binary::Kind::LOGICAL_OR:
	    type = Type::getBool();
	    return;
	default:
	    type = nullptr;
	    return;
    }
}

void
Binary::castOperands(void)
{
    if (!type) {
	error::out() << opLoc
	    << ": ERROR: binary operation not allowed for type '"
	    << left->getType() << "' and type '" << right->getType() << "'"
	    << std::endl;
	error::fatal();
	return;
    }

    auto l = left->getType(),
	 r = right->getType();

    if (kind == Binary::Kind::ADD || kind == Binary::Kind::SUB) {
	l = Type::convertArrayOrFunctionToPointer(l);
	r = Type::convertArrayOrFunctionToPointer(r);
    }

    switch (kind) {
	case Binary::Kind::ASSIGN:
	    if (*r != *type) {
		right = Expr::createCast(std::move(right), type, opLoc);
	    }
	    return;
	case Binary::Kind::POSTFIX_INC:
	case Binary::Kind::ADD:
	    if (l->isPointer() || r->isPointer()) {
		// for pointer arithmeitc let right always be the index
		assert(l->isPointer() && r->isInteger());
	    } else if (l->isInteger() && r->isInteger()) {
		if (*l != *type) {
		    left = Expr::createCast(std::move(left), type);
		}
		if (*r != *type) {
		    right = Expr::createCast(std::move(right), type);
		}
	    }
	    return;
	case Binary::Kind::POSTFIX_DEC:
	case Binary::Kind::SUB:
	    if (l->isInteger() && r->isInteger()) {
		if (*l != *type) {
		    left = Expr::createCast(std::move(left), type);
		}
		if (*r != *type) {
		    right = Expr::createCast(std::move(right), type);
		}
	    }
	    return;
	case Binary::Kind::MUL:
	case Binary::Kind::DIV:
	case Binary::Kind::MOD:
	    if (l->isInteger() && r->isInteger()) {
		if (*l != *type) {
		    left = Expr::createCast(std::move(left), type);
		}
		if (*r != *type) {
		    right = Expr::createCast(std::move(right), type);
		}
	    }
	    return;
	case Binary::Kind::EQUAL:
	case Binary::Kind::NOT_EQUAL:
	case Binary::Kind::GREATER:
	case Binary::Kind::GREATER_EQUAL:
	case Binary::Kind::LESS:
	case Binary::Kind::LESS_EQUAL:
	    {
		auto ty = getCommonType(opLoc, l, r);
		if (*l != *ty) {
		    left = Expr::createCast(std::move(left), ty);
		}
		if (*r != *ty) {
		    right = Expr::createCast(std::move(right), ty);
		}
		return;
	    }
	case Binary::Kind::LOGICAL_AND:
	case Binary::Kind::LOGICAL_OR:
	    {
		auto ty = Type::getBool();
		if (*l != *ty) {
		    left = Expr::createCast(std::move(left), ty);
		}
		if (*r != *ty) {
		    right = Expr::createCast(std::move(right), ty);
		}
		return;
	    }
	default:
	    return;
    }
}

//-- class Conditional ---------------------------------------------------------

void
Conditional::setTypeAndCastOperands(void)
{
    const Type *l = left->getType();
    const Type *r = right->getType();

    type = getCommonType(opRightLoc, l, r);
    if (l != type) {
	left = Expr::createCast(std::move(left), type);
    }
    if (r != type) {
	right = Expr::createCast(std::move(right), type);
    }
}

//-- class Expr: static member functions ---------------------------------------

template <typename IntType, std::uint8_t numBits>
static bool
getIntType(const char *s, const char *end, std::uint8_t radix, const Type *&ty)
{
    IntType result;
    auto [ptr, ec] = std::from_chars(s, end, result, radix);
    if (ec == std::errc{}) {
	ty = Type::getSignedInteger(numBits);
	return true;
    }
    ty = nullptr; 
    return false;
}

static const Type *
getIntType(const char *s, const char *end, std::uint8_t radix, Token::Loc loc)
{
    //TODO: Handle type of integer literals like in Rust. Currently handled
    //	    like in C, i.e. it is at least of type 'int' (where i32 is choosen)
    const Type *ty = nullptr;
    if (getIntType<std::int32_t, 32>(s, end, radix, ty)
     || getIntType<std::int64_t, 64>(s, end, radix, ty))
    {
	return ty;
    }
    error::out() << loc << ": warning: signed integer '" << s
	<< "' does not fit into 64 bits"
	<< std::endl;
    return Type::getSignedInteger(64);
}

ExprPtr
Expr::createNull(const Type *type, Token::Loc loc)
{
    if (type->isPointer()) {
	type = Type::getNullPointer();
    }
    return ExprPtr{new Expr{Literal{UStr{"0"}.c_str(), type, 10, loc}}};
}

ExprPtr
Expr::createLiteral(UStr val, std::uint8_t radix, const Type *type,
		    Token::Loc loc)
{
    if (!type)  {
	auto cstr = val.c_str();
	type = getIntType(cstr, cstr + strlen(cstr), radix, loc);
    }
    return ExprPtr{new Expr{Literal{val, type, radix, loc}}};
}

ExprPtr
Expr::createIdentifier(UStr ident, Token::Loc loc)
{
    auto symEntry = Symtab::get(ident);
    if (!symEntry) {
	error::out() << loc << " undeclared identifier '"
	    << ident.c_str() << "'" << std::endl;
	error::fatal();
    } else if (symEntry->holdsExpr()) {
	return ExprPtr{new Expr{Proxy{symEntry->getExpr()}}};
    }
    auto type = symEntry->getType();
    return ExprPtr{new Expr{Identifier{ident, type, loc}}};
}

ExprPtr
Expr::createProxy(const Expr *expr)
{
    return ExprPtr{new Expr{Proxy{expr}}};
}


ExprPtr
Expr::createUnaryMinus(ExprPtr &&expr, Token::Loc opLoc)
{
    auto ty = expr->getType();
    auto zero = createNull(ty, opLoc);
    return createBinary(Binary::Kind::SUB, std::move(zero), std::move(expr),
			opLoc);
}

ExprPtr
Expr::createLogicalNot(ExprPtr &&expr, Token::Loc opLoc)
{
    auto lNot = new Expr{Unary{Unary::Kind::LOGICAL_NOT, std::move(expr),
			       opLoc}};
    return ExprPtr{lNot};
}

ExprPtr
Expr::createAddr(ExprPtr &&expr, Token::Loc opLoc)
{
    auto addr = new Expr{Unary{Unary::Kind::ADDRESS, std::move(expr), opLoc}};
    return ExprPtr{addr};
}

ExprPtr
Expr::createDeref(ExprPtr &&expr, Token::Loc opLoc)
{
    assert(expr->getType()->isArrayOrPointer());

    auto dref = new Expr{Unary{Unary::Kind::DEREF, std::move(expr), opLoc}};
    return ExprPtr{dref};
}


ExprPtr
Expr::createCast(ExprPtr &&child, const Type *toType, Token::Loc opLoc)
{
    auto expr = new Expr{Unary{Unary::Kind::CAST, std::move(child), toType,
			       opLoc}};
    return ExprPtr{expr};
}

ExprPtr
Expr::createBinary(Binary::Kind kind, ExprPtr &&left, ExprPtr &&right,
		   Token::Loc opLoc)
{
    auto expr = new Expr{Binary{kind, std::move(left), std::move(right),
			        opLoc}};
    //expr->print();
    return ExprPtr{expr};
}

ExprPtr
Expr::createMember(ExprPtr &&from, UStr memberIdent, Token::Loc opLoc)
{
    auto ty = from->getType();
    if (!ty->isStruct() || !ty->hasMember(memberIdent)) {
	error::out() << opLoc << ": "
	    << (ty->isStruct() ? "struct" : "")
	    << " type '" << ty
	    << "' has no member '"
	    << memberIdent.c_str() << "'" << std::endl;
	error::fatal();
	return nullptr;
    }
    ty = ty->getMemberType(memberIdent); 
    auto mem = ExprPtr{new Expr{Identifier{memberIdent, ty, opLoc}}};
    return createBinary(Binary::Kind::MEMBER, std::move(from), std::move(mem));
}

ExprPtr
Expr::createCall(ExprPtr &&fn, ExprVector &&param, Token::Loc opLoc)
{
    if (fn->getType()->isPointer()) {
	fn = createDeref(std::move(fn));
    }
    if (!fn->getType()->isFunction()) {
	error::out() << opLoc << " : not a function" << std::endl;
	error::fatal();
    }
    auto const argType = fn->getType()->getArgType();
    if ((argType.size() > param.size())
     || (argType.size() < param.size() && !fn->getType()->hasVarg())) {
	error::out() << opLoc << ": too "
	    << (argType.size() < param.size() ? "many" : "few")
	    << " arguments to function call, expected "
	    << argType.size() << ", have "
	    << param.size() << std::endl;
	error::fatal();
    }
    for (std::size_t i = 0; i < argType.size(); ++i) {
	const auto &p = param[i];
	if (!Type::getTypeConversion(p->getType(), argType[i], p->getLoc())) {
	    error::out() << opLoc << ": incompatible conversion of type '"
		<< param[i]->getType() << "' to type '" << argType[i]
		<< std::endl;
	    error::fatal();
	}
    }

    auto p = createExprVector(std::move(param));
    auto expr = createBinary(Binary::Kind::CALL, std::move(fn), std::move(p),
			     opLoc);
    return ExprPtr{std::move(expr)};
}

ExprPtr
Expr::createConditional(ExprPtr &&cond, ExprPtr &&left, ExprPtr &&right,
			Token::Loc opLeftLoc, Token::Loc opRightLoc)
{
    auto expr = new Expr{Conditional{std::move(cond), std::move(left),
				     std::move(right), opLeftLoc, opRightLoc}};
    return ExprPtr{std::move(expr)};
}

ExprPtr
Expr::createExprVector(ExprVector &&expr)
{
    return ExprPtr{new Expr{std::move(expr)}};
}

//-- methods -------------------------------------------------------------------

static const char *
exprKindCStr(Unary::Kind kind)
{
    switch (kind) {
	case Unary::Kind::ADDRESS:
	    return "address";
	case Unary::Kind::DEREF:
	    return "address";
	case Unary::Kind::CAST:
	    return "cast";
	default:
	    return "?";
    }
}

static const char *
exprKindCStr(Binary::Kind kind)
{
    switch (kind) {
	case Binary::Kind::CALL:
	    return "call";
	case Binary::Kind::ADD:
	    return "+";
	case Binary::Kind::ASSIGN:
	    return "=";
	case Binary::Kind::EQUAL:
	    return "==";
	case Binary::Kind::NOT_EQUAL:
	    return "!=";
	case Binary::Kind::GREATER:
	    return ">";
	case Binary::Kind::GREATER_EQUAL:
	    return ">=";
	case Binary::Kind::LESS:
	    return "<";
	case Binary::Kind::LESS_EQUAL:
	    return "<=";
	case Binary::Kind::LOGICAL_AND:
	    return "&&";
	case Binary::Kind::LOGICAL_OR:
	    return "||";
	case Binary::Kind::SUB:
	    return "-";
	case Binary::Kind::MUL:
	    return "*";
	case Binary::Kind::DIV:
	    return "/";
	case Binary::Kind::MOD:
	    return "%";
	case Binary::Kind::MEMBER:
	    return ".member";
	default:
	    return "?";
    }
}

const Type *
Expr::getType(void) const
{
    if (std::holds_alternative<Proxy>(variant)) {
	return std::get<Proxy>(variant).expr->getType();
    } else if (std::holds_alternative<Literal>(variant)) {
	return std::get<Literal>(variant).type;
    } else if (std::holds_alternative<Identifier>(variant)) {
	return std::get<Identifier>(variant).type;
    } else if (std::holds_alternative<Unary>(variant)) {
	return std::get<Unary>(variant).type;
    } else if (std::holds_alternative<Binary>(variant)) {
	return std::get<Binary>(variant).type;
    } else if (std::holds_alternative<Conditional>(variant)) {
	return std::get<Conditional>(variant).type;
    } else if (std::holds_alternative<ExprVector>(variant)) {
	const auto &vec = std::get<ExprVector>(variant);
	return vec.empty() ? nullptr : vec.back()->getType();
    }
    assert(0);
    return nullptr; // never reached
}

bool
Expr::isLValue(void) const
{
    if (std::holds_alternative<Proxy>(variant)) {
	return std::get<Proxy>(variant).expr->isLValue();
    } else if (std::holds_alternative<Identifier>(variant)) {
	return true;
    } else if (std::holds_alternative<Unary>(variant)) {
	const auto &unary = std::get<Unary>(variant);
	return unary.kind == Unary::Kind::DEREF;
    } else if (std::holds_alternative<Binary>(variant)) {
	const auto &binary = std::get<Binary>(variant);
	if (binary.kind == Binary::MEMBER) {
	    return binary.right->isLValue();
	}
	return false;
    }
    return false;
}

static bool
isIntegerConstExpr(const Binary &binary)
{
    return binary.type->isInteger()
	&& binary.left->getType()->isInteger()
	&& binary.left->isConst()
	&& binary.right->getType()->isInteger()
	&& binary.right->isConst();
}

static bool
isArithmeticConstExpr(const Binary &binary)
{
    // identical to 'isIntegerConstExp" until float is supported
    return isIntegerConstExpr(binary);
}

static bool
isAddressConstant(const Binary &binary)
{
    return binary.type->isPointer()
	&& binary.left->isConst()
	&& binary.right->isConst();
}

// In ISO/IEC 9899:2017 constant expression are defined in 6.6
bool
Expr::isConst(void) const
{
    if (std::holds_alternative<Proxy>(variant)) {
	return std::get<Proxy>(variant).expr->isConst();
    } else if (std::holds_alternative<Literal>(variant)) {
	return true;
    } else if (std::holds_alternative<Identifier>(variant)) {
	const auto &ident = std::get<Identifier>(variant);
	if (ident.type->isFunction()) {
	    return true;
	}
	if (ident.type->isArray()) {
	    return true;
	}
	return false;
    } else if (std::holds_alternative<Unary>(variant)) {
	const auto &unary = std::get<Unary>(variant);
	switch (unary.kind) {
	    case Unary::Kind::DEREF:
	    case Unary::Kind::CAST:
	    case Unary::Kind::LOGICAL_NOT:
		return unary.child->isConst();
	    case Unary::Kind::ADDRESS:
		return true;
	    default:
		assert(0 && "internal error: case not handled");
		return false;
	}
    } else if (std::holds_alternative<Binary>(variant)) {
	const auto &binary = std::get<Binary>(variant);
	switch (binary.kind) {
	    case Binary::Kind::CALL:
	    case Binary::Kind::ASSIGN:
	    case Binary::Kind::POSTFIX_INC:
	    case Binary::Kind::POSTFIX_DEC:
		return false;
	    default:
		return isArithmeticConstExpr(binary)
		    || isAddressConstant(binary);
	}
    } else if (std::holds_alternative<Conditional>(variant)) {
	const auto &expr = std::get<Conditional>(variant);
	if (!expr.cond->isConst()) {
	    return false;
	}
	auto condVal = expr.cond->loadConst();
	if (condVal->isZeroValue()) {
	    return expr.left->isConst();
	} else {
	    return expr.right->isConst();
	}
    }
    assert(0);
    return false;
}

gen::ConstVal
Expr::loadConst(void) const
{
    assert(isConst());

    using T = std::remove_pointer_t<gen::ConstVal>;
    auto constVal = llvm::dyn_cast<T>(loadValue());
    return constVal;
}

gen::ConstIntVal
Expr::loadConstInt(void) const
{
    assert(isConst());
    assert(getType()->isInteger());

    using T = std::remove_pointer_t<gen::ConstIntVal>;
    auto check = llvm::dyn_cast<T>(loadValue());
    return check;
}

static gen::AluOp
getGenAluOp(Binary::Kind kind, const Type *type)
{
    bool isSignedInt = type->isInteger()
		    && type->getIntegerKind() == Type::SIGNED;
    switch (kind) {
	case Binary::Kind::POSTFIX_INC:
	case Binary::Kind::ADD:
	    return gen::ADD;
	case Binary::Kind::POSTFIX_DEC:
	case Binary::Kind::SUB:
	    return gen::SUB;
	case Binary::Kind::MUL:
	    return gen::SMUL;
	case Binary::Kind::DIV:
	    return isSignedInt ? gen::SDIV : gen::UDIV;
	case Binary::Kind::MOD:
	    return isSignedInt ? gen::SMOD : gen::UMOD;
	default:
	    assert(0);
	    return gen::ADD;
    }
}

static gen::CondOp
getGenCondOp(Binary::Kind kind, const Type *type)
{
    bool isSignedInt = type->isInteger()
		    && type->getIntegerKind() == Type::SIGNED;
    switch (kind) {
	case Binary::Kind::EQUAL:
	    return gen::EQ;
	case Binary::Kind::NOT_EQUAL:
	    return gen::NE;
	case Binary::Kind::LESS:
	    return isSignedInt ? gen::SLT : gen::ULT;
	case Binary::Kind::LESS_EQUAL:
	    return isSignedInt ? gen::SLE : gen::ULE;
	case Binary::Kind::GREATER:
	    return isSignedInt ? gen::SGT : gen::UGT;
	case Binary::Kind::GREATER_EQUAL:
	    return isSignedInt ? gen::SGE : gen::UGE;
	case Binary::Kind::LOGICAL_AND:
	    return gen::AND;
	case Binary::Kind::LOGICAL_OR:
	    return gen::OR;
	default:
	    std::cerr << "Not handled: " << int(kind) << std::endl;
	    assert(0);
	    return gen::EQ;
    }
}

Token::Loc
Expr::getLoc(void) const
{
    if (std::holds_alternative<Proxy>(variant)) {
	return std::get<Proxy>(variant).expr->getLoc();
    } else if (std::holds_alternative<Literal>(variant)) {
	return std::get<Literal>(variant).loc;
    } else if (std::holds_alternative<Identifier>(variant)) {
	return std::get<Identifier>(variant).loc;
    } else if (std::holds_alternative<Unary>(variant)) {
	return std::get<Unary>(variant).opLoc;
    } else if (std::holds_alternative<Binary>(variant)) {
	return std::get<Binary>(variant).opLoc;
    } else if (std::holds_alternative<Conditional>(variant)) {
	return std::get<Conditional>(variant).cond->getLoc();
    } else if (std::holds_alternative<ExprVector>(variant)) {
	const auto &vec = std::get<ExprVector>(variant);
	return vec.size() ? vec[0]->getLoc() : Token::Loc{};
    } else {
	std::cerr << "not handled variant. Index = " << variant.index()
	    << std::endl;
	assert(0);
    }
}


void
Expr::print(int indent) const
{
    std::printf("%*s", indent, "");

    if (std::holds_alternative<Proxy>(variant)) {
	std::get<Proxy>(variant).expr->print(indent);
    } else if (std::holds_alternative<Literal>(variant)) {
	const auto &lit = std::get<Literal>(variant);
	std::printf("Literal: %s ", lit.val.c_str());
	std::cout << getType() << std::endl;
    } else if (std::holds_alternative<Identifier>(variant)) {
	const auto &ident = std::get<Identifier>(variant);
	std::printf("Identifier: %s ", ident.val);
	std::cout << getType() << std::endl;
    } else if (std::holds_alternative<Unary>(variant)) {
	const auto &unary = std::get<Unary>(variant);
	std::printf("[%s] ", exprKindCStr(unary.kind));
	std::cout << getType() << std::endl;
	unary.child->print(indent + 4);
    } else if (std::holds_alternative<Binary>(variant)) {
	const auto &binary = std::get<Binary>(variant);
	std::printf("[%s] ", exprKindCStr(binary.kind));
	std::cout << getType() << std::endl;
	binary.left->print(indent + 4);
	binary.right->print(indent + 4);
    } else if (std::holds_alternative<Conditional>(variant)) {
	const auto &conditional = std::get<Conditional>(variant);
	std::printf("[cond ? left : right]");
	std::cout << getType() << std::endl;
	conditional.cond->print(indent + 4);
	conditional.left->print(indent + 4);
	conditional.right->print(indent + 4);
    } else if (std::holds_alternative<ExprVector>(variant)) {
	const auto &vec = std::get<ExprVector>(variant);
	std::printf("[vec]");
	for (const auto &e: vec) {
	    e->print(indent + 4);
	}
    } else {
	std::cerr << "not handled variant. Index = " << variant.index()
	    << std::endl;
	assert(0);
    }
}

//-- code generation -----------------------------------------------------------

static gen::Reg
loadValue(const Literal &l)
{
    if (!l.type->isInteger() && !l.type->isNullPointer()) {
	error::out() << l.loc << ": literal has type '"
	    << l.type << "' and value '" << l.val.c_str() << "'"
	    << std::endl;
	if (l.type->isPointer()) {
	    error::out() << l.loc << ": deref type '"
		<< l.type->getRefType() << "'" << std::endl;
	}
	error::fatal();
    }
    return gen::loadIntConst(l.val.c_str(), l.type, l.radix);
}

static gen::Reg
loadValue(const Identifier &ident)
{
    if (ident.type->isFunction() || ident.type->isArray()) {
	return gen::loadAddr(ident.val);
    }

    return gen::fetch(ident.val, ident.type);
}

static void condJmp(const Unary &, gen::Label, gen::Label);
static gen::Reg loadAddr(const Binary &binary);

static gen::Reg
loadValue(const Unary &unary)
{
    switch (unary.kind) {
	case Unary::Kind::CAST:
	    if (unary.type->isBool()) {
		auto ty = unary.child->getType();
		auto zero = Expr::createNull(ty, unary.opLoc)->loadValue();
		return gen::cond(gen::NE, unary.child->loadValue(), zero);
	    }
	    return gen::cast(unary.child->loadValue(), unary.child->getType(),
			     unary.type);
	case Unary::Kind::LOGICAL_NOT:
	    {
		auto ty = unary.child->getType();
		ty = Type::convertArrayOrFunctionToPointer(ty);
		auto zero = Expr::createNull(ty, unary.opLoc)->loadValue();
		return gen::cond(gen::EQ, unary.child->loadValue(), zero);
	    }
	case Unary::Kind::DEREF:
	    {
		auto addr = unary.child->loadValue();
		if (unary.type->isFunction() || unary.type->isArray()) {
		    return addr;
		} else if (unary.type->isNullPointer()) {
		    error::out() << unary.opLoc
			<< ": Error: dereferencing null pointer" << std::endl;
		    error::fatal();
		}
		auto ty = unary.child->getType()->getRefType();
		return gen::fetch(addr, ty);
	    }
	case Unary::Kind::ADDRESS:
	    {
		auto lValue = unary.child.get();
		assert(lValue->isLValue());
		if (std::holds_alternative<Identifier>(lValue->variant)) {
		    auto ident = std::get<Identifier>(lValue->variant);
		    return gen::loadAddr(ident.val);
		} else if (std::holds_alternative<Unary>(lValue->variant)) {
		    const auto &unary = std::get<Unary>(lValue->variant);
		    if (unary.kind == Unary::Kind::DEREF) {
			return unary.child->loadValue();
		    }
		    assert(0);
		    return nullptr;
		} else if (std::holds_alternative<Binary>(lValue->variant)) {
		    const auto &binary = std::get<Binary>(lValue->variant);
		    return loadAddr(binary);
		} else {
		    assert(0); // not implemented
		    return nullptr;
		}
	    }
	default:
	    assert(0);
	    return nullptr;
    }
}
	
static void condJmp(const Binary &, gen::Label, gen::Label);
static gen::Reg loadAddr(const Binary &binary);

static gen::Reg
loadValue(const Binary &binary)
{
    switch (binary.kind) {
	case Binary::Kind::MEMBER:
	    {
		auto addr = loadAddr(binary);
		if (binary.type->isArray()) {
		    return addr;
		}
		return gen::fetch(addr, binary.type);
	    }
	case Binary::Kind::CALL:
	    {
		auto const &left = binary.left;
		auto const &right = binary.right;
		assert(std::holds_alternative<ExprVector>(right->variant));

		auto fnType = left->getType();
		auto &r = std::get<ExprVector>(right->variant);
		std::vector<gen::Reg> param{r.size()};
		for (std::size_t i = 0; i < r.size(); ++i) {
		    param[i] = r[i]->loadValue();
		    // cast parameters
		    if (i < fnType->getArgType().size()) {
			auto fromType = r[i]->getType();
			auto toType = fnType->getArgType()[i];
			param[i] = gen::cast(param[i], fromType, toType);
		    }
		}

		if (std::holds_alternative<Identifier>(left->variant)) {
		    auto l = std::get<Identifier>(left->variant);
		    return gen::call(l.val, param);
		}
		auto fnPtr = left->loadValue();
		return gen::call(fnPtr, fnType, param);
	    }
	case Binary::Kind::ASSIGN:
	    {
		auto addr = binary.left->loadAddr();
		auto val = binary.right->loadValue();
		gen::store(val, addr, binary.type);
		return val;
	    }
	case Binary::Kind::POSTFIX_INC:
	case Binary::Kind::POSTFIX_DEC:
	    {
		gen::Reg val, retVal = nullptr;

		if (binary.kind == Binary::Kind::POSTFIX_INC
		 && binary.type->isPointer())
		{
		    assert(binary.left->getType()->isPointer());
		    assert(binary.right->getType()->isInteger());

		    auto ty = binary.left->getType()->getRefType();
		    retVal = binary.left->loadValue();
		    auto r = binary.right->loadValue();
		    val = gen::ptrInc(ty, retVal, r);
		} else {
		    retVal = binary.left->loadValue();
		    auto r = binary.right->loadValue();
		    auto op = getGenAluOp(binary.kind, binary.type);
		    val = gen::aluInstr(op, retVal, r);
		}
		auto addr = binary.left->loadAddr();
		gen::store(val, addr, binary.type);
		return retVal; // -> return loaded lValue
	    }
	case Binary::Kind::ADD:
	case Binary::Kind::SUB:
	case Binary::Kind::MUL:
	case Binary::Kind::DIV:
	case Binary::Kind::MOD:
	    if (binary.kind == Binary::Kind::ADD && binary.type->isPointer()) {
		assert(binary.left->getType()->isPointer()
			|| binary.left->getType()->isArray()
			|| binary.left->getType()->isFunction());
		assert(binary.right->getType()->isInteger());

		auto ptrTy = binary.left->getType();
		ptrTy = Type::convertArrayOrFunctionToPointer(ptrTy);
		auto refTy = ptrTy->getRefType();
		if (refTy->isFunction() && !ptrTy->isPointer()) {
		    error::out() << binary.opLoc
			<< ": incompatible cast of '" << refTy
			<< "' to integer" << std::endl;
		    refTy = Type::getUnsignedInteger(8);
		}

		auto l = binary.left->getType()->isArray()
			? binary.left->loadAddr()
			: binary.left->loadValue();
		auto r = binary.right->loadValue();
		return gen::ptrInc(refTy, l, r);
	    } else if (binary.kind == Binary::Kind::SUB
		    && binary.left->getType()->isArrayOrPointer()
		    && binary.right->getType()->isArrayOrPointer()) {
		auto ty = binary.left->getType()->getRefType();
		auto l = binary.left->loadValue();
		auto r = binary.right->loadValue();
		return gen::ptrDiff(ty, l, r);
	    } else {
		auto l = binary.left->loadValue();
		auto r = binary.right->loadValue();
		auto op = getGenAluOp(binary.kind, binary.type);
		return gen::aluInstr(op, l, r);
	    }
	case Binary::Kind::LESS:
	case Binary::Kind::LESS_EQUAL:
	case Binary::Kind::GREATER:
	case Binary::Kind::GREATER_EQUAL:
	case Binary::Kind::NOT_EQUAL:
	case Binary::Kind::EQUAL:
	    {
		assert(*binary.left->getType() == *binary.right->getType()
			&& "operand types not casted?");
		auto ty = binary.left->getType();
		auto condOp = getGenCondOp(binary.kind, ty);
		auto l = binary.left->loadValue();
		auto r = binary.right->loadValue();
		return gen::cond(condOp, l, r);
	    }
	case Binary::Kind::LOGICAL_AND:
	case Binary::Kind::LOGICAL_OR:
	    {
		assert(*binary.left->getType() == *binary.right->getType()
			&& "operand types not casted?");
		auto ty = binary.type;
		ty = Type::convertArrayOrFunctionToPointer(ty);

		if (binary.left->isConst() && binary.right->isConst()) {
		    auto val = binary.left->loadConstInt()->getZExtValue();
		    if (val && binary.kind == Binary::Kind::LOGICAL_OR) {
			return gen::loadIntConst(1, ty);
		    }
		    if (!val && binary.kind == Binary::Kind::LOGICAL_AND) {
			return gen::loadIntConst(0, ty);
		    }
		    val = binary.right->loadConstInt()->getZExtValue();
		    if (val) {
			return gen::loadIntConst(1, ty);
		    }
		    return gen::loadIntConst(0, ty);
		} else {
		    auto trueLabel = gen::getLabel("true");
		    auto falseLabel = gen::getLabel("false");
		    auto phiLabel = gen::getLabel("true");

		    condJmp(binary, trueLabel, falseLabel);

		    gen::labelDef(trueLabel);
		    auto one = gen::loadIntConst(1, ty);
		    gen::jmp(phiLabel);

		    gen::labelDef(falseLabel);
		    auto zero = gen::loadIntConst(0, ty);
		    gen::jmp(phiLabel);

		    gen::labelDef(phiLabel);
		    return gen::phi(one, trueLabel, zero, falseLabel, ty);
		}
	    }
	default:
	    error::out() << "binary.kind = " << int(binary.kind) << std::endl;
	    assert(0);
	    return nullptr;
    }
}

static gen::Reg
loadValue(const Conditional &expr)
{
    if (expr.cond->isConst()) {
	auto condVal = expr.cond->loadConst();
	if (!condVal->isZeroValue()) {
	    return expr.left->loadValue();
	} else {
	    return expr.right->loadValue();
	}
    }
    auto thenLabel = gen::getLabel("condTrue");
    auto elseLabel = gen::getLabel("condFalse");
    auto endLabel = gen::getLabel("end");

    expr.cond->condJmp(thenLabel, elseLabel);
    gen::labelDef(thenLabel);
    auto condTrue = expr.left->loadValue();
    thenLabel = gen::jmp(endLabel); // update needed for phi
    gen::labelDef(elseLabel);
    auto condFalse = expr.right->loadValue();
    elseLabel = gen::jmp(endLabel); // update needed for phi
    gen::labelDef(endLabel);
    return gen::phi(condTrue, thenLabel, condFalse, elseLabel, expr.type);
}

gen::Reg
Expr::loadValue(void) const
{
    if (std::holds_alternative<Proxy>(variant)) {
	return std::get<Proxy>(variant).expr->loadValue();
    } else if (std::holds_alternative<Literal>(variant)) {
	return ::loadValue(std::get<Literal>(variant));
    } else if (std::holds_alternative<Identifier>(variant)) {
	return ::loadValue(std::get<Identifier>(variant));
    } else if (std::holds_alternative<Unary>(variant)) {
	return ::loadValue(std::get<Unary>(variant));
    } else if (std::holds_alternative<Binary>(variant)) {
	return ::loadValue(std::get<Binary>(variant));
    } else if (std::holds_alternative<Conditional>(variant)) {
	return ::loadValue(std::get<Conditional>(variant));
    } else if (std::holds_alternative<ExprVector>(variant)) {
	assert(0);
	return nullptr;
    }
    assert(0);
    return nullptr; // never reached
}

static gen::Reg
loadAddr(const Binary &binary)
{
    assert(binary.kind == Binary::MEMBER);
    auto baseAddr = binary.left->loadAddr();
    auto baseType = binary.left->getType();

    const auto &member = std::get<Identifier>(binary.right->variant);
    auto index = baseType->getMemberIndex(member.val);
    return gen::ptrMember(baseType, baseAddr, index);
}

gen::Reg
Expr::loadAddr(void) const
{
    if (!isLValue()) {
	error::out() << getLoc() << ": not a lvalue" << std::endl;
	error::fatal();
    }

    if (std::holds_alternative<Proxy>(variant)) {
	return std::get<Proxy>(variant).expr->loadAddr();
    } else if (std::holds_alternative<Identifier>(variant)) {
	const auto &ident = std::get<Identifier>(variant);
	return gen::loadAddr(ident.val);
    } else if (std::holds_alternative<Unary>(variant)) {
	const auto &unary = std::get<Unary>(variant);
	if (unary.kind == Unary::Kind::DEREF) {
	    return unary.child->loadValue();
	}
    } else if (std::holds_alternative<Binary>(variant)) {
	const auto &binary = std::get<Binary>(variant);
	return ::loadAddr(binary);
    }
    assert(0);
    return nullptr;
}

static void
condJmp(const Unary &unary, gen::Label trueLabel, gen::Label falseLabel)
{
    switch (unary.kind) {
	case Unary::Kind::LOGICAL_NOT:
	    {
		unary.child->condJmp(falseLabel, trueLabel);
		return;
	    }
	default:
	    {
		auto ty = unary.type;
		auto zero = Expr::createNull(ty, unary.opLoc)->loadValue();
		auto cond = gen::cond(gen::NE, loadValue(unary), zero);
		gen::jmp(cond, trueLabel, falseLabel);
	    }
    }
}

static void
condJmp(const Binary &binary, gen::Label trueLabel, gen::Label falseLabel)
{
    switch (binary.kind) {
	case Binary::Kind::LESS:
	case Binary::Kind::LESS_EQUAL:
	case Binary::Kind::GREATER:
	case Binary::Kind::GREATER_EQUAL:
	case Binary::Kind::NOT_EQUAL:
	case Binary::Kind::EQUAL:
	    {
		assert(*binary.left->getType() == *binary.right->getType());
		auto ty = binary.left->getType();

		auto condOp = getGenCondOp(binary.kind, ty);
		auto l = binary.left->loadValue();
		auto r = binary.right->loadValue();
		auto cond = gen::cond(condOp, l, r);
		gen::jmp(cond, trueLabel, falseLabel);
		return;
	    }
	case Binary::Kind::LOGICAL_AND:
            {
		auto chkRightLabel = gen::getLabel("chkRight");

		binary.left->condJmp(chkRightLabel, falseLabel);
		gen::labelDef(chkRightLabel);
		binary.right->condJmp(trueLabel, falseLabel);
		return;
            }
	case Binary::Kind::LOGICAL_OR:
            {
		auto chkRightLabel = gen::getLabel("chkRight");
		
		binary.left->condJmp(trueLabel, chkRightLabel);
		gen::labelDef(chkRightLabel);
		binary.right->condJmp(trueLabel, falseLabel);
		return;
            }
	default:
	    {
		auto ty = binary.type;
		auto zero = Expr::createNull(ty, binary.opLoc)->loadValue();
		auto cond = gen::cond(gen::NE, loadValue(binary), zero);
		gen::jmp(cond, trueLabel, falseLabel);
		return;
	    }
    }
}

void
Expr::condJmp(gen::Label trueLabel, gen::Label falseLabel) const
{
    if (std::holds_alternative<Binary>(variant)) {
	::condJmp(std::get<Binary>(variant), trueLabel, falseLabel);
    } else if (std::holds_alternative<Unary>(variant)) {
	::condJmp(std::get<Unary>(variant), trueLabel, falseLabel);
    } else if (std::holds_alternative<Proxy>(variant)) {
	std::get<Proxy>(variant).expr->condJmp(trueLabel, falseLabel);
    } else {
	auto zero = createNull(getType(), getLoc())->loadValue();
	auto cond = gen::cond(gen::NE, loadValue(), zero);
	gen::jmp(cond, trueLabel, falseLabel);
    }
}


