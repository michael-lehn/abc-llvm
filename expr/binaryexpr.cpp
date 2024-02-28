#include <cassert>
#include <iostream>
#include <iomanip>

#include "gen/instruction.hpp"
#include "lexer/error.hpp"

#include "binaryexpr.hpp"
#include "identifier.hpp"
#include "promotion.hpp"

static const char *kindStr(abc::BinaryExpr::Kind kind);
static gen::AluOp getGenAluOp(abc::BinaryExpr::Kind kind,
			      const abc::Type *type);
/*
static gen::CondOp getGenCondOp(abc::BinaryExpr::Kind kind,
				const abc::Type *type);
*/

static void printFlat(std::ostream &out, int precCaller, int prec,
		      const abc::Expr *left, const abc::Expr *right,
		      const char *op);

//------------------------------------------------------------------------------

namespace abc {

BinaryExpr::BinaryExpr(Kind kind, ExprPtr &&left, ExprPtr &&right,
		       const Type *type, lexer::Loc loc)
    : Expr{loc, type}, kind{kind}, left{std::move(left)}
    , right{std::move(right)}
{
}

ExprPtr
BinaryExpr::create(Kind kind, ExprPtr &&left, ExprPtr &&right, lexer::Loc loc)
{
    assert(left);
    assert(right);
    auto promotion = promotion::binary(kind, std::move(left),
				       std::move(right), &loc);
    auto p = new BinaryExpr{kind,
			    std::move(std::get<0>(promotion)),
			    std::move(std::get<1>(promotion)),
			    std::get<2>(promotion),
			    loc};
    return std::unique_ptr<BinaryExpr>{p};
}

bool
BinaryExpr::hasAddr() const
{
    return false;
}

bool
BinaryExpr::isLValue() const
{
    return false;
}

//-- for checking constness 

bool
BinaryExpr::isIntegerConstExpr() const
{
    assert(type);
    return type->isInteger()
	&& left->type->isInteger() && left->isConst()
	&& right->type->isInteger() && right->isConst();
}

bool
BinaryExpr::isArithmeticConstExpr() const
{
    return isIntegerConstExpr();
}

bool
BinaryExpr::isAddressConstant() const
{
    assert(type);
    return type->isPointer() && left->isConst() && right->isConst();
}

bool
BinaryExpr::isConst() const
{
    switch (kind) {
	case ASSIGN:
	    return false;
	default:
	    return isArithmeticConstExpr() || isAddressConstant();
    }
}

gen::Constant
BinaryExpr::loadConstant() const
{
    using T = std::remove_pointer_t<gen::Constant>;
    return llvm::dyn_cast<T>(loadValue());
}

// for code generation

gen::Value
BinaryExpr::loadValue() const
{
    assert(type);
    switch (kind) {
	case ASSIGN:
	    assert(0 && "Not implemented");
	    return nullptr;
	    /*
	    return gen::store(right->loadValue(),
			      left->loadAddr(),
			      type);
	    */
	case ADD:
	case SUB:
	case MUL:
	case DIV:
	case MOD:
	    if (kind == ADD && type->isPointer()) {
		// pointer + integer
		assert(left->type->isPointer());
		assert(right->type->isInteger());
		assert(0 && "Not implemented");
		return nullptr;
		/*
		return gen::ptrInc(left->type->getRefType(),
				   left->loadValue(),
				   right->loadValue());
		*/
	    } else if (kind == SUB && left->type->isPointer()) {
		// pointer - pointer
		assert(right->type->isPointer());
		assert(type->isInteger());
		assert(0 && "Not implemented");
		return nullptr;
		/*
		return gen::ptrDiff(left->type->getRefType(),
				    left->loadValue(),
				    right->loadValue());
		*/
	    } else {
		return gen::instruction(getGenAluOp(kind, type),
					left->loadValue(),
					right->loadValue());
	    }
	case LESS:
	case LESS_EQUAL:
	case GREATER:
	case GREATER_EQUAL:
	case NOT_EQUAL:
	case EQUAL:
	    assert(0 && "Not implemented");
	    return nullptr;
	    /*
	    return gen::cond(getGenCondOp(kind, left->type),
			     left->loadValue(),
			     right->loadValue());
	    */
	case LOGICAL_AND:
	case LOGICAL_OR:
	    {
		/*
		auto trueLabel = gen::getLabel("true");
		auto falseLabel = gen::getLabel("false");
		auto phiLabel = gen::getLabel("true");

		condJmp(trueLabel, falseLabel);

		gen::labelDef(trueLabel);
		auto one = gen::loadIntConst(1, type);
		gen::jmp(phiLabel);

		gen::labelDef(falseLabel);
		auto zero = gen::loadIntConst(0, type);
		gen::jmp(phiLabel);

		gen::labelDef(phiLabel);
		return gen::phi(one, trueLabel, zero, falseLabel, type);
		*/
		assert(0 && "Not implemented");
		return nullptr;
	    }
	    
	default:
	    error::out() << "kind = " << int(kind) << std::endl;
	    assert(0);
	    return nullptr;
    }
}

gen::Value
BinaryExpr::loadAddress() const
{
    assert(0 && "Binary expression has no address");
}

void
BinaryExpr::condJmp(gen::Label trueLabel, gen::Label falseLabel) const
{
    assert(0 && "Not implemented");
    /*
    switch (kind) {
	case LESS:
	case LESS_EQUAL:
	case GREATER:
	case GREATER_EQUAL:
	case NOT_EQUAL:
	case EQUAL:
	    {
		assert(*left->type == *right->type);
		auto cond = gen::cond(getGenCondOp(kind, left->type),
				      left->loadValue(),
				      right->loadValue());
		gen::jmp(cond, trueLabel, falseLabel);
		return;
	    }
	case LOGICAL_AND:
            {
		auto chkRightLabel = gen::getLabel("chkRight");

		left->condJmp(chkRightLabel, falseLabel);
		gen::labelDef(chkRightLabel);
		right->condJmp(trueLabel, falseLabel);
		return;
            }
	case LOGICAL_OR:
            {
		auto chkRightLabel = gen::getLabel("chkRight");
		
		left->condJmp(trueLabel, chkRightLabel);
		gen::labelDef(chkRightLabel);
		right->condJmp(trueLabel, falseLabel);
		return;
            }
	default:
	    {
		auto zero = gen::loadZero(type);
		auto cond = gen::cond(gen::NE, loadValue(), zero);
		gen::jmp(cond, trueLabel, falseLabel);
		return;
	    }
    }
    */
}

// for debugging and educational purposes

void
BinaryExpr::print(int indent) const
{
    if (indent) {
	std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    }
    std::cerr << kindStr(kind) << " [ " << type << " ] " << std::endl;
    left->print(indent + 4);
    right->print(indent + 4);
}

void
BinaryExpr::printFlat(std::ostream &out, int prec) const
{
    switch (kind) {
	case ADD:
	    ::printFlat(out, prec, 11, left.get(), right.get(), "+");
	    break;
	case ASSIGN:
	    ::printFlat(out, prec, 2, left.get(), right.get(), "=");
	    break;
	case EQUAL:
	    ::printFlat(out, prec, 9, left.get(), right.get(), "==");
	    break;
	case NOT_EQUAL:
	    ::printFlat(out, prec, 9, left.get(), right.get(), "!=");
	    break;
	case GREATER:
	    ::printFlat(out, prec, 10, left.get(), right.get(), ">");
	    break;
	case GREATER_EQUAL:
	    ::printFlat(out, prec, 10, left.get(), right.get(), ">=");
	    break;
	case LESS:
	    ::printFlat(out, prec, 10, left.get(), right.get(), "<");
	    break;
	case LESS_EQUAL:
	    ::printFlat(out, prec, 10, left.get(), right.get(), "<=");
	    break;
	case LOGICAL_AND:
	    ::printFlat(out, prec, 5, left.get(), right.get(), "&&");
	    break;
	case LOGICAL_OR:
	    ::printFlat(out, prec, 4, left.get(), right.get(), "||");
	    break;
	case SUB:
	    ::printFlat(out, prec, 11, left.get(), right.get(), "-");
	    break;
	case MUL:
	    ::printFlat(out, prec, 13, left.get(), right.get(), "*");
	    break;
	case DIV:
	    ::printFlat(out, prec, 13, left.get(), right.get(), "/");
	    break;
	case MOD:
	    ::printFlat(out, prec, 13, left.get(), right.get(), "%");
	    break;
	default:
	    out << " <binary kind " << kind << ">";
    }
}

} // namespace abc

