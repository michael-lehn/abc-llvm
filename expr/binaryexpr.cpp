#include <cassert>
#include <iostream>
#include <iomanip>

#include "gen/constant.hpp"
#include "gen/instruction.hpp"
#include "gen/label.hpp"
#include "gen/variable.hpp"
#include "lexer/error.hpp"
#include "type/integertype.hpp"

#include "binaryexpr.hpp"
#include "identifier.hpp"
#include "promotion.hpp"

static const char *kindStr(abc::BinaryExpr::Kind kind);
static gen::InstructionOp getGenInstructionOp(abc::BinaryExpr::Kind kind,
					      const abc::Type *type);

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

void
BinaryExpr::apply(std::function<bool(const Expr *)> op) const
{
    if (op(this)) {
	left->apply(op);
	right->apply(op);
    }
}

bool
BinaryExpr::hasConstantAddress() const
{
    if (kind == INDEX && right->isConst()) {
	if (left->type->isArray() && left->hasConstantAddress()) {
	    return true;
	} else if (left->type->isPointer() && left->isConst()) {
	    return true;
	}
    }
    return false;
}

bool
BinaryExpr::hasAddress() const
{
    return kind == INDEX;
}

bool
BinaryExpr::isLValue() const
{
    return kind == INDEX;
}

bool
BinaryExpr::isConst() const
{
    // special case
    if (kind == SUB && left->type->isPointer() && right->type->isPointer()) {
	auto refType = left->type->refType();
	auto diff = gen::pointerConstantDifference(refType,
						   left->loadValue(),
						   right->loadValue());
	return diff.has_value();
    }

    // remaining cases just depend on whether operands are const
    switch (kind) {
	default:
	    return false;
	case SUB:
	case ADD:
	case LESS:
	case LESS_EQUAL:
	case GREATER:
	case GREATER_EQUAL:
	case NOT_EQUAL:
	case EQUAL:
	case MUL:
	case DIV:
	case MOD:
	case BITWISE_AND:
	case BITWISE_OR:
	case BITWISE_XOR:
	case BITWISE_LEFT_SHIFT:
	case BITWISE_RIGHT_SHIFT:
	case LOGICAL_AND:
	case LOGICAL_OR:
	    return left->isConst() && right->isConst();
    }
}

// for code generation
gen::Constant
BinaryExpr::loadConstant() const
{
    assert(type);
    assert(isConst());
    switch (kind) {
	default:
	    assert(0);
	    return nullptr;
	case ADD:
	    if (type->isPointer()) {
		// pointer + integer
		assert(left->type->isPointer());
		assert(right->type->isInteger());
		auto offset = right->getUnsignedIntValue();
		auto addr =  gen::pointerIncrement(left->type->refType(),
						   left->loadConstant(),
						   offset);
		assert(addr);
		return addr;
	    } else {
		return gen::instruction(getGenInstructionOp(kind, type),
					left->loadConstant(),
					right->loadConstant());
	    }
	case SUB:
	    if (left->type->isPointer() && right->type->isPointer()) {
		// pointer - pointer
		assert(right->type->isPointer());
		assert(Type::equals(left->type->refType(),
				    right->type->refType()));
		auto refType = left->type->refType();
		auto diff =  gen::pointerConstantDifference(refType,
							    left->loadValue(),
							    right->loadValue());
		assert(diff.has_value());
		return diff.value();
	    } else {
		return gen::instruction(getGenInstructionOp(kind, type),
					left->loadConstant(),
					right->loadConstant());
	    }
	case MUL:
	case DIV:
	case MOD:
	case BITWISE_AND:
	case BITWISE_OR:
	case BITWISE_XOR:
	case BITWISE_LEFT_SHIFT:
	case BITWISE_RIGHT_SHIFT:
	    return gen::instruction(getGenInstructionOp(kind, type),
				    left->loadConstant(),
				    right->loadConstant());
	case LESS:
	case LESS_EQUAL:
	case GREATER:
	case GREATER_EQUAL:
	case NOT_EQUAL:
	case EQUAL:
	    if (left->type->isPointer()) {
		// pointer - pointer
		assert(right->type->isPointer());
		assert(Type::equals(left->type->refType(),
				    right->type->refType()));
		auto refType = left->type->refType();
		auto diff =  gen::pointerConstantDifference(refType,
							    left->loadValue(),
							    right->loadValue());
		assert(diff.has_value());
		auto ptrdiffType = IntegerType::createPtrdiffType();
		return gen::instruction(getGenInstructionOp(kind, left->type),
					diff.value(),
					gen::getConstantZero(ptrdiffType));
	    } else {
		return gen::instruction(getGenInstructionOp(kind, left->type),
					left->loadConstant(),
					right->loadConstant());
	    }
	case LOGICAL_AND:
	    if (left->loadConstant()->isZeroValue()) {
		return gen::getFalse();
	    }
	    if (right->loadConstant()->isZeroValue()) {
		return gen::getFalse();
	    }
	    return gen::getTrue();
	case LOGICAL_OR:
	    if (!left->loadConstant()->isZeroValue()) {
		return gen::getTrue();
	    }
	    if (!right->loadConstant()->isZeroValue()) {
		return gen::getTrue();
	    }
	    return gen::getFalse();
    }
}

