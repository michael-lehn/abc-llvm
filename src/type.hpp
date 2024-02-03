#ifndef TYPE_HPP
#define TYPE_HPP

#include <cassert>
#include <cstddef>
#include <ostream>
#include <unordered_map>
#include <variant>
#include <vector>

#include "lexer.hpp" // for Token::Loc
#include "ustr.hpp"

class Type
{
    public:
	enum Id {
	    VOID,
	    INTEGER,
	    POINTER,
	    ARRAY,
	    FUNCTION,
	    STRUCT,
	} id;

	UStr getAliasIdent() const;
	// only types with size can be used to define a variabel
	bool hasSize() const;
	bool isVoid() const;

	// for integer (sub-)types 
	enum IntegerKind {
	    SIGNED,
	    UNSIGNED,
	};

	bool isBool() const;
	bool hasConstFlag() const;
	bool isInteger() const;
	IntegerKind getIntegerKind() const;
	std::size_t getIntegerNumBits() const;

	// for pointer and array (sub-)types
	bool isPointer() const;
	bool isNullPointer() const;
	bool isArray() const;
	bool isArrayOrPointer() const;
	const Type *getRefType() const;
	std::size_t getDim() const;

	// for function (sub-)types 
	bool isFunction() const;
	const Type *getRetType() const;
	bool hasVarg() const;
	const std::vector<const Type *> &getArgType() const;

	// for struct (sub-)types
	bool isStruct() const;
	const Type *complete(std::vector<const char *> &&ident,
			     std::vector<const Type *> &&type);
	UStr getName() const;
	std::size_t getNumMembers() const;
	bool hasMember(UStr ident) const;
	std::size_t getMemberIndex(UStr ident) const;
	const Type *getMemberType(std::size_t index) const;
	const Type *getMemberType(UStr ident) const;
	const std::vector<const Type *> &getMemberType() const;
	const std::vector<const char *> &getMemberIdent() const;

    protected:
	struct IntegerData {
	    std::size_t numBits;
	    IntegerKind kind;
	    bool constFlag;
	};

	struct PointerData {
	    const Type * const refType;
	    const bool isNullptr;
	    const bool constFlag;
	};

	struct ArrayData {
	    const Type *refType;
	    std::size_t dim;
	};

	struct FunctionData {
	    const Type *retType;
	    std::vector<const Type *> argType;
	    bool hasVarg;
	};

	struct StructData {
	    std::size_t id;
	    UStr name; // needed for getting non-const type and printing type
	    bool isComplete;
	    bool constFlag;
	  
	    // for complete struct types:
	    std::unordered_map<const char *, std::size_t> index;
	    std::vector<const Type *> type;
	    std::vector<const char *> ident;

	    StructData(std::size_t id, UStr name);
	    StructData(const StructData &data, bool constFlag);
	};

	std::variant<IntegerData, PointerData, ArrayData, FunctionData,
		     StructData> data;
	UStr aliasIdent;

	template <typename Data>
	Type(Id id, Data &&data, UStr aliasIdent = UStr{})
	    : id{id}, data{data}, aliasIdent{aliasIdent}
	{}

    public:
	static const Type *createAlias(UStr aliasIdent, const Type *forType);
	static const Type *getConst(const Type *type);
	static const Type *getConstRemoved(const Type *type);
	static const Type *getVoid();
	static const Type *getChar();
	static const Type *getBool();
	static const Type *getUnsignedInteger(std::size_t numBits);
	static const Type *getSignedInteger(std::size_t numBits);
	static const Type *getPointer(const Type *refType);
	static const Type *getNullPointer();
	static const Type *getArray(const Type *refType, std::size_t dim);
	static const Type *getFunction(const Type *retType,
				       std::vector<const Type *> argType,
				       bool hasVarg = false);
	static Type *createIncompleteStruct(UStr name);
	static void remove(const Type *ty);

	// type casts
	static const Type *getTypeConversion(const Type *from, const Type *to,
					     Token::Loc loc = Token::Loc{});
	static const Type *convertArrayOrFunctionToPointer(const Type *ty);
};

bool operator==(const Type &x, const Type &y);
std::ostream &operator<<(std::ostream &out, const Type *type);
void printTypeSet();

#endif // TYPE_HPP
