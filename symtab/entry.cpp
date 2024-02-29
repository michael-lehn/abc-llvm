#include "entry.hpp"

namespace abc { namespace symtab {

Entry::Entry(lexer::Loc loc, UStr id, const Type *type)
    : loc{loc}, id{id}, type{type}
{
}

} } // namespace symtab, abc

