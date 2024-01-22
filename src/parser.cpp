#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include "error.hpp"
#include "gen.hpp"
#include "lexer.hpp"
#include "parseexpr.hpp"
#include "parser.hpp"
#include "symtab.hpp"
#include "ustr.hpp"

struct ControlStruct
{
    static std::vector<gen::Label> continueLabel;
    static std::vector<gen::Label> breakLabel;
};

std::vector<gen::Label> ControlStruct::continueLabel;
std::vector<gen::Label> ControlStruct::breakLabel;

//------------------------------------------------------------------------------

static bool parseFn(void);
static bool parseGlobalDef(void);
static bool parseStructDef(void);
static bool parseTypeDef(void);

void
parser(void)
{
    getToken();
    while (token.kind != TokenKind::EOI) {
	if (!parseGlobalDef() && !parseFn() && !parseTypeDef()
		&& !parseStructDef()) {
	    error::out() << token.loc
		<< ": expected function declaration,"
		<< " global variable definition,"
		<< " type definition"
		<< " or EOF"
		<< std::endl;
	    error::fatal();
	}
    }
}

//------------------------------------------------------------------------------

static bool
parseTypeDef(void)
{
    if (token.kind != TokenKind::TYPE) {
	return false;
    }
    getToken();

    while (token.kind == TokenKind::IDENTIFIER) {
	const char *ident = token.val.c_str();
	getToken();
	error::expected(TokenKind::EQUAL);
	getToken();
	auto ty = parseType();
	if (!ty) {
	    error::out() << token.loc << ": type expected" << std::endl;
	    error::fatal();
	}
	if (!Symtab::addTypeAlias(ident, ty)) {
	    error::out() << token.loc << ": '" << ident << "' already defined "
		<< std::endl;
	    error::fatal();
	}
	if (token.kind != TokenKind::COMMA) {
	    break;
	}
	getToken();
    }
    error::expected(TokenKind::SEMICOLON);
    getToken();
    return true;
}

static const Type *parseStructDecl(void);

static bool
parseStructDef(void)
{
    if (token.kind != TokenKind::STRUCT) {
	return false;
    }
    auto structTok = token;
    auto ty = parseStructDecl();
    if (!ty) {
	error::out() << structTok.loc
	    << ": struct definition expected" << std::endl;
	error::fatal();
    }
    error::expected(TokenKind::SEMICOLON);
    getToken();
    return true;
}

static bool
parseGlobalDef(void)
{
    if (token.kind != TokenKind::GLOBAL) {
	return false;
    }
    getToken();

    do {
	error::expected(TokenKind::IDENTIFIER);
	auto loc = token.loc;
	auto ident = token.val;
	getToken();

	error::expected(TokenKind::COLON);
	getToken();

	auto type = parseType();
	if (!type) {
	    error::out() << token.loc << " type expected" << std::endl;
	    error::fatal();
	} else if (!type->hasSize()) {
	    error::out() << token.loc
		<< ": variable '" << ident.c_str() << "' has incomplete type '"
		<< type << "'" << std::endl;
	    error::fatal();
	}

	auto s = Symtab::addDeclToRootScope(loc, ident.c_str(), type);
	auto ty = s->getType();

	// parse initalizer
	ExprPtr init = nullptr;
	if (token.kind == TokenKind::EQUAL) {
	    auto opLoc = token.loc;
	    getToken();
	    init = parseConstExpr();
	    if (!init) {
		error::out() << token.loc
		    << " expected non-empty constant expression" << std::endl;
		error::fatal();
	    }
	    auto toTy = Type::getTypeConversion(init->getType(), ty, opLoc);
	    if (toTy != ty) {
		init->print();
		error::out() << opLoc << " can not cast expression of type '"
		    << init->getType() << "' to type '" << ty
		    << "'" <<std::endl;
		error::fatal();
	    } else if (toTy != init->getType()) {
	    	init = Expr::createCast(std::move(init), toTy, loc);
	    }
	}
	auto initValue = init ? init->loadConst() : nullptr;
	gen::defGlobal(s->ident.c_str(), ty, initValue);
	if (token.kind != TokenKind::COMMA) {
	    break;
	}
	error::expected(TokenKind::COMMA);
	getToken();
    } while (true);

    error::expected(TokenKind::SEMICOLON);
    getToken();
    
    return true;
}

//------------------------------------------------------------------------------

