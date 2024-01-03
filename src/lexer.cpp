#include <cassert>
#include <string>
#include <unordered_map>

#include "lexer.hpp"

Token token;

const char *
tokenCStr(TokenKind kind)
{
    switch (kind) {
	case TokenKind::BAD:
	    return "BAD";
	case TokenKind::EOI:
	    return "EOI";
	case TokenKind::IDENTIFIER:
	    return "IDENTIFIER";
	case TokenKind::DECIMAL_LITERAL:
	    return "DECIMAL_LITERAL";
	case TokenKind::HEXADECIMAL_LITERAL:
	    return "HEXADECIMAL_LITERAL";
	case TokenKind::OCTAL_LITERAL:
	    return "OCTAL_LITERAL";
	case TokenKind::STRING_LITERAL:
	    return "STRING_LITERAL";
	case TokenKind::I8:
	    return "I8";
	case TokenKind::I16:
	    return "I16";
	case TokenKind::I32:
	    return "I32";
	case TokenKind::I64:
	    return "I64";
	case TokenKind::U8:
	    return "U8";
	case TokenKind::U16:
	    return "U16";
	case TokenKind::U32:
	    return "U32";
	case TokenKind::U64:
	    return "U64";
	case TokenKind::FN:
	    return "FN";
	case TokenKind::RETURN:
	    return "RETURN";
	case TokenKind::DECL:
	    return "DECL";
	case TokenKind::GLOBAL:
	    return "GLOBAL";
	case TokenKind::LOCAL:
	    return "LOCAL";
	case TokenKind::STATIC:
	    return "STATIC";
	case TokenKind::FOR:
	    return "FOR";
	case TokenKind::SEMICOLON:
	    return "SEMICOLON";
	case TokenKind::COLON:
	    return "COLON";
	case TokenKind::COMMA:
	    return "COMMA";
	case TokenKind::LBRACE:
	    return "LBRACE";
	case TokenKind::RBRACE:
	    return "RBRACE";
	case TokenKind::LPAREN:
	    return "LPAREN";
	case TokenKind::RPAREN:
	    return "RPAREN";
	case TokenKind::LBRACKET:
	    return "LBRACKET";
	case TokenKind::RBRACKET:
	    return "RBRACKET";
	case TokenKind::CARET:
	    return "CARET";
	case TokenKind::PLUS:
	    return "PLUS";
	case TokenKind::PLUS2:
	    return "PLUS2";
	case TokenKind::PLUS_EQUAL:
	    return "PLUS_EQUAL";
	case TokenKind::MINUS:
	    return "MINUS";
	case TokenKind::MINUS2:
	    return "MINUS2";
	case TokenKind::MINUS_EQUAL:
	    return "MINUS_EQUAL";
	case TokenKind::ARROW:
	    return "ARROW";
	case TokenKind::ASTERISK:
	    return "ASTERISK";
	case TokenKind::ASTERISK_EQUAL:
	    return "ASTERISK_EQUAL";
	case TokenKind::SLASH:
	    return "SLASH";
	case TokenKind::SLASH_EQUAL:
	    return "SLASH_EQUAL";
	case TokenKind::PERCENT:
	    return "PERCENT";
	case TokenKind::PERCENT_EQUAL:
	    return "PERCENT_EQUAL";
	case TokenKind::EQUAL:
	    return "EQUAL";
	case TokenKind::EQUAL2:
	    return "EQUAL2";
	case TokenKind::NOT:
	    return "NOT";
	case TokenKind::NOT_EQUAL:
	    return "NOT_EQUAL";
	case TokenKind::GREATER:
	    return "GREATER";
	case TokenKind::GREATER_EQUAL:
	    return "GREATER_EQUAL";
	case TokenKind::LESS:
	    return "LESS";
	case TokenKind::LESS_EQUAL:
	    return "LESS_EQUAL";
	default:
	    assert(0); // never reached
	    return 0;
    }
}

static Token::Loc::Pos curr = { 1, 0 };
static char ch;

