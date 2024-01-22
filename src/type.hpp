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

	// only types with size can be used to define a variabel
	bool hasSize() const
	{
	    if (isVoid()) {
		return false;
	    } else if (isStruct()) {
		return std::get<StructData>(data).isComplete;
	    } else {
		return true;
	    }
	}

	bool isVoid() const
	{
	    return id == VOID;
	}

	// for integer (sub-)types 

	enum IntegerKind {
	    SIGNED,
	    UNSIGNED,
	};

	bool isBool() const
	{
	    return id == INTEGER && getIntegerNumBits() == 1;
	}

	bool isInteger() const
	{
	    return id == INTEGER;
	}

	IntegerKind getIntegerKind() const
	{
	    return std::get<IntegerData>(data).kind;
	}

	std::size_t getIntegerNumBits() const
	{
	    assert(std::holds_alternative<IntegerData>(data));
	    return std::get<IntegerData>(data).numBits;
	}

	// for pointer and array (sub-)types

	bool isPointer() const
	{
	    return id == POINTER;
	}

	bool isNullPointer() const
	{
	    if (!isPointer()) {
		return false;
	    }
	    assert(std::holds_alternative<PointerData>(data));
	    return std::get<PointerData>(data).isNullptr;
	}

	bool isArray() const
	{
	    return id == ARRAY;
	}

	bool isArrayOrPointer() const
	{
	    return isArray() || isPointer();
	}

	const Type *getRefType() const
	{
	    assert(!isNullPointer());
	    if (std::holds_alternative<ArrayData>(data)) {
		return std::get<ArrayData>(data).refType;
	    }
	    assert(std::holds_alternative<PointerData>(data));
	    return std::get<PointerData>(data).refType;
	}

	std::size_t getDim() const
	{
	    assert(std::holds_alternative<ArrayData>(data));
	    return std::get<ArrayData>(data).dim;
	}

	// for function (sub-)types 
	bool isFunction() const
	{
	    return id == FUNCTION;
	}

	const Type *getRetType() const
	{
	    assert(std::holds_alternative<FunctionData>(data));
	    return std::get<FunctionData>(data).retType;
	}

	bool hasVarg() const
	{
	    assert(std::holds_alternative<FunctionData>(data));
	    return std::get<FunctionData>(data).hasVarg;
	}

	const std::vector<const Type *> &getArgType() const
	{
	    assert(std::holds_alternative<FunctionData>(data));
	    return std::get<FunctionData>(data).argType;
	}

	// for struct (sub-)types
	bool isStruct() const
	{
	    return id == STRUCT;
	}

	const Type *complete(std::vector<const char *> &&ident,
			     std::vector<const Type *> &&type)
	{
	    assert(std::holds_alternative<StructData>(data));
	    assert(ident.size() == type.size());
	    auto &structData = std::get<StructData>(data);

	    if (structData.isComplete) {
		// struct members already defined
		return nullptr;
	    }

	    structData.isComplete = true;	
	    structData.ident = ident;	
	    structData.type = type;
	    for (std::size_t i = 0; i < ident.size(); ++i) {
		structData.index[ident.at(i)] = i;
	    }
	    return this;
	}

	UStr getName(void) const
	{
	    assert(std::holds_alternative<StructData>(data));
	    const auto &structData = std::get<StructData>(data);
	    return structData.name;
	}


	bool hasMember(UStr ident) const
	{
	    assert(std::holds_alternative<StructData>(data));
	    const auto &structData = std::get<StructData>(data);
	    return structData.index.contains(ident.c_str());
	}

	std::size_t getMemberIndex(UStr ident) const
	{
	    assert(std::holds_alternative<StructData>(data));
	    const auto &structData = std::get<StructData>(data);
	    assert(hasMember(ident));
	    return structData.index.at(ident.c_str());
	}

	const Type *getMemberType(UStr ident) const
	{
	    assert(std::holds_alternative<StructData>(data));
	    const auto &structData = std::get<StructData>(data);
	    assert(hasMember(ident));
	    return structData.type[getMemberIndex(ident)];
	}

	const std::vector<const Type *> &getMemberType(void) const
	{
	    assert(std::holds_alternative<StructData>(data));
	    const auto &structData = std::get<StructData>(data);
	    return structData.type;
	}

	const std::vector<const char *> &getMemberIdent(void) const
	{
	    assert(std::holds_alternative<StructData>(data));
	    const auto &structData = std::get<StructData>(data);
	    return structData.ident;
	}


    protected:
	struct IntegerData {
	    std::size_t numBits;
	    IntegerKind kind;
	};

	struct PointerData {
	    const Type *refType;
	    bool isNullptr;
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
	    UStr name; // only needed for error messages
	    bool isComplete = false;
	  
	    // info about members:
	    std::unordered_map<const char *, std::size_t> index;
	    std::vector<const Type *> type;
	    std::vector<const char *> ident;

	    StructData(std::size_t id, UStr name)
		: id{id}, name{name}
	    {}
	};

	std::variant<IntegerData, PointerData, ArrayData, FunctionData,
		     StructData> data;

	template <typename Data>
	Type(Id id, Data &&data) : id{id}, data{data} {}

    public:
	// for getting a unnamed type
	static const Type *getVoid(void);
	static const Type *getBool(void);
	static const Type *getUnsignedInteger(std::size_t numBits);
	static const Type *getSignedInteger(std::size_t numBits);
	static const Type *getPointer(const Type *refType);
	static const Type *getNullPointer(void);
	static const Type *getArray(const Type *refType, std::size_t dim);
	static const Type *getFunction(const Type *retType,
				       std::vector<const Type *> argType,
				       bool hasVarg = false);
	static Type *createIncompleteStruct(UStr name);
	static void deleteStruct(const Type *ty);

	// type casts
	static const Type *getTypeConversion(const Type *from, const Type *to,
					     Token::Loc loc = Token::Loc{});
	static const Type *convertArrayOrFunctionToPointer(const Type *ty);
};

std::ostream &operator<<(std::ostream &out, const Type *type);

#endif // TYPE_HPP