static void
parseFnParamDeclOrType(std::vector<const Type *> &argType,
		       std::vector<const char *> *paramIdent = nullptr)
{
    static std::size_t paramId;
    paramId = 0;

    argType.clear();
    if (paramIdent) {
	paramIdent->clear();
    }

    while (token.kind == TokenKind::IDENTIFIER
        || token.kind == TokenKind::COLON)
    {
	// if parameter has no identifier give it an interal identifier 
	UStr ident;
	auto loc = token.loc;
	if (token.kind == TokenKind::IDENTIFIER) {
	    ident = token.val.c_str();
	    getToken();
	} else {
	    std::stringstream ss;
	    ss << ".param" << paramId++;
	    ident = UStr{ss.str()};
	}

	error::expected(TokenKind::COLON);
	getToken();

	// parse param type
	auto type = parseType();
	if (!type) {
	    error::out() << token.loc << " type expected"
		<< std::endl;
	    error::fatal();
	}
	argType.push_back(type);

	// add param to symtab if this is a declaration
	if (paramIdent) {
	    auto s = Symtab::addDecl(loc, ident.c_str(), type);
	    paramIdent->push_back(s->ident.c_str());
	}

	// done if we don't get a COMMA
	if (token.kind != TokenKind::COMMA) {
	    break;
	}
	getToken();
    }
}

static void
parseFnParamDecl(std::vector<const Type *> &argType,
		 std::vector<const char *> &paramIdent)
{
    parseFnParamDeclOrType(argType, &paramIdent);
}

static void
parseFnParamType(std::vector<const Type *> &argType)
{
    parseFnParamDeclOrType(argType);
}

//------------------------------------------------------------------------------

static const Type *
parseFnDeclOrType(std::vector<const Type *> &argType, const Type *&retType,
	          Symtab::Entry **fnDecl = nullptr,
		  std::vector<const char *> *fnParamIdent = nullptr)
{
    UStr fnIdent;
    Token::Loc fnLoc;

    if (token.kind != TokenKind::FN) {
	return nullptr;
    }
    getToken();

    // parse function identifier
    if (fnDecl) {
	error::expected(TokenKind::IDENTIFIER);
    }
    if (token.kind == TokenKind::IDENTIFIER) {
	fnLoc = token.loc;
	fnIdent = token.val;
	getToken();
    }

    // parse function parameters
    error::expected(TokenKind::LPAREN);
    getToken();
    if (fnDecl) {
	parseFnParamDecl(argType, *fnParamIdent);
    } else {
	parseFnParamType(argType);
    }
    error::expected(TokenKind::RPAREN);
    getToken();

    // parse function return type
    retType = Type::getVoid();
    auto retTypeLoc = token.loc;
    if (token.kind == TokenKind::COLON) {
	getToken();
	retTypeLoc = token.loc;
	retType = parseType();
	if (!retType) {
	    error::out() << retTypeLoc << ": return type expected" << std::endl;
	    error::fatal();
	}
    } else if (fnIdent == UStr{"main"}) {
	retType = Type::getUnsignedInteger(8);
    }
    if (!retType->isVoid()) {
	Symtab::addDecl(retTypeLoc, UStr{".retVal"}, retType);
    }

    auto fnType = Type::getFunction(retType, argType);
    
    if (fnDecl) {
	*fnDecl = Symtab::addDeclToRootScope(fnLoc, fnIdent.c_str(), fnType);
    }

    return fnType;
}

static Symtab::Entry *
parseFnDecl(std::vector<const char *> &fnParamIdent)
{
    std::vector<const Type *> argType;
    const Type *retType;
    Symtab::Entry *fnDecl;

    parseFnDeclOrType(argType, retType, &fnDecl, &fnParamIdent);
    return fnDecl;
}

static const Type *
parseFnType(void)
{
    std::vector<const Type *> argType;
    const Type *retType;

    return parseFnDeclOrType(argType, retType);
}

//------------------------------------------------------------------------------

const Type *
parseIntType(void)
{
    switch (token.kind) {
	case TokenKind::U8:
	    getToken();
	    return Type::getUnsignedInteger(8);
	case TokenKind::U16:
	    getToken();
	    return Type::getUnsignedInteger(16);
	case TokenKind::U32:
	    getToken();
	    return Type::getUnsignedInteger(32);
	case TokenKind::U64:
	    getToken();
	    return Type::getUnsignedInteger(64);
	case TokenKind::I8:
	    getToken();
	    return Type::getSignedInteger(8);
	case TokenKind::I16:
	    getToken();
	    return Type::getSignedInteger(16);
	case TokenKind::I32:
	    getToken();
	    return Type::getSignedInteger(32);
	case TokenKind::I64:
	    getToken();
	    return Type::getSignedInteger(64);
	default:
	    return nullptr;
    }
}

