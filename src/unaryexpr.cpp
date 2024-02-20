#include <cassert>
#include <iostream>
#include <iomanip>

#include "error.hpp"
#include "identifier.hpp"
#include "promotion.hpp"
#include "unaryexpr.hpp"

UnaryExpr::UnaryExpr(Kind kind, ExprPtr &&child, const Type *type,
		     Token::Loc loc)
    : Expr{loc, type}, kind{kind}, child{std::move(child)}
{
}

ExprPtr
UnaryExpr::create(Kind kind, ExprPtr &&child, Token::Loc loc)
{
    if (!child) {
	error::out() << loc << "error: expression expected after operator"
	    << std::endl;
	error::fatal();
	return nullptr;
    }
    auto promotion = promotion::unary(kind, std::move(child), &loc);
    auto p = new UnaryExpr{kind,
			   std::move(std::get<0>(promotion)),
			   std::move(std::get<1>(promotion)),
			   loc};
    return std::unique_ptr<UnaryExpr>{p};
}

bool
UnaryExpr::hasAddr() const
{
    return isLValue();
}

bool
UnaryExpr::isLValue() const
{
    return kind == DEREF;
}

//-- for checking constness 

bool
UnaryExpr::isConst() const
{
    switch (kind) {
	case DEREF:
	case LOGICAL_NOT:
	case MINUS:
	    return child->isConst();
	case ADDRESS:
	    return true;
	case POSTFIX_INC:
	case POSTFIX_DEC:
	    return false;
	default:
	    assert(0 && "internal error: case not handled");
	    return false;
    }
}

gen::ConstVal
UnaryExpr::loadConstValue() const
{
    using T = std::remove_pointer_t<gen::ConstVal>;
    return llvm::dyn_cast<T>(loadValue());
}

// for code generation

gen::Reg
UnaryExpr::loadValue() const
{
    assert(type);

    switch (kind) {
	case LOGICAL_NOT:
	    return gen::cond(gen::EQ,
			     child->loadValue(),
			     gen::loadIntConst(0, child->type));
	case DEREF:
	    if (child->type->isNullPointer()) {
		error::out() << loc
		    << ": Error: dereferencing null pointer" << std::endl;
		error::fatal();
	    }
	    return gen::fetch(child->loadValue(), type);
	case ADDRESS:
	    return child->loadAddr();
	case POSTFIX_INC:
	case POSTFIX_DEC:
	    {
		auto prevLeftVal = child->loadValue();
		auto incType = type->isPointer()
		    ? Type::getSignedInteger(8)
		    : child->type;
		auto inc = kind == POSTFIX_INC
		    ? gen::loadIntConst(1, incType, true)
		    : gen::loadIntConst(-1, incType, true);
		gen::Reg val = type->isPointer()
		    ? gen::ptrInc(child->type->getRefType(), prevLeftVal, inc)
		    : gen::aluInstr(gen::ADD, prevLeftVal, inc);
		gen::store(val, child->loadAddr(), type);
		return prevLeftVal;
	    }
	case MINUS:
	    return gen::aluInstr(gen::SUB,
				 gen::loadIntConst(0, type), 
				 child->loadValue());

	default:
	    assert(0);
	    return nullptr;
    }
}

gen::Reg
UnaryExpr::loadAddr() const
{
    assert(hasAddr());
    assert(kind == DEREF);
    return child->loadValue();
}

void
UnaryExpr::condJmp(gen::Label trueLabel, gen::Label falseLabel) const
{
    assert(type);
    switch (kind) {
	case LOGICAL_NOT:
	    {
		child->condJmp(falseLabel, trueLabel);
		return;
	    }
	default:
	    gen::jmp(gen::cond(gen::NE,
			       loadValue(),
			       gen::loadIntConst(0, type)),
		     trueLabel,
		     falseLabel);
    }
}

// for debugging and educational purposes
static const char * kindStr(UnaryExpr::Kind kind);

void
UnaryExpr::print(int indent) const
{
    if (indent) {
	std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    }
    std::cerr << kindStr(kind) << " [ " << type << " ] " << std::endl;
    child->print(indent + 4);
}

void
UnaryExpr::printFlat(std::ostream &out, bool isFactor) const
{
    switch (kind) {
	case UnaryExpr::DEREF:
	    out << "*" << child;
	    break;
	case UnaryExpr::LOGICAL_NOT:
	    out << "!" << child;
	    break;
	case UnaryExpr::ADDRESS:
	    out << "&" << child;
	    break;
	case UnaryExpr::POSTFIX_INC:
	    out << child << "++";
	    break;
	case UnaryExpr::POSTFIX_DEC:
	    out << child << "--";
	    break;
	case UnaryExpr::MINUS:
	    out << "-" << child;
	    break;
	default:
	    out << " <unary kind " << kind << ">";
    }
}

static const char *
kindStr(UnaryExpr::Kind kind)
{
    switch (kind) {
	case UnaryExpr::DEREF: return "* (deref)";
	case UnaryExpr::LOGICAL_NOT: return "! (logical)";
	case UnaryExpr::ADDRESS: return "& (address)";
	case UnaryExpr::POSTFIX_INC: return "++ (postfix)";
	case UnaryExpr::POSTFIX_DEC: return "-- (postfix)";
	case UnaryExpr::MINUS: return "- (unary)";
	default: return "?? (unary)";
    }
}
