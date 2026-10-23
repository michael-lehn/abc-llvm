// TEST: T0152
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "lexer/lexer.hpp"
#include "lexer/reader.hpp"

#include "check.hpp"

using abc::lexer::Token;
using abc::lexer::TokenKind;

// Write 'src' into a temporary file and return all tokens up to (but not
// including) EOI.
static std::vector<Token>
lex(const std::string &src)
{
    static int count;
    auto path = std::filesystem::temp_directory_path() /
                ("test_lexer_" + std::to_string(count++) + ".abc");
    {
	std::ofstream out{path};
	out << src;
    }
    abc::lexer::init();
    // drop reader of previous input, otherwise it is treated like the
    // including file and the lexer returns to it at the end of input
    abc::lexer::reader.reset();
    if (!abc::lexer::openInputfile(path)) {
	std::cout << "can not open " << path << std::endl;
	std::exit(1);
    }
    std::vector<Token> tokens;
    while (abc::lexer::getToken() != TokenKind::EOI) {
	tokens.push_back(abc::lexer::token);
	if (abc::lexer::token.kind == TokenKind::BAD) {
	    break;
	}
    }
    std::filesystem::remove(path);
    return tokens;
}

static std::vector<TokenKind>
kinds(const std::string &src)
{
    std::vector<TokenKind> result;
    for (const auto &tok : lex(src)) {
	result.push_back(tok.kind);
    }
    return result;
}

namespace abc::lexer {

// found by argument dependent lookup in CHECK_EQ
static std::ostream &
operator<<(std::ostream &out, const std::vector<TokenKind> &kinds)
{
    out << "{";
    for (const auto &kind : kinds) {
	out << " " << kind;
    }
    return out << " }";
}

} // namespace abc::lexer

// kind and processed value of a single token
static bool
isSingle(const std::string &src, TokenKind kind, const std::string &val)
{
    auto tokens = lex(src);
    return tokens.size() == 1 && tokens[0].kind == kind &&
           tokens[0].processedVal.c_str() == val;
}

//------------------------------------------------------------------------------

static void
testPunctuators()
{
    using enum TokenKind;
    CHECK_EQ(kinds(". ... ; : , { } ( ) [ ] ?"),
             (std::vector{DOT, DOT3, SEMICOLON, COLON, COMMA, LBRACE, RBRACE,
                          LPAREN, RPAREN, LBRACKET, RBRACKET, QUERY}));
    CHECK_EQ(kinds("+ ++ += - -- -= -> * *= / /= % %="),
             (std::vector{PLUS, PLUS2, PLUS_EQUAL, MINUS, MINUS2, MINUS_EQUAL,
                          ARROW, ASTERISK, ASTERISK_EQUAL, SLASH, SLASH_EQUAL,
                          PERCENT, PERCENT_EQUAL}));
    CHECK_EQ(kinds("= == ! != > >= < <= & && | ||"),
             (std::vector{EQUAL, EQUAL2, NOT, NOT_EQUAL, GREATER,
                          GREATER_EQUAL, LESS, LESS_EQUAL, AND, AND2, OR,
                          OR2}));
    CHECK_EQ(kinds("^ ^= &= |= << <<= >> >>="),
             (std::vector{CARET, CARET_EQUAL, AND_EQUAL, OR_EQUAL, LESS2,
                          LESS2_EQUAL, GREATER2, GREATER2_EQUAL}));
    // '~' is not a token of ABC ('#' is an open question, #0023)
    CHECK_EQ(kinds("~"), (std::vector{BAD}));
}

static void
testMaximalMunch()
{
    using enum TokenKind;
    // tokens end if the next character can not be added
    CHECK_EQ(kinds("a+++b"), (std::vector{IDENTIFIER, PLUS2, PLUS, IDENTIFIER}));
    CHECK_EQ(kinds("a--->b"),
             (std::vector{IDENTIFIER, MINUS2, ARROW, IDENTIFIER}));
    CHECK_EQ(kinds("x->->"), (std::vector{IDENTIFIER, ARROW, ARROW}));
    CHECK_EQ(kinds("a>>=b"), (std::vector{IDENTIFIER, GREATER2_EQUAL,
                                          IDENTIFIER}));
    CHECK_EQ(kinds("a&&&b"), (std::vector{IDENTIFIER, AND2, AND, IDENTIFIER}));
    // '..' is not a token
    CHECK_EQ(kinds(".."), (std::vector{BAD}));
}

