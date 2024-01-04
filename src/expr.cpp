#include <iostream>
#include <variant>

#include "expr.hpp"

struct Literal
{
    const char *val;

    Literal(const char *val) : val{val} {}
};

struct Identifier
{
    const char *val;

    Identifier(const char *val) : val{val} {}
};

struct Binary
{
    ExprKind kind;
    ExprUniquePtr left, right;

    Binary(ExprKind kind, ExprUniquePtr &&left, ExprUniquePtr &&right)
	: kind{kind}, left{std::move(left)}, right{std::move(right)}
    {}
};

struct Expr
{
    std::variant<Literal, Identifier, Binary> variant;

    Expr(Literal &&val) : variant{std::move(val)} {}
    Expr(Identifier &&ident) : variant{std::move(ident)} {}
    Expr(Binary &&binary) : variant{std::move(binary)} {}
};

void
ExprDeleter::operator()(Expr *expr) const
{
    delete expr;
};

ExprUniquePtr
makeLiteralExpr(const char *val)
{
    return ExprUniquePtr(new Expr{Literal{val}});
}

ExprUniquePtr
makeIdentifierExpr(const char *val)
{
    return ExprUniquePtr(new Expr{Identifier{val}});
}

ExprUniquePtr
makeUnaryMinusExpr(ExprUniquePtr &&expr)
{
    auto zero = makeLiteralExpr("0");
    return makeBinaryExpr(ExprKind::SUB, std::move(zero), std::move(expr));
}

ExprUniquePtr
makeBinaryExpr(ExprKind kind, ExprUniquePtr &&left, ExprUniquePtr &&right)
{
    auto expr = new Expr{Binary{kind, std::move(left), std::move(right)}};
    return ExprUniquePtr(expr);
}

static const char *
exprKindCStr(ExprKind kind)
{
    switch (kind) {
	case ExprKind::ADD:
	    return "+";
	case ExprKind::ASSIGN:
	    return "=";
	case ExprKind::EQUAL:
	    return "==";
	case ExprKind::NOT_EQUAL:
	    return "!=";
	case ExprKind::GREATER:
	    return ">";
	case ExprKind::GREATER_EQUAL:
	    return ">=";
	case ExprKind::LESS:
	    return "<";
	case ExprKind::LESS_EQUAL:
	    return "<=";
	case ExprKind::LOGICAL_AND:
	    return "&&";
	case ExprKind::LOGICAL_OR:
	    return "||";
	case ExprKind::SUB:
	    return "-";
	case ExprKind::MUL:
	    return "*";
	case ExprKind::DIV:
	    return "/";
	case ExprKind::MOD:
	    return "%";
	default:
	    return "?";
    }
}

void
print(const Expr *expr, int indent)
{
    if (!expr) {
	std::printf("empty expression\n");
	return;
    }
    std::printf("%*s", indent, "");

    if (std::holds_alternative<Literal>(expr->variant)) {
	const auto &lit = std::get<Literal>(expr->variant);
	std::printf("Literal: %s\n", lit.val);
    } else if (std::holds_alternative<Identifier>(expr->variant)) {
	const auto &ident = std::get<Identifier>(expr->variant);
	std::printf("Identifier: %s\n", ident.val);
    } else if (std::holds_alternative<Binary>(expr->variant)) {
	const auto &binary = std::get<Binary>(expr->variant);
	std::printf("[%s]\n", exprKindCStr(binary.kind));
	print(binary.left.get(), indent + 4);
	print(binary.right.get(), indent + 4);
    }
}

// TODO: for div/mod the type is needed
static gen::AluOp
getGenAluOp(ExprKind kind)
{
    std::printf("getGenAluOp: [%s]\n", exprKindCStr(kind));
    switch (kind) {
	case ExprKind::ADD:
	    return gen::ADD;
	case ExprKind::SUB:
	    return gen::SUB;
	case ExprKind::MUL:
	    return gen::SMUL;
	case ExprKind::DIV:
	    return gen::UDIV;
	case ExprKind::MOD:
	    return gen::UMOD;
	default:
	    assert(0);
	    return gen::ADD;
    }
}

static gen::CondOp
getGenCondOp(ExprKind kind)
{
    std::printf("getGenCondOp: [%s]\n", exprKindCStr(kind));
    switch (kind) {
	case ExprKind::EQUAL:
	    return gen::EQ;
	case ExprKind::NOT_EQUAL:
	    return gen::NE;
	default:
	    assert(0);
	    return gen::EQ;
    }
}


static gen::Reg
load(const Literal &l)
{
    return gen::loadConst(l.val, Type::getUnsignedInteger(64));
}

static gen::Reg
load(const Identifier &ident)
{
    return gen::fetch(ident.val, Type::getUnsignedInteger(64));
}

static gen::Reg
load(const Binary &binary)
{
    switch (binary.kind) {
	case ExprKind::ASSIGN:
	    {
		auto r = load(binary.right.get());
		auto l = std::get<Identifier>(binary.left.get()->variant);
		gen::store(r, l.val, Type::getUnsignedInteger(64));
		return r;
	    }
	case ExprKind::ADD:
	case ExprKind::SUB:
	case ExprKind::MUL:
	case ExprKind::DIV:
	case ExprKind::MOD:
	    {
		auto l = load(binary.left.get());
		auto r = load(binary.right.get());
		auto op = getGenAluOp(binary.kind);
		return gen::aluInstr(op, l, r);
	    }
	case ExprKind::NOT_EQUAL:
	case ExprKind::EQUAL:
	    {
		auto ty =  Type::getUnsignedInteger(64);

		auto thenLabel = gen::getLabel("then");
		auto elseLabel = gen::getLabel("else");
		auto endLabel = gen::getLabel("end");

		auto l = load(binary.left.get());
		auto r = load(binary.right.get());
		auto cond = gen::cond(getGenCondOp(binary.kind), l, r);
		gen::jmp(cond, thenLabel, elseLabel);
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
load(const Expr *expr)
{
    if (!expr) {
	return nullptr;
    } else if (std::holds_alternative<Literal>(expr->variant)) {
	return load(std::get<Literal>(expr->variant));
    } else if (std::holds_alternative<Identifier>(expr->variant)) {
	return load(std::get<Identifier>(expr->variant));
    } else if (std::holds_alternative<Binary>(expr->variant)) {
	return load(std::get<Binary>(expr->variant));
    }
    assert(0);
    return nullptr; // never reached
}