gen::Value
BinaryExpr::handleArithmetricOperation(Kind kind) const
{
    assert(type);
    switch (kind) {
	default:
	    assert(0);
	    return nullptr;
	case ADD:
	    if (type->isPointer()) {
		// pointer + integer
		assert(left->type->isPointer());
		assert(right->type->isInteger());
		return gen::pointerIncrement(left->type->refType(),
					     left->loadValue(),
					     right->loadValue());
	    } else {
		return gen::instruction(getGenInstructionOp(kind, type),
					left->loadValue(),
					right->loadValue());
	    }
	case SUB:
	    if (kind == SUB && left->type->isPointer()) {
		// pointer - pointer
		assert(right->type->isPointer());
		assert(type->isInteger());
		return gen::pointerDifference(left->type->refType(),
					      left->loadValue(),
					      right->loadValue());
	    } else {
		return gen::instruction(getGenInstructionOp(kind, type),
					left->loadValue(),
					right->loadValue());
	    }
	case MUL:
	case DIV:
	case MOD:
	case BITWISE_AND:
	case BITWISE_OR:
	case BITWISE_XOR:
	case BITWISE_LEFT_SHIFT:
	case BITWISE_RIGHT_SHIFT:
	    return gen::instruction(getGenInstructionOp(kind, type),
				    left->loadValue(),
				    right->loadValue());
    }
}

gen::Value
BinaryExpr::loadValue() const
{
    if (isConst()) {
	return loadConstant();
    }
    assert(type);
    switch (kind) {
	case ASSIGN:
	    return gen::store(right->loadValue(),
			      left->loadAddress());
	case ADD_ASSIGN:
	    return gen::store(handleArithmetricOperation(ADD),
			      left->loadAddress());
	case SUB_ASSIGN:
	    return gen::store(handleArithmetricOperation(SUB),
			      left->loadAddress());
	case MUL_ASSIGN:
	    return gen::store(handleArithmetricOperation(MUL),
			      left->loadAddress());
	case DIV_ASSIGN:
	    return gen::store(handleArithmetricOperation(DIV),
			      left->loadAddress());
	case MOD_ASSIGN:
	    return gen::store(handleArithmetricOperation(MOD),
			      left->loadAddress());
	case BITWISE_AND_ASSIGN:
	    return gen::store(handleArithmetricOperation(BITWISE_AND),
			      left->loadAddress());
	case BITWISE_OR_ASSIGN:
	    return gen::store(handleArithmetricOperation(BITWISE_OR),
			      left->loadAddress());
	case BITWISE_XOR_ASSIGN:
	    return gen::store(handleArithmetricOperation(BITWISE_XOR),
			      left->loadAddress());
	case BITWISE_LEFT_SHIFT_ASSIGN:
	    return gen::store(handleArithmetricOperation(BITWISE_LEFT_SHIFT),
			      left->loadAddress());
	case BITWISE_RIGHT_SHIFT_ASSIGN:
	    return gen::store(handleArithmetricOperation(BITWISE_RIGHT_SHIFT),
			      left->loadAddress());
	case ADD:
	case SUB:
	case MUL:
	case DIV:
	case MOD:
	case BITWISE_AND:
	case BITWISE_OR:
	case BITWISE_XOR:
	case BITWISE_LEFT_SHIFT:
	case BITWISE_RIGHT_SHIFT:
	    return handleArithmetricOperation(kind);
	case LESS:
	case LESS_EQUAL:
	case GREATER:
	case GREATER_EQUAL:
	case NOT_EQUAL:
	case EQUAL:
	    return gen::instruction(getGenInstructionOp(kind, left->type),
				    left->loadValue(),
				    right->loadValue());
	case LOGICAL_AND:
	case LOGICAL_OR:
	    {
		auto trueLabel = gen::getLabel("true");
		auto falseLabel = gen::getLabel("false");
		auto phiLabel = gen::getLabel("true");

		condition(trueLabel, falseLabel);

		gen::defineLabel(trueLabel);
		trueLabel = gen::jumpInstruction(phiLabel);

		gen::defineLabel(falseLabel);
		falseLabel = gen::jumpInstruction(phiLabel);

		gen::defineLabel(phiLabel);
		return gen::phi(gen::getTrue(), trueLabel,
				gen::getFalse(), falseLabel,
				IntegerType::createBool());
	    }
	case INDEX:
	    return gen::fetch(loadAddress(), left->type->refType());
	    
	default:
	    error::out() << "kind = " << int(kind) << std::endl;
	    assert(0);
	    return nullptr;
    }
}