static const Type *
parsePtrType(void)
{
    if (token.kind != TokenKind::ARROW) {
	return nullptr;
    }
    getToken();
    auto baseTy = parseType();
    if (!baseTy) {
	baseTy = Type::getVoid();
    }
    return Type::getPointer(baseTy);
}

static const Type *
parseArrayDimAndType(void)
{
    if (token.kind != TokenKind::LBRACKET) {
	return nullptr;
    }
    getToken();
    auto dim = parseExpr();
    if (!dim || !dim->isConst() || !dim->getType()->isInteger()) {
	error::out() << token.loc
	    << " dimension has to be a constant integer expression"
	    << std::endl;
	error::fatal();
    }
    auto dimVal = llvm::dyn_cast<llvm::ConstantInt>(dim->loadConst());

    if (dimVal->isNegative()) {
	error::out() << token.loc
	    << " dimension can not be negative"
	    << std::endl;
	error::fatal();
    }

    error::expected(TokenKind::RBRACKET);
    getToken();

    const Type *ty = nullptr;
    if (token.kind == TokenKind::OF) {
	getToken();
	ty = parseType();
    } else {
	ty = parseArrayDimAndType();
    }
    if (!ty) {
	error::out() << token.loc << " expected element type"
	    << std::endl;
	error::fatal();
    }
    return Type::getArray(ty, dimVal->getZExtValue());
}

static const Type *
parseArrayType(void)
{
    if (token.kind != TokenKind::ARRAY) {
	return nullptr;
    }
    getToken();
    return parseArrayDimAndType();
}

static const Type *
parseStructDecl(void)
{
    if (token.kind != TokenKind::STRUCT) {
	return nullptr;
    }
    getToken();

    error::expected(TokenKind::IDENTIFIER);
    auto structIdTok = token;
    getToken();

    auto structTy = Type::createIncompleteStruct(structIdTok.val);
    if (token.kind != TokenKind::LBRACE) {
	return structTy;
    }
    getToken();

    if (!structTy) {
	error::out() << structIdTok.loc << ": struct '"
	    << structIdTok.val.c_str() << "' already defined in this scope"
	    << std::endl;
	error::fatal();
    }

    std::vector<const char *> ident;
    std::vector<const Type *> type;
    while (token.kind == TokenKind::IDENTIFIER) {
	ident.push_back(token.val.c_str());
	getToken();
	error::expected(TokenKind::COLON);
	getToken();
	auto ty = parseType();
	if (!ty) {
	    error::out() << token.loc << ": type expected" << std::endl;
	    error::fatal();
	}
	type.push_back(ty);
	error::expected(TokenKind::SEMICOLON);
	getToken();
    }

    error::expected(TokenKind::RBRACE);
    getToken();
    return structTy->complete(std::move(ident), std::move(type));
}

static const Type *
parseNamedType(const char *name = nullptr)
{
    if (token.kind != TokenKind::IDENTIFIER) {
	return nullptr;
    }
    auto ty = Symtab::getNamedType(token.val, Symtab::AnyScope);
    if (!ty) {
	error::out()
	    << token.loc << ": type expected. '" << token.val.c_str()
	    << "' is not a type." << std::endl;
	error::fatal();
    }
    getToken();
    return ty;
}

const Type *
parseType(void)
{
    if (auto ty = parseFnType()) {
	return ty;
    } else if (auto ty = parsePtrType()) {
	return ty;
    } else if (auto ty = parseArrayType()) {
	return ty;
    } else if (auto ty = parseNamedType()) {
	return ty;
    }
    return parseIntType();
}

//------------------------------------------------------------------------------

