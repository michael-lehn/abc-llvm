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
	InitializerList() = default;
	InitializerList(const Type *type);

	const Type *type() const;
	bool isConst() const;

	void set(ExprPtr &&expr);
	void add(ExprPtr &&expr);
	void add(InitializerList &&initList, Token::Loc loc = Token::Loc{});

	void store(gen::Reg addr) const;
	void store(size_t index, gen::Reg addr) const;

	gen::ConstVal loadConstValue() const;
	gen::ConstVal loadConstValue(size_t index) const;

	void print(int indent = 0) const;
	void printFlat(std::ostream &out, bool isFactor) const;

    private:
	const Type *type_ = nullptr;
	std::size_t pos = 0;
	std::vector<std::variant<ExprPtr, InitializerList>> value;
	std::vector<const Type *> valueType;
	std::vector<Token::Loc> valueLoc;
};


#endif // CONSTEXPR_HPP