static void
testKeywords()
{
    using enum TokenKind;
    CHECK_EQ(kinds("array break case const continue default do else"),
             (std::vector{ARRAY, BREAK, CASE, CONST, CONTINUE, DEFAULT, DO,
                          ELSE}));
    CHECK_EQ(kinds("enum extern fn for global goto if label local nullptr"),
             (std::vector{ENUM, EXTERN, FN, FOR, GLOBAL, GOTO, IF, LABEL, LOCAL,
                          NULLPTR}));
    CHECK_EQ(kinds("of return sizeof struct switch then type union while"),
             (std::vector{OF, RETURN, SIZEOF, STRUCT, SWITCH, THEN, TYPE, UNION,
                          WHILE}));
    CHECK_EQ(kinds("static assert true false"),
             (std::vector{STATIC, ASSERT, TRUE, FALSE}));
    // 'readonly' is a synonym for 'const'
    CHECK_EQ(kinds("readonly"), (std::vector{CONST}));
    // keywords are case sensitive and only match complete identifiers
    CHECK_EQ(kinds("While fnord iff"),
             (std::vector{IDENTIFIER, IDENTIFIER, IDENTIFIER}));
    // predefined type names are identifiers, not keywords
    CHECK_EQ(kinds("int u8 void"),
             (std::vector{IDENTIFIER, IDENTIFIER, IDENTIFIER}));
    // README reserves alignas and alignof
    CHECK_KNOWN_BUG("T0158", "#0020", kinds("alignas")[0] != IDENTIFIER &&
                                          kinds("alignof")[0] != IDENTIFIER);
}

static void
testIdentifiers()
{
    using enum TokenKind;
    CHECK(isSingle("_", IDENTIFIER, "_"));
    CHECK(isSingle("_a1", IDENTIFIER, "_a1"));
    CHECK(isSingle("Abc_123", IDENTIFIER, "Abc_123"));
    // identifiers can not start with a digit
    CHECK_EQ(kinds("1abc"), (std::vector{DECIMAL_LITERAL, IDENTIFIER}));
}

static void
testIntegerLiterals()
{
    using enum TokenKind;
    CHECK(isSingle("1", DECIMAL_LITERAL, "1"));
    CHECK(isSingle("1234567890123456789012", DECIMAL_LITERAL,
                   "1234567890123456789012"));
    // a leading zero makes the literal octal
    CHECK(isSingle("0", OCTAL_LITERAL, "0"));
    CHECK(isSingle("0755", OCTAL_LITERAL, "0755"));
    CHECK(isSingle("0x1f", HEXADECIMAL_LITERAL, "0x1f"));
    CHECK(isSingle("0XAbC", HEXADECIMAL_LITERAL, "0XAbC"));
    // a type suffix is a separate identifier token
    CHECK_EQ(kinds("256u64"), (std::vector{DECIMAL_LITERAL, IDENTIFIER}));
    CHECK_EQ(kinds("-1"), (std::vector{MINUS, DECIMAL_LITERAL}));
}

static void
testFloatLiterals()
{
    using enum TokenKind;
    CHECK(isSingle("1.5", FLOAT_DECIMAL_LITERAL, "1.5"));
    CHECK(isSingle("3.", FLOAT_DECIMAL_LITERAL, "3."));
    CHECK(isSingle("1e3", FLOAT_DECIMAL_LITERAL, "1e3"));
    CHECK(isSingle("1.5E-3", FLOAT_DECIMAL_LITERAL, "1.5E-3"));
    CHECK(isSingle("0.25", FLOAT_DECIMAL_LITERAL, "0.25"));
    // digits 8 and 9 after a leading 0 are allowed in floating literals
    CHECK(isSingle("09.5", FLOAT_DECIMAL_LITERAL, "09.5"));
    CHECK_EQ(kinds("1.5.x"),
             (std::vector{FLOAT_DECIMAL_LITERAL, DOT, IDENTIFIER}));
}

static void
testCharacterLiterals()
{
    using enum TokenKind;
    CHECK(isSingle("'a'", CHARACTER_LITERAL, "a"));
    CHECK(isSingle("' '", CHARACTER_LITERAL, " "));
    CHECK(isSingle("'\\n'", CHARACTER_LITERAL, "\n"));
    CHECK(isSingle("'\\t'", CHARACTER_LITERAL, "\t"));
    CHECK(isSingle("'\\''", CHARACTER_LITERAL, "'"));
    CHECK(isSingle("'\\\\'", CHARACTER_LITERAL, "\\"));
    CHECK(isSingle("'\\x41'", CHARACTER_LITERAL, "A"));
    CHECK(isSingle("'\\101'", CHARACTER_LITERAL, "A"));
    CHECK(isSingle("'\"'", CHARACTER_LITERAL, "\""));
}