static bool
parseLocalDef(void)
{
    if (token.kind != TokenKind::LOCAL) {
	return false;
    }
    getToken();

    do {
	error::expected(TokenKind::IDENTIFIER);
	auto loc = token.loc;
	auto ident = token.val;
	getToken();

	error::expected(TokenKind::COLON);
	getToken();

	auto type = parseType();
	if (!type) {
	    error::out() << token.loc << " type expected"
		<< std::endl;
	    error::fatal();
	} else if (!type->hasSize()) {
	    error::out() << token.loc
		<< ": variable '" << ident.c_str() << "' has incomplete type '"
		<< type << "'" << std::endl;
	    error::fatal();
	} else if (type->isFunction()) {
	    error::out() << loc
		<< ": Function can not be defined as local variable. "
		<< std::endl
		<< "\tIf '" << ident.c_str()
		<< "' is supposed to be a function pointer use type '"
		<< Type::getPointer(type)
		<< "'" << std::endl;
	    error::fatal();
	}

	auto s = Symtab::addDecl(loc, ident.c_str(), type);
	gen::defLocal(s->ident.c_str(), s->getType());

	// parse initalizer
	if (token.kind == TokenKind::EQUAL) {
	    auto opLoc = token.loc;
	    getToken();
	    auto init = parseExpr();
	    if (!init) {
		error::out() << token.loc << " expected non-empty expression"
		    << std::endl;
		error::fatal();
	    }

	    auto var = Expr::createIdentifier(ident.c_str(), loc);
	    init = Expr::createBinary(Binary::Kind::ASSIGN,
				      std::move(var),
				      std::move(init),
				      opLoc);
	    init->loadValue();
	}
	if (token.kind != TokenKind::COMMA) {
	    return true;
	}
	error::expected(TokenKind::COMMA);
	getToken();
    } while (true);
}

static bool
parseLocalDefStmt(void)
{
    if (parseLocalDef()) {
	error::expected(TokenKind::SEMICOLON);
	getToken();
	return true;
    }
    return false;
}

//------------------------------------------------------------------------------

static bool parseStmt(void);

static bool
parseCompoundStmt(bool openScope)
{
    if (token.kind != TokenKind::LBRACE) {
	return false;
    }
    getToken();

    if (openScope) {
	Symtab::openScope();
    }

    while (parseStmt()) {
    }

    error::expected(TokenKind::RBRACE);
    getToken();
    if (openScope) {
	Symtab::closeScope();
    }
    return true;
}

static void
reachableCheck(Token tok = token, const char *s = nullptr)
{
    if (!gen::openBuildingBlock()) {
	error::out() << tok.loc << ": warning: "
	    << (s ? "" : "'")
	    << (s ? s : tok.val.c_str())
	    << (s ? "" : "'")
	    << " not reachable" << std::endl;
    }
}

static bool
parseIfStmt(void)
{
    if (token.kind != TokenKind::IF) {
	return false;
    }
    reachableCheck();
    getToken();

    // parse expr
    error::expected(TokenKind::LPAREN);
    getToken();
    auto expr = parseExpr();
    if (!expr) {
	error::out() << token.loc << " expected non-empty expression"
	    << std::endl;
	error::fatal();
    }
    error::expected(TokenKind::RPAREN);
    getToken();

    auto thenLabel = gen::getLabel("then");
    auto elseLabel = gen::getLabel("else");
    auto endLabel = gen::getLabel("end");

    expr->condJmp(thenLabel, elseLabel);
    
    // parse 'then' block
    gen::labelDef(thenLabel);
    if (!parseCompoundStmt(true)) {
	error::out() << token.loc << " expected compound statement block"
	    << std::endl;
	error::fatal();
    }

    gen::jmp(endLabel);

    // parse optional 'else' block
    gen::labelDef(elseLabel);
    if (token.kind == TokenKind::ELSE) {
	getToken();
	if (!parseCompoundStmt(true) && !parseIfStmt()) {
	    error::out() << token.loc
		<< " compound statement block or if statement" << std::endl;
	    error::fatal();
	}
    }
    gen::jmp(endLabel); // connect with 'end' (even if 'else' is empyt)

    // end of 'then' and 'else' block
    gen::labelDef(endLabel);
    return true;
}

static bool
parseWhileStmt(void)
{
    if (token.kind != TokenKind::WHILE) {
	return false;
    }
    reachableCheck();
    getToken();

    // parse expr
    error::expected(TokenKind::LPAREN);
    getToken();
    auto expr = parseExpr();
    if (!expr) {
	error::out() << token.loc << " expected non-empty expression"
	    << std::endl;
	error::fatal();
    }
    error::expected(TokenKind::RPAREN);
    getToken();

    auto condLabel = gen::getLabel("cond");
    auto loopLabel = gen::getLabel("loop");
    auto endLabel = gen::getLabel("end");

    ControlStruct::continueLabel.push_back(condLabel);
    ControlStruct::breakLabel.push_back(endLabel);

    gen::jmp(condLabel);

    // 'while-cond' block
    gen::labelDef(condLabel);
    expr->condJmp(loopLabel, endLabel);
    
    // 'while-loop' block
    gen::labelDef(loopLabel);
    if (!parseCompoundStmt(true)) {
	error::out() << token.loc << " expected compound statement block"
	    << std::endl;
	error::fatal();
    }
    gen::jmp(condLabel);

    // end of loop
    gen::labelDef(endLabel);
    ControlStruct::continueLabel.pop_back();
    ControlStruct::breakLabel.pop_back();
    return true;
}

