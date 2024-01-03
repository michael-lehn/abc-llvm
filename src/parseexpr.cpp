#include <unordered_map>

#include "lexer.hpp"
#include "parser.hpp"
#include "parseexpr.hpp"
#include "symtab.hpp"

static bool parseAssignment(void);
static bool parseBinary(int prec);
static bool parseUnary(void);
static bool parsePrimary(void);

bool
parseExpr(void)
{
    return parseAssignment();
}

static bool
parseAssignment(void)
{
    if (!parseBinary(1)) {
	return false;
    }
    while (token.kind == TokenKind::EQUAL) {
	getToken();
	if (!parseAssignment()) {
	    expectedError("assignment expression");
	}
    }
    return true;
}

static std::unordered_map<TokenKind, int> binaryOpPrec = {
    { TokenKind::ASTERISK, 13},
    { TokenKind::SLASH, 13},
    { TokenKind::PERCENT, 13},

    { TokenKind::PLUS, 11},
    { TokenKind::MINUS, 11},

    { TokenKind::LESS, 10},
    { TokenKind::LESS_EQUAL, 10},
    { TokenKind::GREATER, 10},
    { TokenKind::GREATER_EQUAL, 10},
    
    { TokenKind::EQUAL2, 9},
    { TokenKind::NOT_EQUAL, 9},
};

static int
tokenKindPrec(TokenKind kind)
{
    return binaryOpPrec.contains(kind) ? binaryOpPrec[kind] : 0;
}

static bool
parseBinary(int prec)
{
    if (!parseUnary()) {
	return false;
    }
    for (int p = tokenKindPrec(token.kind); p >= prec; --p) {
        while (tokenKindPrec(token.kind) == p) {
            getToken();
            if (!parseBinary(p + 1)) {
                expectedError("non-empty expression");
            }
        }
    }
    return true;
}

static bool
parseUnary(void)
{
    if (token.kind == TokenKind::PLUS || token.kind == TokenKind::MINUS
     || token.kind == TokenKind::NOT)
    {
        getToken();
        if (!parseUnary()) {
            expectedError("non-empty expression");
        }
    }
    return parsePrimary();
} 

static bool
parsePrimary(void)
{
    if (token.kind == TokenKind::IDENTIFIER) {
	if (!symtab::get(token.val.c_str())) {
	    std::string msg = "undeclared identifier '";
	    msg += token.val.c_str();
	    msg += "'";
	    semanticError(msg.c_str());
	}
        getToken();
        return true;
    } else if (token.kind == TokenKind::DECIMAL_LITERAL) {
        getToken();
        return true;
    } else if (token.kind == TokenKind::HEXADECIMAL_LITERAL) {
        getToken();
        return true;
    } else if (token.kind == TokenKind::OCTAL_LITERAL) {
        getToken();
        return true;
    } else if (token.kind == TokenKind::LPAREN) {
        getToken();
        if (!parseAssignment()) {
            expectedError("expression");
	}
        expected(TokenKind::RPAREN);
        getToken();
        return true;
    }
    return false;
} 
