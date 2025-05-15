#include "symtab/symtab.hpp"
#include "type/floattype.hpp"
#include "type/integertype.hpp"
#include "type/voidtype.hpp"

#include "defaulttype.hpp"

namespace abc {

void
initDefaultType()
{
    Symtab::addType(lexer::Loc{}, UStr::create("void"),
                    VoidType::create()->getAlias("void"));

    Symtab::addType(lexer::Loc{}, UStr::create("float"),
                    FloatType::createFloat());
    Symtab::addType(lexer::Loc{}, UStr::create("double"),
                    FloatType::createDouble());

    Symtab::addType(lexer::Loc{}, UStr::create("bool"),
                    IntegerType::createBool()->getAlias("bool"));
    Symtab::addType(lexer::Loc{}, UStr::create("char"),
                    IntegerType::createChar()->getAlias("char"));
    Symtab::addType(lexer::Loc{}, UStr::create("int"),
                    IntegerType::createInt()->getAlias("int"));
    Symtab::addType(lexer::Loc{}, UStr::create("unsigned"),
                    IntegerType::createUnsigned()->getAlias("unsigned"));
    Symtab::addType(lexer::Loc{}, UStr::create("long"),
                    IntegerType::createInt()->getAlias("long"));
    Symtab::addType(lexer::Loc{}, UStr::create("size_t"),
                    IntegerType::createSizeType()->getAlias("size_t"));
    Symtab::addType(lexer::Loc{}, UStr::create("ptrdiff_t"),
                    IntegerType::createSizeType()->getAlias("ptrdiff_t"));

    Symtab::addType(lexer::Loc{}, UStr::create("i8"),
                    IntegerType::createSigned(8));
    Symtab::addType(lexer::Loc{}, UStr::create("i16"),
                    IntegerType::createSigned(16));
    Symtab::addType(lexer::Loc{}, UStr::create("i32"),
                    IntegerType::createSigned(32));
    Symtab::addType(lexer::Loc{}, UStr::create("i64"),
                    IntegerType::createSigned(64));

    Symtab::addType(lexer::Loc{}, UStr::create("u8"),
                    IntegerType::createUnsigned(8));
    Symtab::addType(lexer::Loc{}, UStr::create("u16"),
                    IntegerType::createUnsigned(16));
    Symtab::addType(lexer::Loc{}, UStr::create("u32"),
                    IntegerType::createUnsigned(32));
    Symtab::addType(lexer::Loc{}, UStr::create("u64"),
                    IntegerType::createUnsigned(64));
}

} // namespace abc
