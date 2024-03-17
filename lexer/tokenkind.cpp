#include <cassert>
#include <iostream>

#include "tokenkind.hpp"

namespace abc { namespace lexer {

const char *
TokenKindCStr(TokenKind kind)
{
    switch (kind) {
	case TokenKind::BAD: return "BAD";
	case TokenKind::EOI: return "EOI";
	case TokenKind::IDENTIFIER: return "IDENTIFIER";
	case TokenKind::DECIMAL_LITERAL: return "DECIMAL_LITERAL";
	case TokenKind::HEXADECIMAL_LITERAL: return "HEXADECIMAL_LITERAL";
	case TokenKind::OCTAL_LITERAL: return "OCTAL_LITERAL";
	case TokenKind::STRING_LITERAL: return "STRING_LITERAL";
	case TokenKind::CHARACTER_LITERAL: return "CHARACTER_LITERAL";

	case TokenKind::ARRAY: return "ARRAY";
	case TokenKind::ASSERT: return "ASSERT";
	case TokenKind::BREAK: return "BREAK";
	case TokenKind::CASE: return "CASE";
	case TokenKind::CONST: return "CONST";
	case TokenKind::CONTINUE: return "CONTINUE";
	case TokenKind::DEFAULT: return "DEFAULT";
	case TokenKind::DO: return "DO";
	case TokenKind::ELSE: return "ELSE";
	case TokenKind::ENUM: return "ENUM";
	case TokenKind::EXTERN: return "EXTERN";
	case TokenKind::FN: return "FN";
	case TokenKind::FOR: return "FOR";
	case TokenKind::GLOBAL: return "GLOBAL";
	case TokenKind::GOTO: return "GOTO";
	case TokenKind::IF: return "IF";
	case TokenKind::LABEL: return "LABEL";
	case TokenKind::LOCAL: return "LOCAL";
	case TokenKind::NULLPTR: return "NULLPTR";
	case TokenKind::OF: return "OF";
	case TokenKind::RETURN: return "RETURN";
	case TokenKind::SIZEOF: return "SIZEOF";
	case TokenKind::STRUCT: return "STRUCT";
	case TokenKind::SWITCH: return "SWITCH";
	case TokenKind::THEN: return "THEN";
	case TokenKind::TYPE: return "TYPE";
	case TokenKind::UNION: return "UNION";
	case TokenKind::WHILE: return "WHILE";

	case TokenKind::DOT: return "DOT";
	case TokenKind::DOT3: return "DOT3";
	case TokenKind::SEMICOLON: return "SEMICOLON";
	case TokenKind::COLON: return "COLON";
	case TokenKind::COMMA: return "COMMA";
	case TokenKind::LBRACE: return "LBRACE";
	case TokenKind::RBRACE: return "RBRACE";
	case TokenKind::LPAREN: return "LPAREN";
	case TokenKind::RPAREN: return "RPAREN";
	case TokenKind::LBRACKET: return "LBRACKET";
	case TokenKind::RBRACKET: return "RBRACKET";
	case TokenKind::CARET: return "CARET";
	case TokenKind::PLUS: return "PLUS";
	case TokenKind::PLUS2: return "PLUS2";
	case TokenKind::PLUS_EQUAL: return "PLUS_EQUAL";
	case TokenKind::MINUS: return "MINUS";
	case TokenKind::MINUS2: return "MINUS2";
	case TokenKind::MINUS_EQUAL: return "MINUS_EQUAL";
	case TokenKind::ARROW: return "ARROW";
	case TokenKind::ASTERISK: return "ASTERISK";
	case TokenKind::ASTERISK_EQUAL: return "ASTERISK_EQUAL";
	case TokenKind::SLASH: return "SLASH";
	case TokenKind::SLASH_EQUAL: return "SLASH_EQUAL";
	case TokenKind::PERCENT: return "PERCENT";
	case TokenKind::PERCENT_EQUAL: return "PERCENT_EQUAL";
	case TokenKind::EQUAL: return "EQUAL";
	case TokenKind::EQUAL2: return "EQUAL2";
	case TokenKind::NOT: return "NOT";
	case TokenKind::NOT_EQUAL: return "NOT_EQUAL";
	case TokenKind::GREATER: return "GREATER";
	case TokenKind::GREATER_EQUAL: return "GREATER_EQUAL";
	case TokenKind::LESS: return "LESS";
	case TokenKind::LESS_EQUAL: return "LESS_EQUAL";
	case TokenKind::AND: return "AND";
	case TokenKind::AND2: return "AND2";
	case TokenKind::OR: return "OR";
	case TokenKind::OR2: return "OR2";
	case TokenKind::QUERY: return "QUERY";
	case TokenKind::HASH: return "HASH";
	default:
	    std::cerr << "kind = " << int(kind) << std::endl;
	    assert(0); // never reached
	    return 0;
    }
}

static const char *
getCStr(TokenKind kind)
{
    switch (kind) {
	case TokenKind::ARRAY: return "array";
	case TokenKind::ASSERT: return "assert";
	case TokenKind::BREAK: return "break";
	case TokenKind::CASE: return "case";
	case TokenKind::CONST: return "const";
	case TokenKind::CONTINUE: return "continue";
	case TokenKind::DEFAULT: return "default";
	case TokenKind::DO: return "do";
	case TokenKind::ELSE: return "else";
	case TokenKind::ENUM: return "enum";
	case TokenKind::EXTERN: return "extern";
	case TokenKind::FN: return "fn";
	case TokenKind::FOR: return "for";
	case TokenKind::GLOBAL: return "global";
	case TokenKind::GOTO: return "goto";
	case TokenKind::IF: return "if";
	case TokenKind::LABEL: return "label";
	case TokenKind::LOCAL: return "local";
	case TokenKind::NULLPTR: return "nullptr";
	case TokenKind::OF: return "of";
	case TokenKind::RETURN: return "return";
	case TokenKind::SIZEOF: return "sizeof";
	case TokenKind::STRUCT: return "struct";
	case TokenKind::SWITCH: return "switch";
	case TokenKind::THEN: return "then";
	case TokenKind::TYPE: return "type";
	case TokenKind::UNION: return "union";
	case TokenKind::WHILE: return "while";

	case TokenKind::DOT: return ".";
	case TokenKind::DOT3: return "...";
	case TokenKind::SEMICOLON: return ";";
	case TokenKind::COLON: return ":";
	case TokenKind::COMMA: return ",";
	case TokenKind::LBRACE: return "{";
	case TokenKind::RBRACE: return "}";
	case TokenKind::LPAREN: return "(";
	case TokenKind::RPAREN: return ")";
	case TokenKind::LBRACKET: return "[";
	case TokenKind::RBRACKET: return "]";
	case TokenKind::CARET: return "^";
	case TokenKind::PLUS: return "+";
	case TokenKind::PLUS2: return "++";
	case TokenKind::PLUS_EQUAL: return "+=";
	case TokenKind::MINUS: return "-";
	case TokenKind::MINUS2: return "--";
	case TokenKind::MINUS_EQUAL: return "-=";
	case TokenKind::ARROW: return "->";
	case TokenKind::ASTERISK: return "*";
	case TokenKind::ASTERISK_EQUAL: return "*=";
	case TokenKind::SLASH: return "/";
	case TokenKind::SLASH_EQUAL: return "/=";
	case TokenKind::PERCENT: return "%";
	case TokenKind::PERCENT_EQUAL: return "%=";
	case TokenKind::EQUAL: return "=";
	case TokenKind::EQUAL2: return "==";
	case TokenKind::NOT: return "!";
	case TokenKind::NOT_EQUAL: return "!=";
	case TokenKind::GREATER: return ">";
	case TokenKind::GREATER_EQUAL: return ">=";
	case TokenKind::LESS: return "<";
	case TokenKind::LESS_EQUAL: return "<=";
	case TokenKind::AND: return "&";
	case TokenKind::AND2: return "&&";
	case TokenKind::OR: return "|";
	case TokenKind::OR2: return "||";
	case TokenKind::QUERY: return "?";
	case TokenKind::HASH: return "#";
	default:
	    return TokenKindCStr(kind);
    }
}

std::ostream &
operator<<(std::ostream &out, TokenKind kind)
{
    out << getCStr(kind);
    return out;
}

} } // namespace lexer, abc
