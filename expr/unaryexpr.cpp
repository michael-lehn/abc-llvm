#include <cassert>
#include <iostream>
#include <iomanip>

#include "gen/constant.hpp"
#include "gen/instruction.hpp"
#include "gen/variable.hpp"
#include "lexer/error.hpp"
#include "type/integertype.hpp"

#include "promotion.hpp"
#include "unaryexpr.hpp"

static const char * kindStr(abc::UnaryExpr::Kind kind);

//------------------------------------------------------------------------------

namespace abc {

UnaryExpr::UnaryExpr(Kind kind, ExprPtr &&child, const Type *type,
		     lexer::Loc loc)
    : Expr{loc, type}, kind{kind}, child{std::move(child)}
{
}

ExprPtr
UnaryExpr::create(Kind kind, ExprPtr &&child, lexer::Loc loc)
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
UnaryExpr::hasAddress() const
{
    return isLValue();
}

bool
UnaryExpr::isLValue() const
{
    return kind == ARROW_DEREF || kind == ASTERISK_DEREF;
}

//-- for checking constness 

bool
UnaryExpr::isConst() const
{
    switch (kind) {
	case LOGICAL_NOT:
	case MINUS:
	    return child->isConst();
	case ADDRESS:
	    return child->hasConstantAddress();
	case ARROW_DEREF:
	case ASTERISK_DEREF:
	case POSTFIX_INC:
	case POSTFIX_DEC:
	case PREFIX_DEC:
	case PREFIX_INC:
	    return false;
	default:
	    assert(0 && "internal error: case not handled");
	    return false;
    }
}

gen::Constant
UnaryExpr::loadConstant() const
{
    assert(type);
    assert(isConst());
    switch (kind) {
	default:
	    assert(0);
	    return nullptr;
	case LOGICAL_NOT:
	case MINUS:
	    assert(0 && "Not implemented");
	    return nullptr;
	case ADDRESS:
	    assert(0 && "Not implemented");
	    return nullptr;
    }
}

// for code generation

gen::Value
UnaryExpr::loadValue() const
{
    assert(type);

    switch (kind) {
	case LOGICAL_NOT:
	    return gen::instruction(gen::EQ,
				    child->loadValue(),
				    gen::getConstantZero(child->type));
	case ARROW_DEREF:
	case ASTERISK_DEREF:
	    if (child->type->isNullptr()) {
		error::out() << loc
		    << ": Error: dereferencing null pointer" << std::endl;
		error::fatal();
	    }
	    return gen::fetch(child->loadValue(), type);
	case ADDRESS:
	    return child->loadAddress();
	case PREFIX_INC:
	case PREFIX_DEC:
	    {
		auto incType = type->isPointer()
		    ? IntegerType::createSigned(8)
		    : child->type;
		auto inc = kind == PREFIX_INC
		    ? gen::getConstantInt(1, incType)
		    : gen::getConstantInt(-1, incType);
		gen::Value val = type->isPointer()
		    ? gen::pointerIncrement(child->type->refType(),
					    child->loadValue(), inc)
		    : gen::instruction(gen::ADD, child->loadValue(), inc);
		gen::store(val, child->loadAddress());
		return val;
	    }
	case POSTFIX_INC:
	case POSTFIX_DEC:
	    {
		auto prevLeftVal = child->loadValue();
		auto incType = type->isPointer()
		    ? IntegerType::createSigned(8)
		    : child->type;
		auto inc = kind == POSTFIX_INC
		    ? gen::getConstantInt(1, incType)
		    : gen::getConstantInt(-1, incType);
		gen::Value val = type->isPointer()
		    ? gen::pointerIncrement(child->type->refType(),
					    prevLeftVal, inc)
		    : gen::instruction(gen::ADD, prevLeftVal, inc);
		gen::store(val, child->loadAddress());
		return prevLeftVal;
	    }
	case MINUS:
	    return gen::instruction(gen::SUB,
				    gen::getConstantZero(type), 
				    child->loadValue());
	default:
	    assert(0);
	    return nullptr;
    }
}

gen::Value
UnaryExpr::loadAddress() const
{
    assert(hasAddress());
    assert(kind == ARROW_DEREF || kind == ASTERISK_DEREF);
    return child->loadValue();
}

void
UnaryExpr::condition(gen::Label trueLabel, gen::Label falseLabel) const
{
    assert(type);
    switch (kind) {
	case LOGICAL_NOT:
	    {
		child->condition(falseLabel, trueLabel);
		return;
	    }
	default:
	    gen::jumpInstruction(gen::instruction(gen::NE,
						  loadValue(),
						  gen::getConstantZero(type)),
				 trueLabel,
				 falseLabel);
    }
}


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
UnaryExpr::printFlat(std::ostream &out, int prec) const
{
    switch (kind) {
	case UnaryExpr::ARROW_DEREF:
	    if (prec > 16) { out << "("; }
	    child->printFlat(out, 16); out << "->";
	    if (prec > 16) { out << ")"; }
	    break;
	case UnaryExpr::ASTERISK_DEREF:
	    if (prec > 15) { out << "("; }
	    out << "*"; child->printFlat(out, 15);
	    if (prec > 15) { out << ")"; }
	    break;
	case UnaryExpr::LOGICAL_NOT:
	    if (prec > 15) { out << "("; }
	    out << "!"; child->printFlat(out, 15);
	    if (prec > 15) { out << ")"; }
	    break;
	case UnaryExpr::ADDRESS:
	    if (prec > 15) { out << "("; }
	    out << "&"; child->printFlat(out, 15);
	    if (prec > 15) { out << ")"; }
	    break;
	case UnaryExpr::PREFIX_INC:
	    if (prec > 15) { out << "("; }
	    out << "++"; child->printFlat(out, 15);
	    if (prec > 15) { out << ")"; }
	    break;
	case UnaryExpr::PREFIX_DEC:
	    if (prec > 15) { out << "("; }
	    out << "--"; child->printFlat(out, 15);
	    if (prec > 15) { out << ")"; }
	    break;
	case UnaryExpr::POSTFIX_INC:
	    if (prec > 16) { out << "("; }
	    child->printFlat(out, 16); out << "++";
	    if (prec > 16) { out << ")"; }
	    break;
	case UnaryExpr::POSTFIX_DEC:
	    if (prec > 16) { out << "("; }
	    child->printFlat(out, 16); out << "--";
	    if (prec > 16) { out << ")"; }
	    break;
	case UnaryExpr::MINUS:
	    if (prec > 16) { out << "("; }
	    out << "-"; child->printFlat(out, 16);
	    if (prec > 16) { out << ")"; }
	    break;
	default:
	    out << " <unary kind " << kind << ">";
    }
}

} // namespace abc

//------------------------------------------------------------------------------

/*
 * Auxiliary functions
 */

static const char *
kindStr(abc::UnaryExpr::Kind kind)
{
    switch (kind) {
	case abc::UnaryExpr::ASTERISK_DEREF: return "* (deref)";
	case abc::UnaryExpr::ARROW_DEREF: return "-> (deref)";
	case abc::UnaryExpr::LOGICAL_NOT: return "! (logical)";
	case abc::UnaryExpr::ADDRESS: return "& (address)";
	case abc::UnaryExpr::POSTFIX_INC: return "++ (postfix)";
	case abc::UnaryExpr::POSTFIX_DEC: return "-- (postfix)";
	case abc::UnaryExpr::MINUS: return "- (unary)";
	default: return "?? (unary)";
    }
}
