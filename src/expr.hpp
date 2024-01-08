#ifndef EXPR_HPP
#define EXPR_HPP

#include <string>
#include <memory>

#include "gen.hpp"

enum class BinaryExprKind
{
    CALL,
    ADD,
    ASSIGN,
    EQUAL,
    NOT_EQUAL,
    GREATER,
    GREATER_EQUAL,
    LESS,
    LESS_EQUAL,
    LOGICAL_AND,
    LOGICAL_OR,
    SUB,
    MUL,
    DIV,
    MOD,
};

struct Expr;

struct ExprDeleter
{
    void operator()(const Expr *) const;
};

using ExprPtr = std::unique_ptr<const Expr, ExprDeleter>;

using ExprVector = std::vector<ExprPtr>;
using ExprVectorPtr = std::unique_ptr<ExprVector>;

ExprPtr getLiteralExpr(const char *val);
ExprPtr getIdentifierExpr(const char *ident);
ExprPtr getUnaryMinusExpr(ExprPtr &&expr);
ExprPtr getBinaryExpr(BinaryExprKind kind, ExprPtr &&left, ExprPtr &&right);
ExprPtr getCallExpr(ExprPtr &&fn, ExprVector &&param);
ExprPtr getExprVector(ExprVector &&expr);

void print(const Expr *expr, int indent = 0);

bool isConst(const Expr *expr);
gen::ConstVal getConst(const Expr *expr);

gen::Reg load(const Expr *expr);
void condJmp(const Expr *expr, gen::Label trueLabel, gen::Label falseLabel);

#endif // EXPR_HPP
