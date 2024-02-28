#ifndef GEN_VARIABLE_HPP
#define GEN_VARIABLE_HPP

#include "type/type.hpp"

#include "gen.hpp"

namespace gen {

void globalVariableDefinition(const char *ident, const abc::Type *varType,
			      Constant initialValue, bool externalLinkage);

Value localVariableDefinition(const char *ident, const abc::Type *varType);

void forgetAllLocalVariables();

Value loadAddress(const char *ident);
Value fetch(Value addr, const abc::Type *type);
Value store(Value val, Value addr);

// for debugging and educational purposes
void printGlobalVariableList();
void printFunctionList();

} // namespace gen

#endif // GEN_VARIABLE_HPP
