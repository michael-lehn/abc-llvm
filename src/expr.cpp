#include <cstring>
#include <charconv>
#include <iostream>
#include <type_traits>
#include <variant>

#include "expr.hpp"

//------------------------------------------------------------------------------

void
ExprDeleter::operator()(const Expr *expr) const
{
    delete expr;
}

//-- handling type conversions and casts ---------------------------------------

static const Type *
getTypeConversion(const Type *from, const Type *to)
{
    if (from == to) {
	return to;
    } else if (from->isInteger() && to->isInteger()) {
	// TODO: -Wconversion generate warning if sizeof(to) < sizeof(from)
	return to;
    } else if (from->isPointer() && to->isPointer()) {
	// TODO: require explicit cast if types are different
	return from;
    } else if (from->isFunction() && to->isPointer()) {
	// TODO: require explicit cast if types are different
	if (from != to) {
	    std::cerr << "warning: casting '" << from << "' to '" << to << "'"
		<< std::endl;
	}
	return from;
    }
    std::cerr << "can not convert type '"
	<< from << "' to type '" << to << "'" << std::endl;
    assert(0);
    return nullptr;
}

static const Type *
getCommonType(const Type *left, const Type *right)
{
    if (left == right) {
	return left;
    } else if (left->isInteger() && right->isInteger()) {
	auto size = std::max(left->getIntegerNumBits(),
			     right->getIntegerNumBits());
	// when mixing signed and unsigned: unsigned wins 
	return left->getIntegerKind() == Type::UNSIGNED
	    || right->getIntegerKind() == Type::UNSIGNED
	    ? Type::getUnsignedInteger(size)
	    : Type::getSignedInteger(size);
    }
    return nullptr;
}

//-- class Binary --------------------------------------------------------------

