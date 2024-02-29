#ifndef SYMTAB_SYMTAB
#define SYMTAB_SYMTAB

#include <cstddef>
#include <forward_list>
#include <memory>
#include <unordered_map>

#include "lexer/loc.hpp"

#include "entry.hpp"

namespace abc {

class Symtab
{
    public:

	enum Scope {
	    AnyScope,
	    CurrentScope,
	};

	Symtab(UStr prefix = UStr{});
	~Symtab();

	static const symtab::Entry *find(UStr name, Scope inScope = AnyScope);
	static const symtab::Entry *type(UStr name, Scope inScope = AnyScope);

	static std::pair<symtab::Entry *, bool>
	    addDeclaration(lexer::Loc loc, UStr name, const Type *type);

	static std::pair<symtab::Entry *, bool>
	    addType(lexer::Loc loc, UStr name, const Type *type);

	static void print(std::ostream &out);

    private:
	static std::pair<symtab::Entry *, bool>
	    add(UStr name, symtab::Entry &&entry);

	static UStr getId(UStr name);

	using ScopeNode = std::unordered_map<UStr, symtab::Entry>;
	static std::forward_list<std::unique_ptr<ScopeNode>> scope;
	static std::size_t scopeSize;
	static UStr scopePrefix;
};
	
} // namespace abc

#endif // SYMTAB_SYMTAB
