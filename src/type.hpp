#ifndef TYPE_HPP
#define TYPE_HPP

#include <cstddef>
#include <variant>
#include <vector>
#include <ostream>

class Type
{
    public:
	enum Id {
	    INTEGER,
	    POINTER,
	    FUNCTION,
	} id;

	// for integer (sub-)types 

	enum IntegerKind {
	    SIGNED,
	    UNSIGNED,
	};

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
	    return std::get<IntegerData>(data).numBits;
	}

	// for pointer (sub-)types 

	bool isPointer() const
	{
	    return id == POINTER;
	}

	const Type *getRefType() const
	{
	    return std::get<PointerData>(data).refType;
	}

	// for function (sub-)types 
	bool isFunction() const
	{
	    return id == FUNCTION;
	}

	const Type *getRetType() const
	{
	    return std::get<FunctionData>(data).retType;
	}

	const std::vector<const Type *> &getArgType() const
	{
	    return std::get<FunctionData>(data).argType;
	}

    protected:
	struct IntegerData {
	    std::size_t numBits;
	    IntegerKind kind;
	};

	struct PointerData {
	    const Type *refType;
	};

	struct FunctionData {
	    const Type *retType;
	    std::vector<const Type *> argType;
	};

	std::variant<IntegerData, PointerData, FunctionData> data;

	template <typename Data>
	Type(Id id, Data &&data) : id{id}, data{data} {}

    public:
	static const Type *getUnsignedInteger(std::size_t numBits);
	static const Type *getSignedInteger(std::size_t numBits);
	static const Type *getPointer(const Type *refType);
	static const Type *getFunction(const Type *retType,
				       std::vector<const Type *> argType);

};

std::ostream &operator<<(std::ostream &out, const Type *type);

#endif // TYPE_HPP