void
Binary::setType(void)
{
    auto l = left->getType(),
	 r = right->getType();

    switch (kind) {
	case Binary::Kind::CALL:
	    type = l->getRetType();
	    return;
	case Binary::Kind::ASSIGN:
	    type = getTypeConversion(r, l);
	    return;
	case Binary::Kind::ADD:
	case Binary::Kind::PREFIX_INC:
	    if (l->isPointer() || r->isPointer()) {
		if (l->isPointer() && r->isInteger()) {
		    type = l;
		} else if (r->isPointer() && l->isInteger()) {
		    type = r;
		} else {
		    type = nullptr;
		}
	    } else {
		type = getCommonType(l, r);
	    }
	    return;
	case Binary::Kind::SUB:
	case Binary::Kind::PREFIX_DEC:
	    if (l->isPointer() && r->isPointer()) {
		type = Type::getSignedInteger(64); // TODO: some ptrdiff_t
	    } else if (l->isPointer() && r->isInteger()) {
		type = l;
	    } else {
		type = getCommonType(l, r);
	    }
	    return;
	case Binary::Kind::MUL:
	case Binary::Kind::DIV:
	case Binary::Kind::MOD:
	    if (l->isInteger() && r->isInteger()) {
		type = getCommonType(l, r);
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
	    type = Type::getUnsignedInteger(8); // TODO: bool type
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
	left->print();
	right->print();
    }
    assert(type && "setTypes() not called?");

    auto l = left->getType(),
	 r = right->getType();

    switch (kind) {
	case Binary::Kind::ASSIGN:
	    if (r != type) {
		right = Expr::createCast(std::move(right), type);
	    }
	    return;
	case Binary::Kind::PREFIX_INC:
	case Binary::Kind::ADD:
	    if (l->isPointer() || r->isPointer()) {
		if (r->isPointer() && l->isInteger()) {
		    // for pointer arithmeitc let right always be the index
		    left.swap(right);
		    std::swap(l, r);
		}
	    } else if (l->isInteger() && r->isInteger()) {
		if (l != type) {
		    left = Expr::createCast(std::move(left), type);
		}
		if (r != type) {
		    right = Expr::createCast(std::move(right), type);
		}
	    }
	case Binary::Kind::PREFIX_DEC:
	case Binary::Kind::SUB:
	    if (l->isInteger() && r->isInteger()) {
		if (l != type) {
		    left = Expr::createCast(std::move(left), type);
		}
		if (r != type) {
		    right = Expr::createCast(std::move(right), type);
		}
	    }
	    return;
	case Binary::Kind::MUL:
	case Binary::Kind::DIV:
	case Binary::Kind::MOD:
	    if (l->isInteger() && r->isInteger()) {
		if (l != type) {
		    left = Expr::createCast(std::move(left), type);
		}
		if (r != type) {
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
	case Binary::Kind::LOGICAL_AND:
	case Binary::Kind::LOGICAL_OR:
	    {
		auto ty = getCommonType(l, r);
		if (l != ty) {
		    left = Expr::createCast(std::move(left), ty);
		}
		if (r != ty) {
		    right = Expr::createCast(std::move(right), ty);
		}
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

    type = getCommonType(l, r);
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
    if (ec == std::errc()) {
	ty = Type::getSignedInteger(numBits);
	return true;
    }
    ty = nullptr; 
    return false;
}

static const Type *
getIntType(const char *s, const char *end, std::uint8_t radix)
{
    const Type *ty;
    if (getIntType<std::int8_t, 8>(s, end, radix, ty)
     || getIntType<std::int16_t, 16>(s, end, radix, ty)
     || getIntType<std::int32_t, 32>(s, end, radix, ty)
     || getIntType<std::int64_t, 64>(s, end, radix, ty))
    {
	return ty;
    }
    std::cerr << "signed integer '" << s << "' does not fit into 64 bits"
	<< std::endl;
    return Type::getSignedInteger(64);
}

ExprPtr
Expr::createLiteral(const char *val, std::uint8_t radix, const Type *type)
{
    if (!type)  {
	type = getIntType(val, val + strlen(val), radix);
    }
    return ExprPtr{new Expr{Literal{val, type, radix}}};
}

ExprPtr
Expr::createIdentifier(const char *ident, const Type *type)
{
    return ExprPtr{new Expr{Identifier{ident, type}}};
}

ExprPtr
Expr::createUnaryMinus(ExprPtr &&expr)
{
    auto zero = createLiteral("0", 10);
    return createBinary(Binary::Kind::SUB, std::move(zero), std::move(expr));
}

ExprPtr
Expr::createLogicalNot(ExprPtr &&expr)
{
    auto ty = expr->getType();
    auto lNot = new Expr{Unary{Unary::Kind::LOGICAL_NOT, std::move(expr), ty}};
    return ExprPtr{lNot};
}

ExprPtr
Expr::createAddr(ExprPtr &&expr)
{
    auto ty = Type::getPointer(expr->getType());
    auto addr = new Expr{Unary{Unary::Kind::ADDRESS, std::move(expr), ty}};
    return ExprPtr{addr};
}

ExprPtr
Expr::createDeref(ExprPtr &&expr)
{
    assert(expr->getType()->isPointer());
    auto ty = expr->getType()->getRefType();
    auto dref = new Expr{Unary{Unary::Kind::DEREF, std::move(expr), ty}};
    return ExprPtr{dref};
}


ExprPtr
Expr::createCast(ExprPtr &&child, const Type *toType)
{
    auto expr = new Expr{Unary{Unary::Kind::CAST, std::move(child), toType}};
    return ExprPtr{expr};
}

ExprPtr
Expr::createBinary(Binary::Kind kind, ExprPtr &&left, ExprPtr &&right)
{
    auto expr = new Expr{Binary{kind, std::move(left), std::move(right)}};
    return ExprPtr{expr};
}

ExprPtr
Expr::createCall(ExprPtr &&fn, ExprVector &&param)
{
    assert(fn->getType()->isFunction());

    auto p = createExprVector(std::move(param));
    auto expr = createBinary(Binary::Kind::CALL, std::move(fn), std::move(p));
    return ExprPtr{std::move(expr)};
}

ExprPtr
Expr::createConditional(ExprPtr &&cond, ExprPtr &&left, ExprPtr &&right)
{
    auto expr = new Expr{Conditional{std::move(cond), std::move(left),
				     std::move(right)}};
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
	default:
	    return "?";
    }
}

void
Expr::print(int indent) const
{
    std::printf("%*s", indent, "");

    if (std::holds_alternative<Literal>(variant)) {
	const auto &lit = std::get<Literal>(variant);
	std::printf("Literal: %s ", lit.val);
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

const Type *
Expr::getType(void) const
{
    if (std::holds_alternative<Literal>(variant)) {
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
    if (std::holds_alternative<Identifier>(variant)) {
	return true;
    } else if (std::holds_alternative<Unary>(variant)) {
	const auto &unary = std::get<Unary>(variant);
	return unary.kind == Unary::Kind::DEREF;
    }
    return false;
}

bool
Expr::isConst(void) const
{
    if (std::holds_alternative<Literal>(variant)) {
	return true;
    } else if (std::holds_alternative<Identifier>(variant)) {
	return false;
    } else if (std::holds_alternative<Binary>(variant)) {
	const auto &binary = std::get<Binary>(variant);
	switch (binary.kind) {
	    case Binary::Kind::CALL:
	    case Binary::Kind::ASSIGN:
		return false;
	    default:
		return binary.left->isConst()
		    && binary.right->isConst();
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
    return llvm::dyn_cast<T>(loadValue());
}

static gen::AluOp
getGenAluOp(Binary::Kind kind, const Type *type)
{
    bool isSignedInt = type->isInteger()
		    && type->getIntegerKind() == Type::SIGNED;
    switch (kind) {
	case Binary::Kind::PREFIX_INC:
	case Binary::Kind::ADD:
	    return gen::ADD;
	case Binary::Kind::PREFIX_DEC:
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
	default:
	    assert(0);
	    return gen::EQ;
    }
}

static gen::Reg
loadValue(const Literal &l)
{
    return gen::loadConst(l.val, l.type, l.radix);
}

static gen::Reg
loadValue(const Identifier &ident)
{
    if (ident.type->isFunction()) {
	return gen::loadAddr(ident.val);
    }

    return gen::fetch(ident.val, ident.type);
}

static void condJmp(const Unary &, gen::Label, gen::Label);

static gen::Reg
loadValue(const Unary &unary)
{
    switch (unary.kind) {
	case Unary::Kind::CAST:
	    return gen::cast(unary.child->loadValue(), unary.child->getType(),
			     unary.type);
	case Unary::Kind::LOGICAL_NOT:
	    {
		auto thenLabel = gen::getLabel("then");
		auto elseLabel = gen::getLabel("else");
		auto endLabel = gen::getLabel("end");

		condJmp(unary, thenLabel, elseLabel);
		gen::labelDef(thenLabel);
		auto one = Expr::createLiteral("1", 10)->loadValue();
		gen::jmp(endLabel);
		gen::labelDef(elseLabel);
		auto zero = Expr::createLiteral("0", 10)->loadValue();
		gen::jmp(endLabel);
		gen::labelDef(endLabel);
		return gen::phi(one, thenLabel, zero, elseLabel, unary.type);
	    }
	case Unary::Kind::DEREF:
	    {
		auto addr = unary.child->loadValue();
		if (unary.type->isFunction()) {
		    return addr;
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

static gen::Reg
loadValue(const Binary &binary)
{
    switch (binary.kind) {
	case Binary::Kind::CALL:
	    {
		auto const &left = binary.left;
		auto const &right = binary.right;
		assert(std::holds_alternative<ExprVector>(right->variant));

		auto &r = std::get<ExprVector>(right->variant);
		std::vector<gen::Reg> param{r.size()};
		for (std::size_t i = 0; i < r.size(); ++i) {
		    param[i] = r[i]->loadValue();
		}

		if (std::holds_alternative<Identifier>(left->variant)) {
		    auto l = std::get<Identifier>(left->variant);
		    return gen::call(l.val, param);
		}
		auto fnPtr = left->loadValue();
		auto fnType = left->getType();
		return gen::call(fnPtr, fnType, param);
	    }

	case Binary::Kind::ASSIGN:
	    {
		auto addr = binary.left->loadAddr();
		auto val = binary.right->loadValue();
		gen::store(val, addr, binary.type);
		return val;
	    }
	case Binary::Kind::PREFIX_INC:
	case Binary::Kind::PREFIX_DEC:
	    {
		gen::Reg val;
		if (binary.kind == Binary::Kind::PREFIX_INC
		 && binary.type->isPointer())
		{
		    assert(binary.left->getType()->isPointer());
		    assert(binary.right->getType()->isInteger());

		    auto ty = binary.left->getType()->getRefType();
		    auto l = binary.left->loadValue();
		    auto r = binary.right->loadValue();
		    val = gen::ptrInc(ty, l, r);
		} else {
		    auto l = binary.left->loadValue();
		    auto r = binary.right->loadValue();
		    auto op = getGenAluOp(binary.kind, binary.type);
		    val = gen::aluInstr(op, l, r);
		}
		auto addr = binary.left->loadAddr();
		gen::store(val, addr, binary.type);
		return val;
	    }

	case Binary::Kind::ADD:
	case Binary::Kind::SUB:
	case Binary::Kind::MUL:
	case Binary::Kind::DIV:
	case Binary::Kind::MOD:
	    if (binary.kind == Binary::Kind::ADD && binary.type->isPointer()) {
		assert(binary.left->getType()->isPointer());
		assert(binary.right->getType()->isInteger());

		auto ty = binary.left->getType()->getRefType();
		auto l = binary.left->loadValue();
		auto r = binary.right->loadValue();
		return gen::ptrInc(ty, l, r);
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
	case Binary::Kind::LOGICAL_AND:
	case Binary::Kind::LOGICAL_OR:
	    {
		auto thenLabel = gen::getLabel("then");
		auto elseLabel = gen::getLabel("else");
		auto endLabel = gen::getLabel("end");

		condJmp(binary, thenLabel, elseLabel);
		gen::labelDef(thenLabel);
		auto one = Expr::createLiteral("1", 10)->loadValue();
		gen::jmp(endLabel);
		gen::labelDef(elseLabel);
		auto zero = Expr::createLiteral("0", 10)->loadValue();
		gen::jmp(endLabel);
		gen::labelDef(endLabel);
		return gen::phi(one, thenLabel, zero, elseLabel, binary.type);
	    }

	default:
	    std::cerr << "binary.kind = " << int(binary.kind) << std::endl;
	    assert(0);
	    return nullptr;
    }
}

static gen::Reg
loadValue(const Conditional &expr)
{
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
    if (std::holds_alternative<Literal>(variant)) {
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

gen::Reg
Expr::loadAddr(void) const
{
    assert(isLValue());

    if (std::holds_alternative<Identifier>(variant)) {
	const auto &ident = std::get<Identifier>(variant);
	return gen::loadAddr(ident.val);
    } else if (std::holds_alternative<Unary>(variant)) {
	const auto &unary = std::get<Unary>(variant);
	if (unary.kind == Unary::Kind::DEREF) {
	    return unary.child->loadValue();
	}
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
		auto zero = Expr::createLiteral("0", 10, ty)->loadValue();
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
		assert(binary.left->getType() == binary.right->getType());
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
		auto zero = Expr::createLiteral("0", 10, ty)->loadValue();
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
    } else {
	auto zero = createLiteral("0", 10, getType())->loadValue();
	auto cond = gen::cond(gen::NE, loadValue(), zero);
	gen::jmp(cond, trueLabel, falseLabel);
    }
}


