#ifndef SYMTAB_SYMTAB
#define SYMTAB_SYMTAB

#include <cstddef>
#include <forward_list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "expr/expr.hpp"
#include "lexer/loc.hpp"

#include "entry.hpp"

namespace abc {

class Symtab
{
    public:
	enum Scope
	{
	    AnyScope,
	    CurrentScope,
	};

	Symtab(UStr prefix = UStr{});
	~Symtab();

	static std::vector<std::string> didYouMean(UStr name);

	static const symtab::Entry *find(UStr name, Scope inScope);
	static const symtab::Entry *type(UStr name, Scope inScope);
	static const symtab::Entry *variable(UStr name, Scope inScope);
	static const symtab::Entry *constant(UStr name, Scope inScope);

	static std::pair<symtab::Entry *, bool>
	addDeclaration(lexer::Loc loc, UStr name, const Type *type);

	static std::pair<symtab::Entry *, bool>
	addDefinition(lexer::Loc loc, UStr name, const Type *type);

	static std::pair<symtab::Entry *, bool>
	addType(lexer::Loc loc, UStr name, const Type *type);

	static std::pair<symtab::Entry *, bool>
	addExpression(lexer::Loc loc, UStr name, const Expr *expr);

	static void print(std::ostream &out);

    private:
	static std::pair<symtab::Entry *, bool> add(UStr name,
	                                            symtab::Entry &&entry);

	static UStr getId(UStr name);

	using ScopeNode = std::unordered_map<UStr, symtab::Entry>;
	static std::forward_list<std::unique_ptr<ScopeNode>> scope;
	static std::size_t scopeSize;
	static UStr scopePrefix;
};

} // namespace abc

#endif // SYMTAB_SYMTAB
