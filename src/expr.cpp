#include <iostream>
#include <type_traits>
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
    BinaryExprKind kind;
    ExprPtr left, right;

    Binary(BinaryExprKind kind, ExprPtr &&left, ExprPtr &&right)
	: kind{kind}, left{std::move(left)}, right{std::move(right)}
    {}
};

struct Expr
{
    std::variant<Literal, Identifier, Binary, ExprVector> variant;

    Expr(Literal &&val) : variant{std::move(val)} {}
    Expr(Identifier &&ident) : variant{std::move(ident)} {}
    Expr(Binary &&binary) : variant{std::move(binary)} {}
    Expr(ExprVector &&vec) : variant{std::move(vec)} {}

};

void
ExprDeleter::operator()(const Expr *expr) const
{
    delete expr;
};

ExprPtr
getLiteralExpr(const char *val)
{
    return ExprPtr(new Expr{Literal{val}});
}

ExprPtr
getIdentifierExpr(const char *ident)
{
    return ExprPtr(new Expr{Identifier{ident}});
}

ExprPtr
getUnaryMinusExpr(ExprPtr &&expr)
{
    auto zero = getLiteralExpr("0");
    return getBinaryExpr(BinaryExprKind::SUB, std::move(zero),
			 std::move(expr));
}

ExprPtr
getBinaryExpr(BinaryExprKind kind, ExprPtr &&left, ExprPtr &&right)
{
    auto expr = new Expr{Binary{kind, std::move(left), std::move(right)}};
    return ExprPtr(expr);
}

ExprPtr
getCallExpr(ExprPtr &&fn, ExprVector &&param)
{
    auto p = getExprVector(std::move(param));
    auto expr = getBinaryExpr(BinaryExprKind::CALL,
			      std::move(fn), std::move(p));
    return ExprPtr(std::move(expr));
}

ExprPtr
getExprVector(ExprVector &&expr)
{
    return ExprPtr{new Expr{std::move(expr)}};
}

