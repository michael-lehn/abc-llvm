#ifndef CONSTEXPR_HPP
#define CONSTEXPR_HPP

#include <cstdio>
#include <variant>
#include <vector>

#include "expr.hpp"
#include "gen.hpp"
#include "type.hpp"

class InitializerList
{
    public:
	InitializerList(const Type *type = nullptr);

	const Type *getType() const;
	bool isConst() const;

	// only use: if type was not already set in constructor
	void setType(const Type *ty);

	void add(ExprPtr &&expr, Token::Loc loc = Token::Loc{});
	void add(InitializerList &&initList, Token::Loc loc = Token::Loc{});

	void store(gen::Reg addr) const;
	void store(size_t index, gen::Reg addr) const;

	gen::ConstVal loadConst() const;
	gen::ConstVal loadConst(size_t index) const;

	void print(int indent = 0) const;

    private:
	const Type *type;
	std::size_t pos;
	std::vector<std::variant<ExprPtr, InitializerList>> value;
	std::vector<const Type *> valueType;
	std::vector<Token::Loc> valueLoc;
};


#endif // CONSTEXPR_HPP