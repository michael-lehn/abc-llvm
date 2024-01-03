#ifndef SYMTAB_HPP
#define SYMTAB_HPP

#include <ostream>

#include "lexer.hpp"
#include "types.hpp"
#include "ustr.hpp"

namespace symtab {

enum Scope {
    AnyScope,
    CurrentScope,
    RootScope,
};

struct SymEntry
{
    Token::Loc loc;
    const Type *type;
    UStr ident;
};

void openScope(void);
void closeScope(void);

SymEntry *get(UStr ident, Scope scope = AnyScope);

// Add a new symbol to current scope. Returns nullptr if symbol already exists,
// otherwise a pointer to the added entry.
SymEntry *add(Token::Loc loc, UStr ident, const Type *type);

// Add a new symbol to root scope. Returns nullptr if symbol already exists,
// otherwise a pointer to the added entry.
SymEntry *addToRootScope(Token::Loc loc, UStr ident, const Type *type);

std::ostream &print(std::ostream &out);

} // namespace symtab

#endif // SYMTAB_HPP
