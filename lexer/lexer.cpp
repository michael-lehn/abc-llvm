#include <iostream>
#include <string>

#include "util/ustr.hpp"

#include "error.hpp"
#include "lexer.hpp"
#include "macro.hpp"
#include "reader.hpp"

namespace abc { namespace lexer {

Token token;

static std::unordered_map<UStr, TokenKind> keyword;
static std::set<std::filesystem::path> includedFiles_;

static bool isWhiteSpace(int ch);
static bool isDecDigit(int ch);
static bool isOctDigit(int ch);
static bool isLetter(int ch);
static TokenKind getToken_(bool skipNewline = true);
static TokenKind setToken(TokenKind kind, std::string processed = "");

static unsigned hexToVal(char ch);
static std::string parseStringLiteral();
static unsigned parseCharacterLiteral();
static void parseAddDirective();

//------------------------------------------------------------------------------

static bool
isWhiteSpace(int ch)
{
    return ch == ' ' || ch == '\t' || ch == '\r';
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

void
init()
{
    macro::init();
    includedFiles_.clear();
    keyword[UStr::create("array")] = TokenKind::ARRAY;
    keyword[UStr::create("assert")] = TokenKind::ASSERT;
    keyword[UStr::create("break")] = TokenKind::BREAK;
    keyword[UStr::create("case")] = TokenKind::CASE;
    keyword[UStr::create("const")] = TokenKind::CONST;
    keyword[UStr::create("continue")] = TokenKind::CONTINUE;
    keyword[UStr::create("default")] = TokenKind::DEFAULT;
    keyword[UStr::create("do")] = TokenKind::DO;
    keyword[UStr::create("else")] = TokenKind::ELSE;
    keyword[UStr::create("enum")] = TokenKind::ENUM;
    keyword[UStr::create("extern")] = TokenKind::EXTERN;
    keyword[UStr::create("fn")] = TokenKind::FN;
    keyword[UStr::create("for")] = TokenKind::FOR;
    keyword[UStr::create("global")] = TokenKind::GLOBAL;
    keyword[UStr::create("static")] = TokenKind::STATIC;
    keyword[UStr::create("goto")] = TokenKind::GOTO;
    keyword[UStr::create("if")] = TokenKind::IF;
    keyword[UStr::create("label")] = TokenKind::LABEL;
    keyword[UStr::create("local")] = TokenKind::LOCAL;
    keyword[UStr::create("nullptr")] = TokenKind::NULLPTR;
    keyword[UStr::create("of")] = TokenKind::OF;
    keyword[UStr::create("return")] = TokenKind::RETURN;
    keyword[UStr::create("sizeof")] = TokenKind::SIZEOF;
    keyword[UStr::create("struct")] = TokenKind::STRUCT;
    keyword[UStr::create("switch")] = TokenKind::SWITCH;
    keyword[UStr::create("then")] = TokenKind::THEN;
    keyword[UStr::create("type")] = TokenKind::TYPE;
    keyword[UStr::create("union")] = TokenKind::UNION;
    keyword[UStr::create("while")] = TokenKind::WHILE;
}

const std::set<std::filesystem::path> &
includedFiles()
{
    return includedFiles_;
}

static TokenKind
setToken(TokenKind kind, std::string processed)
{
    auto loc = Loc{reader->path, reader->start, reader->pos};
    auto val = UStr::create(reader->val);

    token = processed.empty() && kind != TokenKind::STRING_LITERAL
	? Token(loc, kind, val)
	: Token(loc, kind, val, UStr::create(processed));
    return kind;
}

TokenKind
getToken()
{
    do {
	UStr newVal;
	do {
	    getToken_();
	    // if no replacement can be applied we found a new token
	    if (!macro::expandMacro(token.val, newVal)) {
		break;
	    }
	    // if it was replaced with an empty string get next token
	} while (newVal.empty());
	if (token.kind == TokenKind::IDENTIFIER && keyword.contains(newVal)) {
	    token = Token(token.loc, keyword.at(newVal), newVal);
	} else if (newVal != token.val) {
	    token = Token(token.loc, token.kind, newVal);
	}
    } while (macro::ignoreToken());
    return token.kind;
}

TokenKind
getToken_(bool skipNewline)
{
    // skip white spaces and newlines
    while (isWhiteSpace(reader->ch) || (skipNewline && reader->ch == '\n')) {
        nextCh();
    }

    reader->resetStart();

    if (reader->eof()) {
	return setToken(TokenKind::EOI);
    } else if (reader->ch == '"') {
	auto str = parseStringLiteral();
	return setToken(TokenKind::STRING_LITERAL, str);
    } else if (reader->ch == '\'') {
	std::string str{char(parseCharacterLiteral())};
	return setToken(TokenKind::CHARACTER_LITERAL, str);
    } else if (reader->ch == '@') {
	parseAddDirective();
	return getToken();
    } else if (isLetter(reader->ch)) {
        while (isLetter(reader->ch) || isDecDigit(reader->ch)) {
            nextCh();
        }
        return setToken(TokenKind::IDENTIFIER);
    } else if (isDecDigit(reader->ch)) {
        // parse literal
        if (reader->ch == '0') {
            nextCh();
            if (reader->ch == 'x') {
                nextCh();
		std::string processed = "";
                if (isHexDigit(reader->ch)) {
                    while (isHexDigit(reader->ch)) {
			processed += reader->ch;
                        nextCh();
                    }
                    return setToken(TokenKind::HEXADECIMAL_LITERAL, processed);
                }
                return setToken(TokenKind::BAD);
            }
	    std::string processed = "";
            while (isOctDigit(reader->ch)) {
                nextCh();
            }
            return setToken(TokenKind::OCTAL_LITERAL, processed);
        } else {
            while (isDecDigit(reader->ch)) {
                nextCh();
            }
            return setToken(TokenKind::DECIMAL_LITERAL);
        }
    } else if (reader->ch == '\n') {
	nextCh();
	return setToken(TokenKind::NEWLINE);
    } else if (reader->ch == '.') {
	nextCh();
	if (reader->ch == '.') {
	    nextCh();
	    if (reader->ch == '.') {
		nextCh();
		return setToken(TokenKind::DOT3);
	    }
	    return setToken(TokenKind::BAD);
	}
	return setToken(TokenKind::DOT);
    } else if (reader->ch == ';') {
	nextCh();
	return setToken(TokenKind::SEMICOLON);
    } else if (reader->ch == ':') {
	nextCh();
	return setToken(TokenKind::COLON);
    } else if (reader->ch == ',') {
	nextCh();
	return setToken(TokenKind::COMMA);
    } else if (reader->ch == '{') {
	nextCh();
	return setToken(TokenKind::LBRACE);
    } else if (reader->ch == '}') {
	nextCh();
	return setToken(TokenKind::RBRACE);
    } else if (reader->ch == '(') {
	nextCh();
	return setToken(TokenKind::LPAREN);
    } else if (reader->ch == ')') {
	nextCh();
	return setToken(TokenKind::RPAREN);
    } else if (reader->ch == '[') {
	nextCh();
	return setToken(TokenKind::LBRACKET);
    } else if (reader->ch == ']') {
	nextCh();
	return setToken(TokenKind::RBRACKET);
    } else if (reader->ch == '^') {
	nextCh();
	return setToken(TokenKind::CARET);
    } else if (reader->ch == '+') {
	nextCh();
	if (reader->ch == '+') {
	    nextCh();
	    return setToken(TokenKind::PLUS2);
	} else if (reader->ch == '=') {
	    nextCh();
	    return setToken(TokenKind::PLUS_EQUAL);
	}
	return setToken(TokenKind::PLUS);
    } else if (reader->ch == '-') {
	nextCh();
	if (reader->ch == '-') {
	    nextCh();
	    return setToken(TokenKind::MINUS2);
	} else if (reader->ch == '=') {
	    nextCh();
	    return setToken(TokenKind::MINUS_EQUAL);
	} else if (reader->ch == '>') {
	    nextCh();
	    return setToken(TokenKind::ARROW);
	}
	return setToken(TokenKind::MINUS);
    } else if (reader->ch == '*') {
	nextCh();
	if (reader->ch == '=') {
	    nextCh();
	    return setToken(TokenKind::ASTERISK_EQUAL);
	}
	return setToken(TokenKind::ASTERISK);
    } else if (reader->ch == '/') {
	nextCh();
	if (reader->ch == '=') {
	    nextCh();
	    return setToken(TokenKind::SLASH_EQUAL);
	} else if (reader->ch == '/') {
	    nextCh();
	    // ignore rest of line and return next token
	    while (reader->ch != '\n') {
		nextCh();
	    }
	    return getToken();
	} else if (reader->ch == '*') {
	    nextCh();
	    // skip to next '*', '/'
	    while (reader->ch != EOF) {
		char last = reader->ch;
		nextCh();
		if (last == '*' && reader->ch == '/') {
		    nextCh();
		    break;
		}
	    }
	    if (reader->ch == EOF) {
		error::out() << token.loc
		    <<  ": multi line comment not terminated" << std::endl;
		error::fatal();
		return setToken(TokenKind::BAD);
	    }
	    return getToken();
	}
	return setToken(TokenKind::SLASH);
     } else if (reader->ch == '%') {
	nextCh();
	if (reader->ch == '=') {
	    nextCh();
	    return setToken(TokenKind::PERCENT_EQUAL);
	}
	return setToken(TokenKind::PERCENT);
    } else if (reader->ch == '=') {
	nextCh();
	if (reader->ch == '=') {
	    nextCh();
	    return setToken(TokenKind::EQUAL2);
	}
	return setToken(TokenKind::EQUAL);
    } else if (reader->ch == '!') {
	nextCh();
	if (reader->ch == '=') {
	    nextCh();
	    return setToken(TokenKind::NOT_EQUAL);
	}
	return setToken(TokenKind::NOT);
    } else if (reader->ch == '>') {
	nextCh();
	if (reader->ch == '=') {
	    nextCh();
	    return setToken(TokenKind::GREATER_EQUAL);
	}
	return setToken(TokenKind::GREATER);
    } else if (reader->ch == '<') {
	nextCh();
	if (reader->ch == '=') {
	    nextCh();
	    return setToken(TokenKind::LESS_EQUAL);
	}
	return setToken(TokenKind::LESS);
    } else if (reader->ch == '&') {
	nextCh();
	if (reader->ch == '&') {
	    nextCh();
	    return setToken(TokenKind::AND2);
	}
	return setToken(TokenKind::AND);
    } else if (reader->ch == '|') {
	nextCh();
	if (reader->ch == '|') {
	    nextCh();
	    return setToken(TokenKind::OR2);
	}
	return setToken(TokenKind::OR);
    } else if (reader->ch == '?') {
	nextCh();
	return setToken(TokenKind::QUERY);
    } else {
	nextCh();
	return setToken(TokenKind::BAD);
    }
}

// 
// from: https://github.com/afborchert/astl-c/blob/master/astl-c/scanner.cpp
// 

static unsigned
hexToVal(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    } else if (ch >= 'a' && ch <= 'f') {
        return 10 + ch - 'a';
    } else {
        return 10 + ch - 'A';
    }
}

static std::string
parseStringLiteral()
{
    std::string processed;

    // ch == '"'
    nextCh();
    while (reader->ch != '"') {
        if (reader->ch == '\n') {
	    error::out() << token.loc << ": newline in string literal"
		<< std::endl;
	    error::fatal();
        } else if (reader->ch == '\\') {
            nextCh();
            if (isOctDigit(reader->ch)) {
                unsigned octalval = reader->ch - '0';
                nextCh();
                if (isOctDigit(reader->ch)) {
                    octalval = octalval * 8 + reader->ch - '0';
                    nextCh();
                }
                if (isOctDigit(reader->ch)) {
                    octalval = octalval * 8 + reader->ch - '0';
                    nextCh();
                }
                reader->ch = octalval;
		processed += reader->ch;
            } else {
                switch (reader->ch) {
                    // simple-escape-sequence
                    case '\'':
                        processed += '\'';
                        nextCh();
                        break;
                    case '"':
                        processed += '\"';
                        nextCh();
                        break;
                    case '?':
                        processed += '\?';
                        nextCh();
                        break;
                    case '\\':
                        processed += '\\';
                        nextCh();
                        break;
                    case 'a':
                        processed += '\a';
                        nextCh();
                        break;
                    case 'b':
                        processed += '\b';
                        nextCh();
                        break;
                    case 'f':
                        processed += '\f';
                        nextCh();
                        break;
                    case 'n':
                        processed += '\n';
                        nextCh();
                        break;
                    case 'r':
                        processed += '\r';
                        nextCh();
                        break;
                    case 't':
                        processed += '\t';
                        nextCh();
                        break;
                    case 'v':
                        processed += '\v';
                        nextCh();
                        break;
                    case 'x': {
                        nextCh();
                        if (!isHexDigit(reader->ch)) {
			    error::out() << token.loc
				<< ": expected hex digit" << std::endl;
			    error::fatal();
                        }
                        unsigned hexval = hexToVal(reader->ch);
                        nextCh();
                        while (isHexDigit(reader->ch)) {
                            hexval = hexval * 16 + hexToVal(reader->ch);
                            nextCh();
                        }
			processed += hexval;
                        break;
                    }
                    default:
			error::out() << token.loc
			    << ": invalid character '" << reader->ch
			    << "'" << std::endl;
			error::fatal();
                }
            }
        } else if (reader->ch == EOF) {
	    error::out() << token.loc << ": end of file in string literal"
		<< std::endl;
	    error::fatal();
        } else {
	    processed += reader->ch;
            nextCh();
        }
    }
    nextCh();
    return processed;
}

// 
// from: https://github.com/afborchert/astl-c/blob/master/astl-c/scanner.cpp
// 
static unsigned
parseCharacterLiteral()
{
    unsigned val = 0;
    // ch == '\''
    nextCh();
    if (reader->ch == '\'') {
	error::out() << token.loc << ": single quote as character literal"
	    << std::endl;
	error::fatal();
    }
    do {
        if (reader->ch == '\n') {
	    error::out() << token.loc << ": newline as character literal"
		<< std::endl;
	    error::fatal();
	    break;
        } else if (reader->ch == '\\') {
            nextCh();
            if (isOctDigit(reader->ch)) {
                val = reader->ch - '0';
                nextCh();
                if (isOctDigit(reader->ch)) {
                    val = val * 8 + reader->ch - '0';
                    nextCh();
                }
                if (isOctDigit(reader->ch)) {
                    val = val * 8 + reader->ch - '0';
                    nextCh();
                }
                break;
            } else {
                switch (reader->ch) {
                    // simple-escape-sequence
                    case '\'':
                        val = '\'';
                        nextCh();
                        break;
                    case '"':
                        val = '\"';
                        nextCh();
                        break;
                    case '?':
                        val = '\?';
                        nextCh();
                        break;
                    case '\\':
                        val = '\\';
                        nextCh();
                        break;
                    case 'a':
                        val = '\a';
                        nextCh();
                        break;
                    case 'b':
                        val = '\b';
                        nextCh();
                        break;
                    case 'f':
                        val = '\f';
                        nextCh();
                        break;
                    case 'n':
                        val = '\n';
                        nextCh();
                        break;
                    case 'r':
                        val = '\r';
                        nextCh();
                        break;
                    case 't':
                        val = '\t';
                        nextCh();
                        break;
                    case 'v':
                        val = '\v';
                        nextCh();
                        break;
                    case 'x': {
                        nextCh();
                        if (!isHexDigit(reader->ch)) {
			    error::out() << token.loc
				<< ": expected hex digit" << std::endl;
			    error::fatal();
                        }
                        val = hexToVal(reader->ch);
                        nextCh();
                        while (isHexDigit(reader->ch)) {
                            val = val * 16 + hexToVal(reader->ch);
                            nextCh();
                        }
			break;
                    }
                    default:
			error::out() << token.loc
			    << ": invalid character literal" << std::endl;
			error::fatal();
                        break;
                }
            }
        } else if (reader->ch == EOF) {
	    error::out() << token.loc
		<< ": end of file in character literal" << std::endl;
	    error::fatal();
	    break;
        } else {
	    val = reader->ch; 
            nextCh();
	    break;
        }
    } while (reader->ch != '\'');
    nextCh();
    return val;
}

static void
parseAddDirective()
{
    auto ifdefKw = UStr::create("ifdef");
    auto endifKw = UStr::create("endif");
    auto defineKw = UStr::create("define");

    // ch == '@'
    nextCh();
    getToken_();
    if (token.kind == TokenKind::IDENTIFIER && token.val == ifdefKw) {
	getToken_(false);
	if (token.kind != TokenKind::IDENTIFIER) {
	    error::out() << token.loc
		<< ": expected identifier" << std::endl;
	    error::fatal();
	}
	if (!macro::ifndefDirective(token.val)) {
	    error::out() << token.loc
		<< ": sorry, nested @ifdef are not supported" << std::endl;
	    error::fatal();
	}
    } else if (token.kind == TokenKind::IDENTIFIER && token.val == endifKw) {
	macro::endifDirective();
    } else if (token.kind == TokenKind::IDENTIFIER && token.val == defineKw) {
	getToken_(false);
	if (token.kind != TokenKind::IDENTIFIER) {
	    error::out() << token.loc
		<< ": expected identifier" << std::endl;
	    error::fatal();
	}
	auto from = token.val;
	getToken_(false);
	UStr to;
	if (token.kind == TokenKind::IDENTIFIER) {
	    to = token.val;
	} else if (token.kind != TokenKind::NEWLINE) {
	    error::out() << token.loc
		<< ": expected identifier or newline" << std::endl;
	    error::fatal();
	}
	if (!macro::defineDirective(from, to)) {
	    error::out() << token.loc
		<< ": macro '" << from << "' already defined" << std::endl;
	    error::fatal();
	}
    } else if (token.kind == TokenKind::STRING_LITERAL) {
	if (macro::ignoreToken()) {
	    return;
	}
	if (includedFiles_.contains(token.processedVal.c_str())) {
	    return;
	}
	if (!openInputfile(token.processedVal.c_str())) {
	    error::out() << token.loc
		<< ": can not open file " << token.val << std::endl;
	    error::fatal();
	}
	includedFiles_.insert(token.processedVal.c_str());
    } else if (token.kind == TokenKind::LESS) {
	std::string str;
	while (reader->ch != '>') {
	    str += reader->ch;
	    nextCh();
	}
	nextCh();
	auto path = searchFile(str);
	if (path.empty()) {
	    error::out() << token.loc
		<< ": can not find file " << str << std::endl;
	    error::fatal();
	}
	if (macro::ignoreToken() || includedFiles_.contains(path)) {
	    return;
	}
	includedFiles_.insert(path);
	if (!openInputfile(path)) {
	    error::out() << token.loc
		<< ": can not open file " << path << std::endl;
	    error::fatal();
	}
    } else {
	error::out() << token.loc
	    << ": expected directive or filename" << std::endl;
	error::fatal();
    }
}

} } // namespace lexer, abc