static const char *
exprKindCStr(BinaryExprKind kind)
{
    switch (kind) {
	case BinaryExprKind::ADD:
	    return "+";
	case BinaryExprKind::ASSIGN:
	    return "=";
	case BinaryExprKind::EQUAL:
	    return "==";
	case BinaryExprKind::NOT_EQUAL:
	    return "!=";
	case BinaryExprKind::GREATER:
	    return ">";
	case BinaryExprKind::GREATER_EQUAL:
	    return ">=";
	case BinaryExprKind::LESS:
	    return "<";
	case BinaryExprKind::LESS_EQUAL:
	    return "<=";
	case BinaryExprKind::LOGICAL_AND:
	    return "&&";
	case BinaryExprKind::LOGICAL_OR:
	    return "||";
	case BinaryExprKind::SUB:
	    return "-";
	case BinaryExprKind::MUL:
	    return "*";
	case BinaryExprKind::DIV:
	    return "/";
	case BinaryExprKind::MOD:
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

bool
isConst(const Expr *expr)
{
    if (std::holds_alternative<Literal>(expr->variant)) {
	return true;
    } else if (std::holds_alternative<Identifier>(expr->variant)) {
	return false;
    } else if (std::holds_alternative<Binary>(expr->variant)) {
	const auto &binary = std::get<Binary>(expr->variant);
	switch (binary.kind) {
	    case BinaryExprKind::CALL:
	    case BinaryExprKind::ASSIGN:
		return false;
	    default:
		return isConst(binary.left.get())
		    && isConst(binary.right.get());
	}
    }
    assert(0);
    return false;
}

gen::ConstVal
getConst(const Expr *expr)
{
    if (!expr) {
	return nullptr;
    }

    assert(isConst(expr));

    return llvm::dyn_cast<std::remove_pointer_t<gen::ConstVal>>(load(expr));
}

// TODO: for div/mod the type is needed
static gen::AluOp
getGenAluOp(BinaryExprKind kind)
{
    switch (kind) {
	case BinaryExprKind::ADD:
	    return gen::ADD;
	case BinaryExprKind::SUB:
	    return gen::SUB;
	case BinaryExprKind::MUL:
	    return gen::SMUL;
	case BinaryExprKind::DIV:
	    return gen::UDIV;
	case BinaryExprKind::MOD:
	    return gen::UMOD;
	default:
	    assert(0);
	    return gen::ADD;
    }
}

// TODO: for >,<,>=,<= the type is needed
static gen::CondOp
getGenCondOp(BinaryExprKind kind)
{
    switch (kind) {
	case BinaryExprKind::EQUAL:
	    return gen::EQ;
	case BinaryExprKind::NOT_EQUAL:
	    return gen::NE;
	case BinaryExprKind::LESS:
	    return gen::ULT;
	case BinaryExprKind::LESS_EQUAL:
	    return gen::ULE;
	case BinaryExprKind::GREATER:
	    return gen::UGT;
	case BinaryExprKind::GREATER_EQUAL:
	    return gen::UGE;
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

static void condJmp(const Binary &, gen::Label, gen::Label);

static gen::Reg
load(const Binary &binary)
{
    switch (binary.kind) {
	case BinaryExprKind::CALL:
	    {
		auto l = std::get<Identifier>(binary.left.get()->variant);
		auto &r = std::get<ExprVector>(binary.right.get()->variant);
		std::vector<gen::Reg> param{r.size()};
		for (std::size_t i = 0; i < r.size(); ++i) {
		    param[i] = load(r[i].get());
		}
		return gen::call(l.val, param);
	    }

	case BinaryExprKind::ASSIGN:
	    {
		auto l = std::get<Identifier>(binary.left.get()->variant);
		auto r = load(binary.right.get());
		gen::store(r, l.val, Type::getUnsignedInteger(64));
		return r;
	    }
	case BinaryExprKind::ADD:
	case BinaryExprKind::SUB:
	case BinaryExprKind::MUL:
	case BinaryExprKind::DIV:
	case BinaryExprKind::MOD:
	    {
		auto l = load(binary.left.get());
		auto r = load(binary.right.get());
		auto op = getGenAluOp(binary.kind);
		return gen::aluInstr(op, l, r);
	    }
	case BinaryExprKind::LESS:
	case BinaryExprKind::LESS_EQUAL:
	case BinaryExprKind::GREATER:
	case BinaryExprKind::GREATER_EQUAL:
	case BinaryExprKind::NOT_EQUAL:
	case BinaryExprKind::EQUAL:
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
load(const Expr *expr)
{
    assert(expr);

    if (std::holds_alternative<Literal>(expr->variant)) {
	return load(std::get<Literal>(expr->variant));
    } else if (std::holds_alternative<Identifier>(expr->variant)) {
	return load(std::get<Identifier>(expr->variant));
    } else if (std::holds_alternative<Binary>(expr->variant)) {
	return load(std::get<Binary>(expr->variant));
    } else if (std::holds_alternative<ExprVector>(expr->variant)) {
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
	case BinaryExprKind::LESS:
	case BinaryExprKind::LESS_EQUAL:
	case BinaryExprKind::GREATER:
	case BinaryExprKind::GREATER_EQUAL:
	case BinaryExprKind::NOT_EQUAL:
	case BinaryExprKind::EQUAL:
	    {
		auto condOp = getGenCondOp(binary.kind);
		auto l = load(binary.left.get());
		auto r = load(binary.right.get());
		auto cond = gen::cond(condOp, l, r);
		gen::jmp(cond, trueLabel, falseLabel);
		return;
	    }
	default:
	    {
		auto ty =  Type::getUnsignedInteger(64);
		auto zero = gen::loadConst("0", ty);
		auto cond = gen::cond(gen::NE, load(binary), zero);
		gen::jmp(cond, trueLabel, falseLabel);
	    }
    }
}

void
condJmp(const Expr *expr, gen::Label trueLabel, gen::Label falseLabel)
{
    assert(expr);

    if (std::holds_alternative<Binary>(expr->variant)) {
	condJmp(std::get<Binary>(expr->variant), trueLabel, falseLabel);
    } else {
	auto ty =  Type::getUnsignedInteger(64);
	auto zero = gen::loadConst("0", ty);
	auto cond = gen::cond(gen::NE, load(expr), zero);
	gen::jmp(cond, trueLabel, falseLabel);
    }
}