static bool
parseForStmt(void)
{
    if (token.kind != TokenKind::FOR) {
	return false;
    }
    reachableCheck();
    getToken();

    Symtab::openScope();
    error::expected(TokenKind::LPAREN);
    getToken();
    // parse 'init': local definition or  expr
    if (!parseLocalDef()) {
	auto init = parseExpr();
	if (init) {
	    init->loadValue();
	}
    }
    error::expected(TokenKind::SEMICOLON);
    getToken();
    // parse 'cond' expr
    auto cond = parseExpr();
    if (!cond) {
	cond = Expr::createLiteral("1", 10, nullptr);
    }
    error::expected(TokenKind::SEMICOLON);
    getToken();
    // parse 'update' expr
    auto update = parseExpr();
    error::expected(TokenKind::RPAREN);
    getToken();


    auto condLabel = gen::getLabel("cond");
    auto loopLabel = gen::getLabel("loop");
    auto endLabel = gen::getLabel("end");

    ControlStruct::continueLabel.push_back(condLabel);
    ControlStruct::breakLabel.push_back(endLabel);

    gen::jmp(condLabel);

    // 'for-cond' block
    gen::labelDef(condLabel);
    cond->condJmp(loopLabel, endLabel);
    
    // 'for-loop' block
    gen::labelDef(loopLabel);
    if (!parseCompoundStmt(false)) {
	error::out() << token.loc << " expected compound statement block"
	    << std::endl;
	error::fatal();
    }
    if (update) {
	update->loadValue();
    }
    gen::jmp(condLabel);

    // end of loop
    Symtab::closeScope();
    gen::labelDef(endLabel);
    ControlStruct::continueLabel.pop_back();
    ControlStruct::breakLabel.pop_back();
    return true;
}


static bool
parseReturnStmt(void)
{
    if (token.kind != TokenKind::RETURN) {
	return false;
    }
    reachableCheck();
    getToken();
    auto exprTok = token;
    auto expr = parseExpr();
    error::expected(TokenKind::SEMICOLON);
    getToken();
    auto ret = Expr::createIdentifier(UStr{".retVal"}, exprTok.loc);
    expr = Expr::createBinary(Binary::Kind::ASSIGN, std::move(ret),
			      std::move(expr), exprTok.loc);
    gen::ret(expr->loadValue());
    return true;
}

static bool
parseBreakStmt(void)
{
    if (token.kind != TokenKind::BREAK) {
	return false;
    }
    reachableCheck();
    auto breakTok = token;
    getToken();
    error::expected(TokenKind::SEMICOLON);
    getToken();
    if (ControlStruct::breakLabel.empty()) {
	error::out() << breakTok.loc
	    << ": 'break' statement not in loop or switch statement" <<
	    std::endl;
	error::fatal();
    }
    gen::jmp(ControlStruct::breakLabel.back());
    return true;
}

static bool
parseContinueStmt(void)
{
    if (token.kind != TokenKind::CONTINUE) {
	return false;
    }
    reachableCheck();
    auto contTok = token;
    getToken();
    error::expected(TokenKind::SEMICOLON);
    getToken();
    if (ControlStruct::continueLabel.empty()) {
	error::out() << contTok.loc
	    << ": 'break' statement not in loop or switch statement" <<
	    std::endl;
	error::fatal();
    }
    gen::jmp(ControlStruct::continueLabel.back());
    return true;
}

static bool
parseExprStmt(void)
{
    auto exprTok = token;
    auto expr = parseExpr();
    if (token.kind != TokenKind::SEMICOLON) {
	return false;
    }
    reachableCheck(exprTok, "expression");
    getToken();
    if (expr) {
	expr->loadValue();
    }
    return true;
}

