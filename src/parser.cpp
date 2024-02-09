#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <sstream>

#include "binaryexpr.hpp"
#include "castexpr.hpp"
#include "error.hpp"
#include "gen.hpp"
#include "identifier.hpp"
#include "initializerlist.hpp"
#include "integerliteral.hpp"
#include "lexer.hpp"
#include "parseexpr.hpp"
#include "parser.hpp"
#include "symtab.hpp"
#include "unaryexpr.hpp"
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
static bool parseExternDecl(void);
static bool parseGlobalDef(void);
static bool parseStructDef(void);
static bool parseTypeDef(void);
static bool parseEnumDef(void);

void
parser(void)
{
    getToken();
    while (token.kind != TokenKind::EOI) {
	if (!parseGlobalDef() && !parseFn() && !parseTypeDef()
		&& !parseExternDecl() && !parseStructDef() && !parseEnumDef()) {
	    error::out() << token.loc
		<< ": expected function declaration,"
		<< " global variable definition,"
		<< " type declaration, extern declaration, enum declaration"
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
	error::expected(TokenKind::COLON);
	getToken();
	auto ty = parseType();
	if (!ty) {
	    error::out() << token.loc << ": type expected" << std::endl;
	    error::fatal();
	}
	ty = Type::createAlias(ident, ty);
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
parseEnumDef(void)
{
    if (token.kind != TokenKind::ENUM) {
	return false;
    }
    getToken();

    error::expected(TokenKind::IDENTIFIER);
    auto enumTok = token;
    UStr enumName = token.val;
    getToken();

    error::expected(TokenKind::COLON);
    getToken();

    auto enumTyTok = token;
    auto enumTy = parseType();
    if (!enumTy && !enumTy->isInteger()) {
	error::out() << enumTyTok.loc << ": integer type expected" << std::endl;
	error::fatal();
    }

    enumTy = Symtab::addTypeAlias(enumName, enumTy);
    if (!enumTy) {
	auto sym = Symtab::get(enumName);
	error::out() << enumTok.loc << ": redefinition of type name '"
	    << enumName.c_str() << "'. Previous definition at "
	    << sym->getLoc() << std::endl;
	error::fatal();
    }
    enumTy = Type::createAlias(enumName, enumTy);

    // TODO: support incomplete enum decl
    error::expected(TokenKind::LBRACE);
    getToken();

    // TODO: instead of 'ptrdiff_t' use 'enumTy' equivalent
    for (std::ptrdiff_t i = 0; token.kind == TokenKind::IDENTIFIER; ++i) {
	auto identTok = token;
	getToken();
	if (token.kind == TokenKind::EQUAL) {
	    getToken();
	    auto valTok = token;
	    auto val = parseExpr();
	    if (!val && !val->isConst() && !val->type->isInteger()) {
		error::out() << valTok.loc << ": integer constant expected"
		    << std::endl;
		error::fatal();
	    }
	    using T = std::remove_pointer_t<gen::ConstIntVal>;
	    auto check = llvm::dyn_cast<T>(val->loadConstValue());
	    assert(check);
	    i = check->getSExtValue();
	}
	std::stringstream ss;
	ss << i;
	UStr val{ss.str()};
	auto expr = IntegerLiteral::create(val, 10, enumTy, token.loc);
	Symtab::addConstant(identTok.loc, identTok.val, std::move(expr));
	if (token.kind != TokenKind::COMMA) {
	    break;
	}
	getToken();
    }

    error::expected(TokenKind::RBRACE);
    getToken();
    error::expected(TokenKind::SEMICOLON);
    getToken();
    return true;
}

bool
parseInitializerList(InitializerList &initList, bool global)
{
    if (token.kind != TokenKind::LBRACE) {
	return false;
    }
    getToken();

    auto type = initList.type();
    for (std::size_t pos = 0; type ? pos < type->getNumMembers(): true; ++pos) {
	auto loc = token.loc;
	if (auto expr = parseExpr()) {
	    if (global && !expr->isConst()) {
		error::out() << loc
		    << ": initializer element is not constant" << std::endl;
		error::fatal();
	    }
	    initList.add(std::move(expr));
	} else {
	    auto ty = type ? type->getMemberType(pos) : nullptr;
	    InitializerList constMemberExpr(ty);
	    if (parseInitializerList(constMemberExpr, global)) {
		initList.add(std::move(constMemberExpr));
	    } else {
		break;
	    }
	}
	if (token.kind != TokenKind::COMMA) {
	    break;
	}
	getToken();
    }

    error::expected(TokenKind::RBRACE);
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

	//auto s = Symtab::addDeclToRootScope(loc, ident.c_str(), type);
	auto s = Symtab::addDecl(loc, ident.c_str(), type);
	auto ty = s->type();

	// parse initalizer
	InitializerList initList(type);
	if (token.kind == TokenKind::EQUAL) {
	    getToken();
	    auto opLoc = token.loc;
	    if (auto expr = parseExpr()) {
		if (!expr->isConst()) {
		    error::out() << opLoc << ": initializer element is not a"
			<< " compile-time constant" << std::endl;
		    error::fatal();
		}
		gen::defGlobal(s->ident.c_str(), ty, expr->loadConstValue());
	    } else if (parseInitializerList(initList, true)) {
		gen::defGlobal(s->ident.c_str(), ty, initList.loadConstValue());
	    } else {
		error::out() << opLoc
		    << " expected initializer or expression"
		    << std::endl;
		error::fatal();
	    }
	} else {
	    gen::defGlobal(s->ident.c_str(), ty);
	}

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
		       bool &hasVarg,
		       std::vector<const char *> *paramIdent = nullptr)
{
    std::size_t paramId = 0;

    argType.clear();
    if (paramIdent) {
	paramIdent->clear();
    }

    hasVarg = false;
    while (token.kind == TokenKind::IDENTIFIER
        || token.kind == TokenKind::COLON
	|| token.kind == TokenKind::DOT3)
    {
	// if parameter has no identifier give it an interal identifier 
	UStr ident;
	auto loc = token.loc;
	if (token.kind == TokenKind::IDENTIFIER) {
	    ident = token.val.c_str();
	    getToken();
	} else if (token.kind == TokenKind::DOT3) {
	    getToken();
	    hasVarg = true;
	    break;
	} else {
	    std::stringstream ss;
	    ss << ".param" << paramId++;
	    ident = UStr{ss.str()};
	}

	error::expected(TokenKind::COLON);
	getToken();

	// parse param type
	//auto typeLoc = token.loc;
	auto type = parseType();
	if (!type) {
	    error::out() << token.loc << " type expected"
		<< std::endl;
	    error::fatal();
	}
	/*
	if (type->isArray()) {
	    error::out() << typeLoc << ": warning: treating '" << type
		<< "' as '" << Type::convertArrayOrFunctionToPointer(type)
		<< std::endl;
	    type = Type::convertArrayOrFunctionToPointer(type);
	}
	*/
	argType.push_back(type);

	// add param to symtab if this is a declaration
	if (paramIdent) {
	    auto s = Symtab::addDecl(loc, ident.c_str(), type);
	    if (!s) {
		error::out() << loc << ": parameter '" << ident.c_str()
		    << "' already defined" << std::endl;
		error::fatal();
	    }
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
parseFnParamDecl(std::vector<const Type *> &argType, bool &hasVarg,
		 std::vector<const char *> &paramIdent)
{
    parseFnParamDeclOrType(argType, hasVarg, &paramIdent);
}

static void
parseFnParamType(std::vector<const Type *> &argType, bool &hasVarg)
{
    parseFnParamDeclOrType(argType, hasVarg);
}

//------------------------------------------------------------------------------

static const Type *
parseFnDeclOrType(std::vector<const Type *> &argType, const Type *&retType,
	          Symtab::Entry **fnDecl = nullptr,
		  std::vector<const char *> *fnParamIdent = nullptr)
{
    UStr fnIdent;

    if (token.kind != TokenKind::FN) {
	return nullptr;
    }
    getToken();
    Token::Loc fnLoc = token.loc;

    // parse function identifier
    if (fnDecl) {
	error::expected(TokenKind::IDENTIFIER);
    }
    if (token.kind == TokenKind::IDENTIFIER) {
	fnIdent = token.val;
	getToken();
    }

    // parse function parameters
    error::expected(TokenKind::LPAREN);
    getToken();
    bool hasVarg;
    if (fnDecl) {
	parseFnParamDecl(argType, hasVarg, *fnParamIdent);
    } else {
	parseFnParamType(argType, hasVarg);
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

    auto fnType = Type::getFunction(retType, argType, hasVarg);
    assert(fnType);
    
    if (fnDecl) {
	*fnDecl = Symtab::addDeclToRootScope(fnLoc, fnIdent.c_str(), fnType);
	assert((*fnDecl)->type() == fnType);
    }

    return fnType;
}

static Symtab::Entry *
parseFnDecl(std::vector<const char *> &fnParamIdent)
{
    std::vector<const Type *> argType;
    const Type *retType = nullptr;
    Symtab::Entry *fnDecl = nullptr;

    parseFnDeclOrType(argType, retType, &fnDecl, &fnParamIdent);
    return fnDecl;
}

static Symtab::Entry *
parseFnDecl()
{
    std::vector<const char *> fnParamIdent;
    return parseFnDecl(fnParamIdent);
}

static const Type *
parseFnType(void)
{
    std::vector<const Type *> argType;
    const Type *retType = nullptr;

    return parseFnDeclOrType(argType, retType);
}

//------------------------------------------------------------------------------

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
    if (!dim || !dim->isConst() || !dim->type->isInteger()) {
	error::out() << token.loc
	    << " dimension has to be a constant integer expression"
	    << std::endl;
	error::fatal();
    }
    auto dimVal = llvm::dyn_cast<llvm::ConstantInt>(dim->loadConstValue());

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
    } else if (!ty->hasSize()) {
	error::out() << token.loc << ": array has incomplete element type '"
	    << ty << "'"
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
	std::vector<const char *> identWithSameType;
	while (token.kind == TokenKind::IDENTIFIER) {
	    identWithSameType.push_back(token.val.c_str());
	    getToken();
	    if (token.kind != TokenKind::COMMA) {
		break;
	    }
	    getToken();
	}
	error::expected(TokenKind::COLON);
	getToken();
	auto ty = parseType();
	if (!ty) {
	    ty = parseStructDecl();
	}
	if (!ty) {
	    error::out() << token.loc << ": type expected" << std::endl;
	    error::fatal();
	}
	for (auto id : identWithSameType) {
	    ident.push_back(id);
	    type.push_back(ty);
	}
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

static const Type *
parseUnqualifiedType(void)
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
    return nullptr;
}

const Type *
parseType(void)
{
    bool constFlag = false;
    auto constTok = token;
    if (token.kind == TokenKind::CONST) {
	getToken();
	constFlag = true;
    }
    auto tyTok = token;
    auto ty = parseUnqualifiedType();
    if (constFlag && !ty) {
	error::out() << tyTok.loc
	    << ": expected type after 'const'" << std::endl;
	error::fatal();
    } else if (constFlag && ty->isArray()) {
	error::out() << tyTok.loc
	    << ": 'const' qualifier for array not allowed" << std::endl;
	auto constTy = Type::getConst(ty);
	if (constTy) {
	    error::out() << tyTok.loc
		<< ": You can apply 'const' to the element types: '"
		<< constTy
		<< "'" << std::endl;
	}
	error::fatal();
    } else if (constFlag) {
	auto constTy = Type::getConst(ty);
	if (!constTy) {
	    error::out() << constTok.loc
		<< ": 'const' qualifier can not be used for '"
		<< ty << "'" << std::endl;
	    error::fatal();
	}
	return constTy;
    }
    return ty;
}

const Type *
parseIntType(void)
{
    if (token.kind != TokenKind::IDENTIFIER) {
	return nullptr;
    }
    if (auto ty = Symtab::getNamedType(token.val, Symtab::AnyScope)) {
	getToken();
	return ty;
    }
    return nullptr;

}

//------------------------------------------------------------------------------

static bool
parseInitializer(gen::Reg addr, const Type *type)
{
    if (token.kind != TokenKind::LBRACE) {
	return false;
    }
    getToken();

    for (std::size_t pos = 0; pos < type->getNumMembers(); ++pos) {
	auto dest = gen::ptrMember(type, addr, pos);
	auto memTy = type->getMemberType(pos);
	if (!memTy) {
	    error::out() << token.loc
		<< ": excess elements in initializer" << std::endl;
	    error::fatal();
	}
	auto exprLoc = token.loc;
	if (auto expr = parseExpr()) {
	    if (!Type::getTypeConversion(expr->type, memTy, exprLoc)) {
		error::out() << exprLoc
		    << ": can not cast '" << expr->type << "' to '"
		    << memTy << "'" << std::endl;
		error::fatal();
	    }
	    expr = CastExpr::create(std::move(expr), memTy, exprLoc);
	    auto val = expr->loadValue();
	    gen::store(val, dest, memTy);
	} else if (parseInitializer(dest, memTy)) {
	} else {
	    error::expected(TokenKind::RBRACE);
	    auto val = gen::loadZero(memTy);
	    gen::store(val, dest, memTy);
	}
	if (token.kind == TokenKind::COMMA) {
	    getToken();
	    continue;
	}
	error::expected(TokenKind::RBRACE);
    }
    error::expected(TokenKind::RBRACE);
    getToken();

    return true;
}

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
	gen::defLocal(s->ident.c_str(), s->type());

	// parse initalizer
	if (token.kind == TokenKind::EQUAL) {
	    getToken();

	    auto rcPtrTy = Type::getPointer(Type::getConstRemoved(type));
	    auto var = Identifier::create(ident, loc);
	    var = UnaryExpr::create(UnaryExpr::ADDRESS, std::move(var), loc);
	    var = CastExpr::create(std::move(var), rcPtrTy, loc);
	    var = UnaryExpr::create(UnaryExpr::DEREF, std::move(var));
	    auto addr = var->loadAddr();
	    if (!parseInitializer(addr, var->type)) {
		auto initLoc = token.loc;
		auto init = parseExpr();
		if (!init) {
		    error::out() << token.loc
			<< " expected non-empty expression" << std::endl;
		    error::fatal();
		}
		init = CastExpr::create(std::move(init), var->type, initLoc);
		auto val = init->loadValue();
		gen::store(val, addr, type);
	    }
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
	error::warning();
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
	cond = IntegerLiteral::create("1");
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
    if (expr && !Symtab::get(UStr{".retVal"})) {
	error::out() << exprTok.loc
	    << "  function with return type 'void' should not return a value"
	    << std::endl;
	error::fatal();
    } else if (!expr && Symtab::get(UStr{".retVal"})) {
	auto s = Symtab::get(UStr{".retVal"});
	error::out() << exprTok.loc
	    << "  function with return type '" << s->type()
	    << "' should return a value" << std::endl;
	error::fatal();
    }
    if (expr) { 
	auto retTy = Symtab::get(UStr{".retVal"})->type();
	expr = CastExpr::create(std::move(expr), retTy, exprTok.loc);
	gen::ret(expr->loadValue());
    } else {
	gen::ret();
    }
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
    if (!expr) {
	return false;
    }
    error::expected(TokenKind::SEMICOLON);
    getToken();
    reachableCheck(exprTok, "expression");
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
    auto condTok = token;
    auto cond = parseExpr();
    if (!cond || !cond->type->isInteger()) {
	error::out() << condTok.loc
	    << ": integer expression expected" << std::endl;
	error::fatal();
    }
    error::expected(TokenKind::RPAREN);
    getToken();

    error::expected(TokenKind::LBRACE);
    getToken();
    Symtab::openScope();

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
	    if (!val || !val->isConst() || !val->type->isInteger()) {
		error::out() << valTok.loc
		    << ": constant integer expression required" << std::endl;
		error::fatal();
	    }
	    val = CastExpr::create(std::move(val), cond->type, valTok.loc);
	    error::expected(TokenKind::COLON);
	    getToken();
	    auto label = gen::getLabel("case");
	    using T = std::remove_pointer_t<gen::ConstIntVal>;
	    auto caseVal = llvm::dyn_cast<T>(val->loadConstValue());
	    assert(caseVal);
	    caseLabel.push_back({caseVal, label});
	    if (usedCaseVal.contains(caseVal->getZExtValue())) {
		error::out() << valTok.loc << ": duplicate case value '"
		    << caseVal->getZExtValue() << "'" << std::endl;
		error::fatal();
	    } else {
		gen::labelDef(label);
	    }
	    usedCaseVal.insert(caseVal->getZExtValue());
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
    Symtab::closeScope();
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
	|| parseTypeDef()
	|| parseStructDef()
	|| parseLocalDefStmt()
	|| parseGlobalDef()
	|| parseSwitchStmt()
	|| parseExprStmt();
}

//------------------------------------------------------------------------------

static bool
parseExternDecl(void)
{
    if (token.kind != TokenKind::EXTERN) {
	return false;
    }
    getToken();

    Symtab::openScope();
    auto fnDecl = parseFnDecl();
    Symtab::closeScope();

    if (fnDecl) {
	fnDecl->setExternFlag();
	gen::fnDecl(fnDecl->ident.c_str(), fnDecl->type());
    } else {
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
	s->setExternFlag();
	gen::declGlobal(ident.c_str(), type);
    }
    error::expected(TokenKind::SEMICOLON);
    getToken();

    return true;
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
	gen::fnDecl(fnDecl->ident.c_str(), fnDecl->type());
    } else {
	error::expected(TokenKind::LBRACE);
	gen::fnDef(fnDecl->ident.c_str(), fnDecl->type(), fnParamIdent);
	Symtab::setPrefix(fnDecl->ident.c_str());
	if (fnDecl->ident == UStr{"main"}) {
	    auto ty = fnDecl->type()->getRetType();
	    if (!ty->isInteger() && !ty->isVoid()) {
		error::out() << fnTok.loc
		    << ": function 'main' can only have an integer return type"
		    << std::endl;
		error::fatal();
	    } else if (ty->isInteger()) {
		auto ret = Identifier::create(UStr{".retVal"});
		auto zero = IntegerLiteral::create("0");
		ret = BinaryExpr::create(BinaryExpr::Kind::ASSIGN,
					 std::move(ret),
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