gen::Constant
BinaryExpr::loadConstantAddress() const
{
    assert(hasConstantAddress());
    assert(kind == INDEX);
    if (left->type->isArray()) {
	auto offset = right->getUnsignedIntValue();
	return gen::pointerIncrement(left->type->refType(),
				     left->loadConstantAddress(),
				     offset);
    } else {
	auto offset = right->getUnsignedIntValue();
	return gen::pointerIncrement(left->type->refType(),
				     left->loadConstant(),
				     offset);
    }
}

gen::Value
BinaryExpr::loadAddress() const
{
    assert(hasAddress());
    assert(kind == INDEX);
    if (left->type->isArray()) {
	return gen::pointerIncrement(left->type->refType(),
				     left->loadAddress(),
				     right->loadValue());
    } else {
	return gen::pointerIncrement(left->type->refType(),
				     left->loadValue(),
				     right->loadValue());
    }
}

void
BinaryExpr::condition(gen::Label trueLabel, gen::Label falseLabel) const
{
    switch (kind) {
	case LESS:
	case LESS_EQUAL:
	case GREATER:
	case GREATER_EQUAL:
	case NOT_EQUAL:
	case EQUAL:
	    {
		auto op = getGenInstructionOp(kind, left->type);
		auto cond = gen::instruction(op,
					     left->loadValue(),
					     right->loadValue());
		gen::jumpInstruction(cond, trueLabel, falseLabel);
		return;
	    }
	case LOGICAL_AND:
            {
		auto chkRightLabel = gen::getLabel("chkRight");

		left->condition(chkRightLabel, falseLabel);
		gen::defineLabel(chkRightLabel);
		right->condition(trueLabel, falseLabel);
		return;
            }
	case LOGICAL_OR:
            {
		auto chkRightLabel = gen::getLabel("chkRight");
		
		left->condition(trueLabel, chkRightLabel);
		gen::defineLabel(chkRightLabel);
		right->condition(trueLabel, falseLabel);
		return;
            }
	default:
	    Expr::condition(trueLabel, falseLabel);
    }
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
	case INDEX:
	    left->printFlat(out, 16);
	    out << "[";
	    right->printFlat(out, 16);
	    out << "]";
	    break;
	case BITWISE_AND:
	    ::printFlat(out, prec, 8, left.get(), right.get(), "&");
	    break;
	case BITWISE_OR:
	    ::printFlat(out, prec, 6, left.get(), right.get(), "|");
	    break;
	case BITWISE_XOR:
	    ::printFlat(out, prec, 7, left.get(), right.get(), "^");
	    break;
	case BITWISE_LEFT_SHIFT:
	    ::printFlat(out, prec, 11, left.get(), right.get(), "<<");
	    break;
	case BITWISE_RIGHT_SHIFT:
	    ::printFlat(out, prec, 11, left.get(), right.get(), ">>");
	    break;
	case ADD:
	    ::printFlat(out, prec, 12, left.get(), right.get(), "+");
	    break;
	case ASSIGN:
	    ::printFlat(out, prec, 2, left.get(), right.get(), "=");
	    break;
	case ADD_ASSIGN:
	    ::printFlat(out, prec, 2, left.get(), right.get(), "+=");
	    break;
	case SUB_ASSIGN:
	    ::printFlat(out, prec, 2, left.get(), right.get(), "-=");
	    break;
	case MUL_ASSIGN:
	    ::printFlat(out, prec, 2, left.get(), right.get(), "*=");
	    break;
	case DIV_ASSIGN:
	    ::printFlat(out, prec, 2, left.get(), right.get(), "/=");
	    break;
	case MOD_ASSIGN:
	    ::printFlat(out, prec, 2, left.get(), right.get(), "%=");
	    break;
	case BITWISE_AND_ASSIGN:
	    ::printFlat(out, prec, 2, left.get(), right.get(), "&=");
	    break;
	case BITWISE_OR_ASSIGN:
	    ::printFlat(out, prec, 2, left.get(), right.get(), "|=");
	    break;
	case BITWISE_XOR_ASSIGN:
	    ::printFlat(out, prec, 2, left.get(), right.get(), "^=");
	    break;
	case BITWISE_LEFT_SHIFT_ASSIGN:
	    ::printFlat(out, prec, 2, left.get(), right.get(), "<<=");
	    break;
	case BITWISE_RIGHT_SHIFT_ASSIGN:
	    ::printFlat(out, prec, 2, left.get(), right.get(), ">>=");
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
	    ::printFlat(out, prec, 12, left.get(), right.get(), "-");
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
static gen::InstructionOp
getGenInstructionOp(abc::BinaryExpr::Kind kind, const abc::Type *type)
{
    assert(type);
    switch (kind) {
	case abc::BinaryExpr::Kind::ADD:
	    return type->isFloatType() ? gen::FADD : gen::ADD;
	case abc::BinaryExpr::Kind::SUB:
	    return type->isFloatType() ? gen::FSUB : gen::SUB;
	case abc::BinaryExpr::Kind::MUL:
	    return type->isFloatType() ? gen::FMUL : gen::SMUL;
	case abc::BinaryExpr::Kind::DIV:
	    if (type->isFloatType()) {
		return gen::FDIV;
	    } else {
		return type->isSignedInteger() ? gen::SDIV : gen::UDIV;
	    }
	case abc::BinaryExpr::Kind::MOD:
	    return type->isSignedInteger() ? gen::SMOD : gen::UMOD;
	case abc::BinaryExpr::Kind::BITWISE_AND:
	    return gen::AND;
	case abc::BinaryExpr::Kind::BITWISE_OR:
	    return gen::OR;
	case abc::BinaryExpr::Kind::BITWISE_XOR:
	    return gen::XOR;
	case abc::BinaryExpr::Kind::BITWISE_LEFT_SHIFT:
	    return gen::SHL;
	case abc::BinaryExpr::Kind::BITWISE_RIGHT_SHIFT:
	    return type->isSignedInteger() ? gen::ASHR : gen::LSHR;
	case abc::BinaryExpr::Kind::EQUAL:
	    return type->isFloatType() ? gen::FEQ : gen::EQ;
	case abc::BinaryExpr::Kind::NOT_EQUAL:
	    return type->isFloatType() ? gen::FNE : gen::NE;
	case abc::BinaryExpr::Kind::LESS:
	    if (type->isFloatType()) {
		return gen::FLT;
	    } else {
		return type->isSignedInteger() ? gen::SLT : gen::ULT;
	    }
	case abc::BinaryExpr::Kind::LESS_EQUAL:
	    if (type->isFloatType()) {
		return gen::FLE;
	    } else {
		return type->isSignedInteger() ? gen::SLE : gen::ULE;
	    }
	case abc::BinaryExpr::Kind::GREATER:
	    if (type->isFloatType()) {
		return gen::FGT;
	    } else {
		return type->isSignedInteger() ? gen::SGT : gen::UGT;
	    }
	case abc::BinaryExpr::Kind::GREATER_EQUAL:
	    if (type->isFloatType()) {
		return gen::FGE;
	    } else {
		return type->isSignedInteger() ? gen::SGE : gen::UGE;
	    }
	case abc::BinaryExpr::Kind::LOGICAL_AND:
	    return gen::AND;
	case abc::BinaryExpr::Kind::LOGICAL_OR:
	    return gen::OR;
	default:
	    assert(0);
	    return gen::ADD;
    }
}

static const char *
kindStr(abc::BinaryExpr::Kind kind)
{
    switch (kind) {
	case abc::BinaryExpr::ADD: return "+";
	case abc::BinaryExpr::ASSIGN: return "=";
	case abc::BinaryExpr::ADD_ASSIGN: return "+=";
	case abc::BinaryExpr::SUB_ASSIGN: return "-=";
	case abc::BinaryExpr::MUL_ASSIGN: return "*=";
	case abc::BinaryExpr::DIV_ASSIGN: return "/=";
	case abc::BinaryExpr::MOD_ASSIGN: return "%=";
	case abc::BinaryExpr::BITWISE_AND_ASSIGN: return "&=";
	case abc::BinaryExpr::BITWISE_OR_ASSIGN: return "|=";
	case abc::BinaryExpr::BITWISE_XOR_ASSIGN: return "^=";
	case abc::BinaryExpr::BITWISE_LEFT_SHIFT_ASSIGN: return "<<=";
	case abc::BinaryExpr::BITWISE_RIGHT_SHIFT_ASSIGN: return ">>=";
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
	case abc::BinaryExpr::BITWISE_AND: return "&";
	case abc::BinaryExpr::BITWISE_OR: return "|";
	case abc::BinaryExpr::BITWISE_XOR: return "^";
	case abc::BinaryExpr::BITWISE_LEFT_SHIFT: return "<<";
	case abc::BinaryExpr::BITWISE_RIGHT_SHIFT: return ">>";
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

