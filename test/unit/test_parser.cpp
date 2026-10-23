// TEST: T0153
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

#include "gen/gen.hpp"
#include "lexer/lexer.hpp"
#include "lexer/reader.hpp"
#include "parser/defaulttype.hpp"
#include "parser/parseexpr.hpp"
#include "parser/parser.hpp"
#include "symtab/symtab.hpp"
#include "type/inittypesystem.hpp"
#include "type/integertype.hpp"

#include "check.hpp"

using namespace abc;

// Make 'src' the input of the lexer and read the first token.
static void
setInput(const std::string &src)
{
    static int count;
    auto path = std::filesystem::temp_directory_path() /
                ("test_parser_" + std::to_string(count++) + ".abc");
    {
	std::ofstream out{path};
	out << src << "\n";
    }
    lexer::init();
    lexer::reader.reset();
    if (!lexer::openInputfile(path)) {
	std::cout << "can not open " << path << std::endl;
	std::exit(1);
    }
    std::filesystem::remove(path);
    lexer::getToken();
}

static bool
atEnd()
{
    return lexer::token.kind == lexer::TokenKind::EOI;
}

static ExprPtr
parse(const std::string &src)
{
    setInput(src);
    auto expr = parseAssignmentExpression();
    return atEnd() ? std::move(expr) : nullptr;
}

// value of a constant integer expression (empty if not constant)
static std::optional<std::int64_t>
eval(const std::string &src)
{
    auto expr = parse(src);
    if (!expr || !expr->isConst() || !expr->type->isInteger()) {
	return std::nullopt;
    }
    return expr->type->isSignedInteger()
               ? expr->getSignedIntValue()
               : std::int64_t(expr->getUnsignedIntValue());
}

static const Type *
typeOf(const std::string &src)
{
    auto expr = parse(src);
    return expr ? expr->type : nullptr;
}

static const Type *
type(const std::string &src)
{
    setInput(src);
    auto ty = parseType();
    return atEnd() ? ty : nullptr;
}

//------------------------------------------------------------------------------

static void
testPrecedence()
{
    CHECK(eval("2 + 3 * 4") == 14);
    CHECK(eval("(2 + 3) * 4") == 20);
    CHECK(eval("7 - 2 * 3") == 1);
    CHECK(eval("-2 * 3") == -6);
    CHECK(eval("-(2 - 5)") == 3);
    CHECK(eval("- -3") == 3);
    // shift binds weaker than additive operators
    CHECK(eval("1 << 2 + 1") == 8);
    CHECK(eval("1 + 2 << 1") == 6);
    // relational binds stronger than equality
    CHECK(eval("5 > 3 == 1") == 1);
    // equality binds stronger than bitwise and (like in C)
    CHECK(eval("6 & 3 == 3") == 0);
    // & before ^ before |
    CHECK(eval("1 | 2 ^ 3 & 1") == 3);
    CHECK(eval("4 | 1 & 2") == 4);
    // && binds stronger than ||
    CHECK(eval("1 || 0 && 0") == 1);
    CHECK(eval("0 && 1 || 1") == 1);
    // logical not applies to the operand only
    CHECK(eval("!0 + 1") == 2);
    CHECK(eval("!(0 + 1)") == 0);
    // conditional binds weaker than ||
    CHECK(eval("0 || 0 ? 1 : 2") == 2);
}

static void
testAssociativity()
{
    // binary operators are left associative
    CHECK(eval("10 - 4 - 3") == 3);
    CHECK(eval("100 / 10 / 5") == 2);
    CHECK(eval("2 * 3 % 4") == 2);
    CHECK(eval("10 % 4 * 3") == 6);
    CHECK(eval("64 >> 2 >> 1") == 8);
    CHECK(eval("3 > 2 > 1") == 0);
    // the conditional operator is right associative
    CHECK(eval("0 ? 2 : 0 ? 3 : 4") == 4);
    CHECK(eval("1 ? 0 ? 5 : 6 : 7") == 6);
}

static void
testConditionalSyntax()
{
    CHECK(eval("1 ? 2 : 3") == 2);
    CHECK(eval("0 then 2 else 3") == 3);
    CHECK(eval("1 then 2 else 3") == 2);
    // mixed forms ('?' with 'else') are tested in test/run
}

