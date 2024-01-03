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

static gen::Op
getGenOp(ExprKind kind)
{
    switch (kind) {
	case ExprKind::ADD:
	    return gen::Add;
	case ExprKind::SUB:
	    return gen::Sub;
	default:
	    assert(0);
	    return gen::Add;
    }
}

gen::Reg *
load(const Expr *expr)
{
    if (!expr) {
	return nullptr;
    } else if (std::holds_alternative<Literal>(expr->variant)) {
	const auto &lit = std::get<Literal>(expr->variant);
	return gen::loadConst(lit.val, Type::getUnsignedInteger(64));
    } else if (std::holds_alternative<Identifier>(expr->variant)) {
	const auto &ident = std::get<Identifier>(expr->variant);
	return gen::fetch(ident.val, Type::getUnsignedInteger(64));
    } else if (std::holds_alternative<Binary>(expr->variant)) {
	const auto &binary = std::get<Binary>(expr->variant);
	auto l = load(binary.left.get());
	auto r = load(binary.right.get());
	auto op = getGenOp(binary.kind);
	return gen::op2r(op, l, r);
    }
    assert(0);
    return nullptr; // never reached
}