static void
testStringLiterals()
{
    using enum TokenKind;
    CHECK(isSingle("\"\"", STRING_LITERAL, ""));
    CHECK(isSingle("\"hello, world\"", STRING_LITERAL, "hello, world"));
    CHECK(isSingle("\"a\\nb\\tc\"", STRING_LITERAL, "a\nb\tc"));
    CHECK(isSingle("\"\\\"q\\\"\"", STRING_LITERAL, "\"q\""));
    CHECK(isSingle("\"\\\\\"", STRING_LITERAL, "\\"));
    CHECK(isSingle("\"\\x41\\x42\"", STRING_LITERAL, "AB"));
    CHECK(isSingle("\"// no comment\"", STRING_LITERAL, "// no comment"));
    // the raw value keeps the quotes and escape sequences
    auto tokens = lex("\"a\\n\"");
    CHECK(tokens.size() == 1 &&
          std::string{tokens[0].val.c_str()} == "\"a\\n\"");
    // adjacent string literals are separate tokens (joined by the parser)
    CHECK_EQ(kinds("\"a\" \"b\""), (std::vector{STRING_LITERAL,
                                                STRING_LITERAL}));
    CHECK_KNOWN_BUG("T0159", "#0012",
                    isSingle("\"\\101B\"", STRING_LITERAL, "AB"));
}

static void
testComments()
{
    using enum TokenKind;
    CHECK_EQ(kinds("a /* comment */ b"), (std::vector{IDENTIFIER, IDENTIFIER}));
    CHECK_EQ(kinds("a // comment\nb"), (std::vector{IDENTIFIER, IDENTIFIER}));
    CHECK_EQ(kinds("a/**/b"), (std::vector{IDENTIFIER, IDENTIFIER}));
    CHECK_EQ(kinds("/* multi\nline\n*/ a"), (std::vector{IDENTIFIER}));
    CHECK_EQ(kinds("a /* // */ b"), (std::vector{IDENTIFIER, IDENTIFIER}));
    // nested comments are not supported: comment ends at first '*/'
    CHECK_EQ(kinds("/* a /* b */ c */"),
             (std::vector{IDENTIFIER, ASTERISK, SLASH}));
    // a comment separates tokens
    CHECK_EQ(kinds("+/**/+"), (std::vector{PLUS, PLUS}));
}

static void
testLocations()
{
    auto tokens = lex("ab\n  cd\n\tx");
    CHECK_EQ(tokens.size(), 3u);
    if (tokens.size() == 3) {
	CHECK_EQ(tokens[0].loc.from.line, 1u);
	CHECK_EQ(tokens[0].loc.from.col, 1u);
	CHECK_EQ(tokens[0].loc.to.col, 3u);
	CHECK_EQ(tokens[1].loc.from.line, 2u);
	CHECK_EQ(tokens[1].loc.from.col, 3u);
	CHECK_EQ(tokens[2].loc.from.line, 3u);
	// tab stops every 8 columns
	CHECK_KNOWN_BUG("T0160", "#0015", tokens[2].loc.from.col == 9);
    }
}

static void
testMacros()
{
    using enum TokenKind;
    CHECK(isSingle("@define N 42\nN", DECIMAL_LITERAL, "42"));
    CHECK_EQ(kinds("@define TWO 1 + 1\nTWO"),
             (std::vector{DECIMAL_LITERAL, PLUS, DECIMAL_LITERAL}));
    CHECK_EQ(kinds("@ifdef UNDEFINED\na\n@endif\nb"), (std::vector{IDENTIFIER}));
    CHECK_EQ(kinds("@define D\n@ifdef D\na\n@endif\nb"),
             (std::vector{IDENTIFIER, IDENTIFIER}));
}

static void
testEndOfInput()
{
    CHECK(lex("").empty());
    CHECK(lex(" \t\n\n ").empty());
    CHECK(lex("/* only a comment */\n").empty());
    // EOI is sticky
    lex("");
    CHECK_EQ(abc::lexer::getToken(), TokenKind::EOI);
}

int
main(int argc, char *argv[])
{
    testPunctuators();
    testMaximalMunch();
    testKeywords();
    testIdentifiers();
    testIntegerLiterals();
    testFloatLiterals();
    testCharacterLiterals();
    testStringLiterals();
    testComments();
    testLocations();
    testMacros();
    testEndOfInput();
    return check::summary(argv[0]);
}
