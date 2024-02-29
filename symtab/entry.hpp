#ifndef SYMTAB_ENTRY
#define SYMTAB_ENTRY

#include "lexer/loc.hpp"
#include "type/type.hpp"

namespace abc { namespace symtab {

class Entry
{
    public:
	Entry(lexer::Loc loc, UStr id, const Type *type);
	const lexer::Loc loc;
	const UStr id;
	const Type *type;
};

} } // namespace symtab, abc

#endif // SYMTAB_ENTRY