static void
testLiteralTypes()
{
    auto i64 = IntegerType::createSigned(64);
    auto u8 = IntegerType::createUnsigned(8);

    // the type of unsuffixed literals is an open question (#0027)
    CHECK(eval("2147483648") == 2147483648);
    CHECK(eval("017") == 15);
    CHECK(eval("0x1F") == 31);
    CHECK(eval("'A'") == 65);
    CHECK(eval("true") == 1 && eval("false") == 0);
    CHECK(typeOf("true")->isBool());
    // literals with type suffix
    CHECK(Type::equals(typeOf("200u8"), u8));
    CHECK(Type::equals(typeOf("1i64"), i64));
    // comparisons have type bool
    CHECK(typeOf("1 < 2")->isBool());
    CHECK(typeOf("1.5 < 2")->isBool());
    CHECK(typeOf("1 + 1.5")->isDouble());

    CHECK_KNOWN_BUG("T0156", "#0005", eval("0x100000000") == 4294967296);
}

static void
testSizeof()
{
    CHECK(eval("sizeof(int)") == 4);
    CHECK(eval("sizeof(u8)") == 1);
    CHECK(eval("sizeof(i64)") == 8);
    CHECK(eval("sizeof(array[3] of i16)") == 6);
    CHECK(eval("sizeof(array[2][3] of i32)") == 24);
    CHECK(eval("sizeof(-> int)") == sizeof(void *));
    CHECK(eval("sizeof(1i64)") == 8);
}

static void
testCasts()
{
    CHECK(eval("(u8)300") == 44);
    CHECK(eval("(i8)255") == -1);
    CHECK(eval("(int)-3") == -3);
    CHECK(Type::equals(typeOf("(u16)1"), IntegerType::createUnsigned(16)));
}

static void
testDeclarationSyntax()
{
    auto i32 = IntegerType::createSigned(32);

    // -> int: pointer to int
    auto p = type("-> int");
    CHECK(p && p->isPointer() && Type::equals(p->refType(), i32));

    // C: int *a[10];  ABC: array of 10 pointers to int
    auto a = type("array[10] of -> int");
    CHECK(a && a->isArray() && a->dim() == 10 && a->refType()->isPointer());

    // C: int (*b)[10];  ABC: pointer to array of 10 ints
    auto b = type("-> array[10] of int");
    CHECK(b && b->isPointer() && b->refType()->isArray() &&
          b->refType()->dim() == 10);

    // C: const int *c[10];
    auto c = type("array[10] of -> readonly int");
    CHECK(c && c->refType()->isPointer() &&
          !c->refType()->hasConstFlag() &&
          c->refType()->refType()->hasConstFlag());

    // C: int (* const d)[10]; here: array of readonly pointers to int
    auto d = type("array[10] of readonly -> int");
    CHECK(d && d->refType()->hasConstFlag() &&
          !d->refType()->refType()->hasConstFlag());

    // multidimensional array
    auto m = type("array[2][3] of int");
    CHECK(m && m->dim() == 2 && m->refType()->isArray() &&
          m->refType()->dim() == 3);

    // array dimension can be a constant expression
    auto e = type("array[2 * 3 + 1] of u8");
    CHECK(e && e->dim() == 7);

    // function types; parameter names are ignored
    auto f = type("fn(:int): int");
    CHECK(f && f->isFunction() && f->paramType().size() == 1);
    CHECK(Type::equals(f, type("fn(value: int): int")));
    auto g = type("-> fn(:int)");
    CHECK(g && g->isPointer() && g->refType()->retType()->isVoid());
    auto v = type("fn(fmt: -> char, ...): int");
    CHECK(v && v->hasVarg() && v->paramType().size() == 1);

    // pointer to function with two int parameters returning a pointer to a
    // function with one int parameter returning a pointer to int
    auto h = type("-> fn(:int, :int): -> fn(:int): -> int");
    CHECK(h && h->refType()->paramType().size() == 2);
    CHECK(h && h->refType()->retType()->isPointer() &&
          h->refType()->retType()->refType()->retType()->isPointer() &&
          Type::equals(
              h->refType()->retType()->refType()->retType()->refType(),
              i32));

    // not a type
    CHECK(!type("42"));
}

int
main(int argc, char *argv[])
{
    initTypeSystem();
    gen::init("test_parser");
    Symtab root;
    initDefaultType();

    testPrecedence();
    testAssociativity();
    testConditionalSyntax();
    testLiteralTypes();
    testSizeof();
    testCasts();
    testDeclarationSyntax();
    return check::summary(argv[0]);
}