//------------------------------------------------------------------------------

/*
 * Auxiliary functions
 */

static gen::AluOp
getGenAluOp(abc::BinaryExpr::Kind kind, const abc::Type *type)
{
    assert(type);
    switch (kind) {
	case abc::BinaryExpr::Kind::ADD:
	    return gen::ADD;
	case abc::BinaryExpr::Kind::SUB:
	    return gen::SUB;
	case abc::BinaryExpr::Kind::MUL:
	    return gen::SMUL;
	case abc::BinaryExpr::Kind::DIV:
	    return type->isSignedInteger() ? gen::SDIV : gen::UDIV;
	case abc::BinaryExpr::Kind::MOD:
	    return type->isSignedInteger() ? gen::SMOD : gen::UMOD;
	default:
	    assert(0);
	    return gen::ADD;
    }
}

/*
static gen::CondOp
getGenCondOp(abc::BinaryExpr::Kind kind, const abc::Type *type)
{
    assert(type);
    switch (kind) {
	case abc::BinaryExpr::Kind::EQUAL:
	    return gen::EQ;
	case abc::BinaryExpr::Kind::NOT_EQUAL:
	    return gen::NE;
	case abc::BinaryExpr::Kind::LESS:
	    return type->isSignedInteger() ? gen::SLT : gen::ULT;
	case abc::BinaryExpr::Kind::LESS_EQUAL:
	    return type->isSignedInteger() ? gen::SLE : gen::ULE;
	case abc::BinaryExpr::Kind::GREATER:
	    return isSignedInt ? gen::SGT : gen::UGT;
	case abc::BinaryExpr::Kind::GREATER_EQUAL:
	    return type->isSignedInteger() ? gen::SGE : gen::UGE;
	case abc::BinaryExpr::Kind::LOGICAL_AND:
	    return gen::AND;
	case abc::BinaryExpr::Kind::LOGICAL_OR:
	    return gen::OR;
	default:
	    std::cerr << "Not handled: " << int(kind) << std::endl;
	    assert(0);
	    return gen::EQ;
    }
}
*/