static char
nextCh(void)
{
    ch = getchar();
    ++curr.col;
    if (ch == '\n') {
	++curr.line;
	curr.col = 1;
    }
    return ch;
}

static bool
isWhiteSpace(int ch)
{
    return ch == ' ' || ch == '\t';
}

static bool
isDecDigit(int ch)
{
    return ch >= '0' && ch <= '9';
}

static bool
isOctDigit(int ch)
{
    return ch >= '0' && ch <= '7';
}

static bool
isHexDigit(int ch)
{
    return isDecDigit(ch) || (ch >= 'a' && ch <= 'f') ||
           (ch >= 'A' && ch <= 'F');
}

static bool
isLetter(int ch)
{
    return ((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A' && ch <= 'Z')) ||
           ch == '_';
}

static std::unordered_map<UStr, TokenKind> kw = {
    { "i8", TokenKind::I8 },
    { "i16", TokenKind::I16 },
    { "i32", TokenKind::I32 },
    { "i64", TokenKind::I64 },
    { "u8", TokenKind::U8 },
    { "u16", TokenKind::U16 },
    { "u32", TokenKind::U32 },
    { "u64", TokenKind::U64 },
    { "fn", TokenKind::FN },
    { "return", TokenKind::RETURN },
    { "decl", TokenKind::DECL },
    { "global", TokenKind::GLOBAL },
    { "local", TokenKind::LOCAL },
    { "static", TokenKind::STATIC },
    { "for", TokenKind::FOR },
};

static std::string token_str;

static void
tokenReset(void)
{
    token.loc.from.line = curr.line;
    token.loc.from.col = curr.col;
    token.val = UStr{};
    token_str = "";
}

static void
tokenUpdate(void)
{
    token_str += ch;
    token.loc.to.line = curr.line;
    token.loc.to.col = curr.col;
}

static TokenKind
tokenSet(TokenKind kind)
{
    token.kind = kind;
    token.val = UStr{token_str};
    if (kind == TokenKind::IDENTIFIER && kw.contains(token.val)) {
	token.kind = kw[token.val];
    }
    return token.kind;
}

TokenKind
getToken(void)
{
    // init ch, skip white spaces and newlines
    while (ch == 0 || isWhiteSpace(ch) || ch == '\n') {
        nextCh();
    }

    tokenReset();
    if (ch == EOF) {
	return tokenSet(TokenKind::EOI);
    } else if (isLetter(ch)) {
        while (isLetter(ch) || isDecDigit(ch)) {
	    tokenUpdate();
            nextCh();
        }
        return tokenSet(TokenKind::IDENTIFIER);
    } else if (isDecDigit(ch)) {
        // parse literal
        if (ch == '0') {
	    tokenUpdate();
            nextCh();
            if (ch == 'x') {
		tokenUpdate();
                nextCh();
                if (isHexDigit(ch)) {
                    while (isHexDigit(ch)) {
			tokenUpdate();
                        nextCh();
                    }
                    return tokenSet(TokenKind::HEXADECIMAL_LITERAL);
                }
                return tokenSet(TokenKind::BAD);
            }
            while (isOctDigit(ch)) {
		tokenUpdate();
                nextCh();
            }
            return tokenSet(TokenKind::OCTAL_LITERAL);
        } else if (isDecDigit(ch)) {
            while (isDecDigit(ch)) {
		tokenUpdate();
                nextCh();
            }
            return tokenSet(TokenKind::DECIMAL_LITERAL);
        }
    } else if (ch == '"') {
	do {
	    tokenUpdate();
	    nextCh();
	} while (ch != '"');
	tokenUpdate();
	nextCh();
	return tokenSet(TokenKind::STRING_LITERAL);
    } else if (ch == ';') {
	tokenUpdate();
	nextCh();
	return tokenSet(TokenKind::SEMICOLON);
    } else if (ch == ':') {
	tokenUpdate();
	nextCh();
	return tokenSet(TokenKind::COLON);
    } else if (ch == ',') {
	tokenUpdate();
	nextCh();
	return tokenSet(TokenKind::COMMA);
    } else if (ch == '{') {
	tokenUpdate();
	nextCh();
	return tokenSet(TokenKind::LBRACE);
    } else if (ch == '}') {
	tokenUpdate();
	nextCh();
	return tokenSet(TokenKind::RBRACE);
    } else if (ch == '(') {
	tokenUpdate();
	nextCh();
	return tokenSet(TokenKind::LPAREN);
    } else if (ch == ')') {
	tokenUpdate();
	nextCh();
	return tokenSet(TokenKind::RPAREN);
    } else if (ch == '[') {
	tokenUpdate();
	nextCh();
	return tokenSet(TokenKind::LBRACKET);
    } else if (ch == ']') {
	tokenUpdate();
	nextCh();
	return tokenSet(TokenKind::RBRACKET);
    } else if (ch == '^') {
	tokenUpdate();
	nextCh();
	return tokenSet(TokenKind::CARET);
    } else if (ch == '+') {
	tokenUpdate();
	nextCh();
	if (ch == '+') {
	    tokenUpdate();
	    nextCh();
	    return tokenSet(TokenKind::PLUS2);
	} else if (ch == '=') {
	    tokenUpdate();
	    nextCh();
	    return tokenSet(TokenKind::PLUS_EQUAL);
	}
	return tokenSet(TokenKind::PLUS);
    } else if (ch == '-') {
	tokenUpdate();
	nextCh();
	if (ch == '-') {
	    tokenUpdate();
	    nextCh();
	    return tokenSet(TokenKind::MINUS2);
	} else if (ch == '=') {
	    tokenUpdate();
	    nextCh();
	    return tokenSet(TokenKind::MINUS_EQUAL);
	} else if (ch == '>') {
	    tokenUpdate();
	    nextCh();
	    return tokenSet(TokenKind::ARROW);
	}
	return tokenSet(TokenKind::MINUS);
    } else if (ch == '*') {
	tokenUpdate();
	nextCh();
	if (ch == '=') {
	    tokenUpdate();
	    nextCh();
	    return tokenSet(TokenKind::ASTERISK_EQUAL);
	}
	return tokenSet(TokenKind::ASTERISK);
    } else if (ch == '/') {
	tokenUpdate();
	nextCh();
	if (ch == '=') {
	    tokenUpdate();
	    nextCh();
	    return tokenSet(TokenKind::SLASH_EQUAL);
	}
	return tokenSet(TokenKind::SLASH);
     } else if (ch == '%') {
	tokenUpdate();
	nextCh();
	if (ch == '=') {
	    tokenUpdate();
	    nextCh();
	    return tokenSet(TokenKind::PERCENT_EQUAL);
	}
	return tokenSet(TokenKind::PERCENT);
    } else if (ch == '=') {
	tokenUpdate();
	nextCh();
	if (ch == '=') {
	    tokenUpdate();
	    nextCh();
	    return tokenSet(TokenKind::EQUAL2);
	}
	return tokenSet(TokenKind::EQUAL);
    } else if (ch == '!') {
	tokenUpdate();
	nextCh();
	if (ch == '=') {
	    tokenUpdate();
	    nextCh();
	    return tokenSet(TokenKind::NOT_EQUAL);
	}
	return tokenSet(TokenKind::NOT);
    } else if (ch == '>') {
	tokenUpdate();
	nextCh();
	if (ch == '=') {
	    tokenUpdate();
	    nextCh();
	    return tokenSet(TokenKind::GREATER_EQUAL);
	}
	return tokenSet(TokenKind::GREATER);
    } else if (ch == '<') {
	tokenUpdate();
	nextCh();
	if (ch == '=') {
	    tokenUpdate();
	    nextCh();
	    return tokenSet(TokenKind::LESS_EQUAL);
	}
	return tokenSet(TokenKind::LESS);
    }

    tokenUpdate();
    nextCh();
    return tokenSet(TokenKind::BAD);
}

std::ostream &
operator<<(std::ostream &out, const Token::Loc &loc)
{
    out << loc.from.line << '.' << loc.from.col << ':'
	<< loc.to.line << '.' << loc.to.col;
    return out;
}
