#ifndef GEN_VARIABLE_HPP
#define GEN_VARIABLE_HPP

#include "type/type.hpp"

#include "gen.hpp"

namespace gen {

bool externalVariableDeclaration(const char *ident, const abc::Type *varType);

void globalVariableDefinition(const char *ident, const abc::Type *varType,
			      Constant initialValue = nullptr);

Constant loadStringAddress(const char *str);

Value localVariableDefinition(const char *ident, const abc::Type *varType);


void forgetAllVariables();
void forgetAllLocalVariables();

bool hasConstantAddress(const char *ident);
Constant loadConstantAddress(const char *ident);
Value loadAddress(const char *ident);

Constant pointerIncrement(const abc::Type *type, Constant pointer,
			  std::uint64_t offset);
Value pointerIncrement(const abc::Type *type, Value pointer, Value offset);

std::optional<Constant> pointerConstantDifference(const abc::Type *type,
						  Value pointer1,
						  Value pointer2);
Value pointerDifference(const abc::Type *type, Value pointer1, Value pointer2);
Value pointerToIndex(const abc::Type *type, Value pointer, std::size_t index);

Value fetch(Value addr, const abc::Type *type);
Value store(Value val, Value addr);

// for debugging and educational purposes
void printGlobalVariableList();
void printFunctionList();

} // namespace gen

#endif // GEN_VARIABLE_HPP