static const char *
kindStr(abc::BinaryExpr::Kind kind)
{
    switch (kind) {
	case abc::BinaryExpr::ADD: return "+";
	case abc::BinaryExpr::ASSIGN: return "=";
	case abc::BinaryExpr::EQUAL: return "==";
	case abc::BinaryExpr::NOT_EQUAL: return "!=";
	case abc::BinaryExpr::GREATER: return ">";
	case abc::BinaryExpr::GREATER_EQUAL: return ">=";
	case abc::BinaryExpr::LESS: return "<";
	case abc::BinaryExpr::LESS_EQUAL: return "<=";
	case abc::BinaryExpr::LOGICAL_AND: return "&&";
	case abc::BinaryExpr::LOGICAL_OR: return "||";
	case abc::BinaryExpr::SUB: return "-";
	case abc::BinaryExpr::MUL: return "*";
	case abc::BinaryExpr::DIV: return "/";
	case abc::BinaryExpr::MOD: return "%";
	default: return "?? (binary)";
    }
}

static void
printFlat(std::ostream &out, int precCaller, int prec, const abc::Expr *left,
	  const abc::Expr *right, const char *op)
{
    if (precCaller > prec) {
	out << "(";
    }
    left->printFlat(out, prec);
    out << " " << op << " ";
    right->printFlat(out, prec);
    if (precCaller > prec) {
	out << ")";
    }
}