static bool
parseSwitchStmt(void)
{
    if (token.kind != TokenKind::SWITCH) {
	return false;
    }
    reachableCheck();
    getToken();

    error::expected(TokenKind::LPAREN);
    getToken();
    auto cond = parseExpr();
    error::expected(TokenKind::RPAREN);
    getToken();

    error::expected(TokenKind::LBRACE);
    getToken();

    auto switchLabel = gen::getLabel("switch");
    auto defaultLabel = gen::getLabel("default");
    auto endLabel = gen::getLabel("end");
    std::vector<std::pair<gen::ConstIntVal, gen::Label>> caseLabel;
    std::set<std::uint64_t> usedCaseVal;

    ControlStruct::breakLabel.push_back(endLabel);

    gen::jmp(switchLabel);
    bool hasDefault = false;
    while (token.kind != TokenKind::RBRACE) {
	if (token.kind == TokenKind::CASE) {
	    getToken();
	    auto valTok = token;
	    auto val = parseExpr();
	    if (!val || !val->isConst() || !val->getType()->isInteger()) {
		error::out() << valTok.loc
		    << ": constant integer expression required" << std::endl;
		error::fatal();
	    }
	    error::expected(TokenKind::COLON);
	    getToken();
	    auto label = gen::getLabel("case");
	    caseLabel.push_back({val->loadConstInt(), label});
	    if (usedCaseVal.contains(val->loadConstInt()->getZExtValue())) {
		error::out() << valTok.loc << ": duplicate case value '"
		    << val->loadConstInt()->getZExtValue() << "'" << std::endl;
		error::fatal();
	    } else {
		gen::labelDef(label);
	    }
	    usedCaseVal.insert(val->loadConstInt()->getZExtValue());
	} else if (token.kind == TokenKind::DEFAULT) {
	    getToken();
	    error::expected(TokenKind::COLON);
	    getToken();
	    hasDefault = true;
	    gen::labelDef(defaultLabel);
	} else if (parseStmt()) {
	    continue;
	} else {
	    error::out() << token.loc
		<< ": expected expression" << std::endl;
	    error::fatal();
	}
	// only reached if there was a 'case' or 'default' label
	if (token.kind == TokenKind::RBRACE) {
	    error::out() << token.loc
		<< ": label at end of compound statement: "
		<< "expected statement" << std::endl;
	    error::fatal();
	}
    }
    error::expected(TokenKind::RBRACE);
    getToken();
    if (!hasDefault) {
	gen::labelDef(defaultLabel);
    }
    gen::jmp(endLabel);

    gen::labelDef(switchLabel);
    gen::jmp(cond->loadValue(), defaultLabel, caseLabel);
    gen::labelDef(endLabel);
    ControlStruct::breakLabel.pop_back();
    return true;
}

static bool
parseStmt(void)
{
    return parseCompoundStmt(true)
	|| parseIfStmt()
	|| parseWhileStmt()
	|| parseForStmt()
	|| parseReturnStmt()
	|| parseBreakStmt()
	|| parseContinueStmt()
	|| parseExprStmt()
	|| parseTypeDef()
	|| parseStructDef()
	|| parseLocalDefStmt()
	|| parseSwitchStmt();
}

//------------------------------------------------------------------------------

static bool
parseFn(void)
{
    Symtab::openScope();

    auto fnTok = token;
    std::vector<const char *> fnParamIdent;
    auto fnDecl = parseFnDecl(fnParamIdent);
    if (!fnDecl) {
	Symtab::closeScope();
	return false;
    }

    if (token.kind == TokenKind::SEMICOLON) {
	getToken();
	gen::fnDecl(fnDecl->ident.c_str(), fnDecl->getType());
    } else {
	error::expected(TokenKind::LBRACE);
	gen::fnDef(fnDecl->ident.c_str(), fnDecl->getType(), fnParamIdent);
	if (fnDecl->ident == UStr{"main"}) {
	    auto ty = fnDecl->getType()->getRetType();
	    if (!ty->isInteger() && !ty->isVoid()) {
		error::out() << fnTok.loc
		    << ": function 'main' can only have an integer return type"
		    << std::endl;
		error::fatal();
	    } else if (ty->isInteger()) {
		auto ret = Expr::createIdentifier(UStr{".retVal"});
		auto zero = Expr::createLiteral("0", 10);
		ret = Expr::createBinary(Binary::Kind::ASSIGN, std::move(ret),
					  std::move(zero));
		ret->loadValue();
	    }
	}
	assert(parseCompoundStmt(false));
	gen::fnDefEnd();
    }

    Symtab::closeScope();
    return true;
}
