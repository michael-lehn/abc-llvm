#ifndef EXPR_HPP
#define EXPR_HPP

#include <string>
#include <memory>

#include "gen.hpp"

enum class ExprKind
{
    // binary expression
    BINARY,
    ADD = BINARY,
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
    BINARY_END,

    // primary expression
    PRIMARY = BINARY_END,
    INTEGER_LITERAL = PRIMARY,
    STRING_LITERAL,
    IDENTIFIER,
    PRIMARY_END,
};

struct Expr;

struct ExprDeleter
{
    void operator()(Expr *) const;
};

using ExprUniquePtr = std::unique_ptr<Expr, ExprDeleter>;

ExprUniquePtr makeLiteralExpr(const char *val);
ExprUniquePtr makeIdentifierExpr(const char *val);
ExprUniquePtr makeUnaryMinusExpr(ExprUniquePtr &&expr);
ExprUniquePtr makeBinaryExpr(ExprKind kind, ExprUniquePtr &&left,
			     ExprUniquePtr &&right);

void print(const Expr *expr, int indent = 0);
gen::Reg *load(const Expr *expr);

#endif // EXPR_HPP
