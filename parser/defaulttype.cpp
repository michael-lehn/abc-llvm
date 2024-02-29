#include "symtab/symtab.hpp"
#include "type/integertype.hpp"
#include "type/voidtype.hpp"

#include "defaulttype.hpp"

namespace abc {

void
initDefaultType()
{
    Symtab::addType(lexer::Loc{}, UStr::create("void"),
		    VoidType::create());

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
