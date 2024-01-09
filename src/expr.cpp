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

//-- class Expr: static member functions ---------------------------------------

ExprPtr
Expr::createLiteral(const char *val, std::uint8_t radix, const Type *ty)
{
    return ExprPtr(new Expr{Literal{val}});
}

ExprPtr
Expr::createIdentifier(const char *ident, const Type *ty)
{
    return ExprPtr(new Expr{Identifier{ident}});
}

ExprPtr
Expr::createUnaryMinus(ExprPtr &&expr)
{
    auto zero = createLiteral("0", 10, Type::getUnsignedInteger(64));
    return createBinary(Binary::Kind::SUB, std::move(zero), std::move(expr));
}

ExprPtr
Expr::createBinary(Binary::Kind kind, ExprPtr &&left, ExprPtr &&right)
{
    auto expr = new Expr{Binary{kind, std::move(left), std::move(right)}};
    return ExprPtr(expr);
}

ExprPtr
Expr::createCall(ExprPtr &&fn, ExprVector &&param)
{
    auto p = createExprVector(std::move(param));
    auto expr = createBinary(Binary::Kind::CALL, std::move(fn), std::move(p));
    return ExprPtr(std::move(expr));
}

ExprPtr
Expr::createExprVector(ExprVector &&expr)
{
    return ExprPtr{new Expr{std::move(expr)}};
}

//-- methods -------------------------------------------------------------------

static const char *
exprKindCStr(Binary::Kind kind)
{
    switch (kind) {
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
	std::printf("Literal: %s\n", lit.val);
    } else if (std::holds_alternative<Identifier>(variant)) {
	const auto &ident = std::get<Identifier>(variant);
	std::printf("Identifier: %s\n", ident.val);
    } else if (std::holds_alternative<Binary>(variant)) {
	const auto &binary = std::get<Binary>(variant);
	std::printf("[%s]\n", exprKindCStr(binary.kind));
	binary.left->print(indent + 4);
	binary.right->print(indent + 4);
    }
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

// TODO: for div/mod the type is needed
static gen::AluOp
getGenAluOp(Binary::Kind kind)
{
    switch (kind) {
	case Binary::Kind::ADD:
	    return gen::ADD;
	case Binary::Kind::SUB:
	    return gen::SUB;
	case Binary::Kind::MUL:
	    return gen::SMUL;
	case Binary::Kind::DIV:
	    return gen::UDIV;
	case Binary::Kind::MOD:
	    return gen::UMOD;
	default:
	    assert(0);
	    return gen::ADD;
    }
}

// TODO: for >,<,>=,<= the type is needed
static gen::CondOp
getGenCondOp(Binary::Kind kind)
{
    switch (kind) {
	case Binary::Kind::EQUAL:
	    return gen::EQ;
	case Binary::Kind::NOT_EQUAL:
	    return gen::NE;
	case Binary::Kind::LESS:
	    return gen::ULT;
	case Binary::Kind::LESS_EQUAL:
	    return gen::ULE;
	case Binary::Kind::GREATER:
	    return gen::UGT;
	case Binary::Kind::GREATER_EQUAL:
	    return gen::UGE;
	default:
	    assert(0);
	    return gen::EQ;
    }
}

static gen::Reg
loadValue(const Literal &l)
{
    return gen::loadConst(l.val, Type::getUnsignedInteger(64));
}

static gen::Reg
loadValue(const Identifier &ident)
{
    return gen::fetch(ident.val, Type::getUnsignedInteger(64));
}

static void condJmp(const Binary &, gen::Label, gen::Label);

static gen::Reg
loadValue(const Binary &binary)
{
    switch (binary.kind) {
	case Binary::Kind::CALL:
	    {
		auto l = std::get<Identifier>(binary.left->variant);
		auto &r = std::get<ExprVector>(binary.right->variant);
		std::vector<gen::Reg> param{r.size()};
		for (std::size_t i = 0; i < r.size(); ++i) {
		    param[i] = r[i]->loadValue();
		}
		return gen::call(l.val, param);
	    }

	case Binary::Kind::ASSIGN:
	    {
		auto l = std::get<Identifier>(binary.left->variant);
		auto r = binary.right->loadValue();
		gen::store(r, l.val, Type::getUnsignedInteger(64));
		return r;
	    }
	case Binary::Kind::ADD:
	case Binary::Kind::SUB:
	case Binary::Kind::MUL:
	case Binary::Kind::DIV:
	case Binary::Kind::MOD:
	    {
		auto l = binary.left->loadValue();
		auto r = binary.right->loadValue();
		auto op = getGenAluOp(binary.kind);
		return gen::aluInstr(op, l, r);
	    }
	case Binary::Kind::LESS:
	case Binary::Kind::LESS_EQUAL:
	case Binary::Kind::GREATER:
	case Binary::Kind::GREATER_EQUAL:
	case Binary::Kind::NOT_EQUAL:
	case Binary::Kind::EQUAL:
	    {
		auto ty =  Type::getUnsignedInteger(64);

		auto thenLabel = gen::getLabel("then");
		auto elseLabel = gen::getLabel("else");
		auto endLabel = gen::getLabel("end");

		condJmp(binary, thenLabel, elseLabel);
		gen::labelDef(thenLabel);
		auto r1 = gen::loadConst("1", ty);
		gen::jmp(endLabel);
		gen::labelDef(elseLabel);
		auto r2 = gen::loadConst("0", ty);
		gen::jmp(endLabel);
		gen::labelDef(endLabel);
		return gen::phi(r1, thenLabel, r2, elseLabel, ty);
	    }
	default:
	    std::cerr << "binary.kind = " << int(binary.kind) << std::endl;
	    assert(0);
	    return nullptr;
    }
}

gen::Reg
Expr::loadValue(void) const
{
    if (std::holds_alternative<Literal>(variant)) {
	return ::loadValue(std::get<Literal>(variant));
    } else if (std::holds_alternative<Identifier>(variant)) {
	return ::loadValue(std::get<Identifier>(variant));
    } else if (std::holds_alternative<Binary>(variant)) {
	return ::loadValue(std::get<Binary>(variant));
    } else if (std::holds_alternative<ExprVector>(variant)) {
	assert(0);
	return nullptr;
    }
    assert(0);
    return nullptr; // never reached
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
		auto condOp = getGenCondOp(binary.kind);
		auto l = binary.left->loadValue();
		auto r = binary.right->loadValue();
		auto cond = gen::cond(condOp, l, r);
		gen::jmp(cond, trueLabel, falseLabel);
		return;
	    }
	default:
	    {
		auto ty =  Type::getUnsignedInteger(64);
		auto zero = gen::loadConst("0", ty);
		auto cond = gen::cond(gen::NE, loadValue(binary), zero);
		gen::jmp(cond, trueLabel, falseLabel);
	    }
    }
}

void
Expr::condJmp(gen::Label trueLabel, gen::Label falseLabel) const
{
    if (std::holds_alternative<Binary>(variant)) {
	::condJmp(std::get<Binary>(variant), trueLabel, falseLabel);
    } else {
	auto ty =  Type::getUnsignedInteger(64);
	auto zero = gen::loadConst("0", ty);
	auto cond = gen::cond(gen::NE, loadValue(), zero);
	gen::jmp(cond, trueLabel, falseLabel);
    }
}
