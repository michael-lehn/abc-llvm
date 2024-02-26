#ifndef AST_HPP
#define AST_HPP

#include <functional>
#include <memory>
#include <variant>
#include <vector>

#include "initializerlist.hpp"
#include "lexer.hpp"
#include "type.hpp"

class Ast
{
    public:
	Ast() = default;
	virtual ~Ast() = default;

	virtual void print(int indent = 0) const = 0;
	virtual void codegen();
	virtual void apply(std::function<bool(Ast *)> op);
	virtual const Type *getType() const;
};

using AstPtr = std::unique_ptr<Ast>;

//------------------------------------------------------------------------------

class AstList : public Ast
{
    public:
	std::vector<AstPtr> node;

	AstList() = default;

	std::size_t size() const;
	void append(AstPtr &&ast);
	void print(int indent = 0) const override;
	void codegen() override;
	void apply(std::function<bool(Ast *)> op) override;
};

using AstListPtr = std::unique_ptr<AstList>;

//------------------------------------------------------------------------------

class AstCompound : public Ast
{
    private:
	AstList list;

    public:
	AstCompound() = default;

	void append(AstPtr &&ast);

	void print(int indent) const override;
	void codegen() override;
	void apply(std::function<bool(Ast *)> op) override;
};

//------------------------------------------------------------------------------

class AstIf : public Ast
{
    public:
	AstIf(ExprPtr &&cond, AstPtr &&thenBody);
	AstIf(ExprPtr &&cond, AstPtr &&thenBody, AstPtr &&elseBody);

	const ExprPtr cond;
	const AstPtr thenBody;
	const AstPtr elseBody;

	void print(int indent) const override;
	void codegen() override;
	void apply(std::function<bool(Ast *)> op) override;
};

//------------------------------------------------------------------------------

class AstSwitch : public Ast
{
    private:
	AstList body;
	std::vector<std::size_t> casePos;
	std::vector<ExprPtr> caseExpr;
	std::size_t defaultPos;
	bool hasDefault;

    public:
	AstSwitch(ExprPtr &&expr);

	const ExprPtr expr;

	void appendCase(ExprPtr &&expr);
	bool appendDefault();
	void append(AstPtr &&stmt);

	void print(int indent) const override;
	void codegen() override;
	void apply(std::function<bool(Ast *)> op) override;
};

//------------------------------------------------------------------------------

class AstWhile : public Ast
{
    public:
	AstWhile(ExprPtr &&cond, AstPtr &&body);

	const ExprPtr cond;
	const AstPtr body;

	void print(int indent) const override;
	void codegen() override;
	void apply(std::function<bool(Ast *)> op) override;
};

//------------------------------------------------------------------------------

class AstDoWhile : public Ast
{
    public:
	AstDoWhile(ExprPtr &&cond, AstPtr &&body);

	const ExprPtr cond;
	const AstPtr body;

	void print(int indent) const override;
	void codegen() override;
	void apply(std::function<bool(Ast *)> op) override;
};

//------------------------------------------------------------------------------

class AstFor : public Ast
{
    private:
	AstPtr body;

    public:
	AstFor(ExprPtr &&init, ExprPtr &&cond, ExprPtr &&update);
	AstFor(AstPtr &&init, ExprPtr &&cond, ExprPtr &&update);

	using AstOrExpr = std::variant<AstPtr, ExprPtr>;

	const AstOrExpr init;
	const ExprPtr cond, update;

	void appendBody(AstPtr &&body);

	void print(int indent) const override;
	void codegen() override;
	void apply(std::function<bool(Ast *)> op) override;
};

//------------------------------------------------------------------------------

class AstReturn : public Ast
{
    public:
	AstReturn(ExprPtr &&expr);

	const Type *retType = nullptr;
	ExprPtr expr;

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

class AstGoto : public Ast
{
    public:
	AstGoto(Token::Loc loc, UStr labelIdent);

	const Token::Loc loc;
	UStr labelIdent;
	gen::Label label = nullptr;

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

class AstLabel : public Ast
{
    public:
	AstLabel(Token::Loc loc, UStr labelIdent);

	const Token::Loc loc;
	UStr labelIdent;
	gen::Label label = nullptr;

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

class AstBreak : public Ast
{
    public:
	AstBreak(Token::Loc loc);

	const Token::Loc loc;
	gen::Label label = nullptr;

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

class AstContinue : public Ast
{
    public:
	AstContinue(Token::Loc loc);

	const Token::Loc loc;
	gen::Label label = nullptr;

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

class AstExpr : public Ast
{
    public:
	AstExpr(ExprPtr &&expr);

	ExprPtr expr;

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

class AstInitializerList : public Ast
{
    public:
	AstInitializerList(const Type *type);
	AstInitializerList(const Type *type, ExprPtr &&expr);
	AstInitializerList(const Type *type, Token::Loc loc, UStr str);

	const Type *type;
	std::variant<AstList, ExprPtr> initializer;

	using AstInitializerListPtr = std::unique_ptr<AstInitializerList>;
	void append(AstInitializerListPtr &&initializerItem);

	void print(int indent) const override;
	InitializerList createInitializerList() const;
};

using AstInitializerListPtr = AstInitializerList::AstInitializerListPtr;

//------------------------------------------------------------------------------

class AstVar : public Ast
{
    public:
	AstVar(UStr ident, const Type *type, Token::Loc loc);

	void addInitializer(AstInitializerListPtr &&initializer);

	const UStr ident;
	const Type * const type;
	Token::Loc loc;
	UStr genIdent; // used for code generation, set when defined
	AstInitializerListPtr initializer;
	bool externFlag = false;

	void print(int indent) const override;
};

using AstVarPtr = std::unique_ptr<AstVar>;

//------------------------------------------------------------------------------

class AstExternVar : public Ast
{
    public:
	AstExternVar(AstListPtr &&decl);

	const AstList decl;

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

class AstGlobalVar : public Ast
{
    public:
	AstGlobalVar(AstListPtr &&decl);

	const AstList decl;

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

class AstLocalVar : public Ast
{
    public:
	AstLocalVar(AstListPtr &&decl);

	const AstList decl;

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

class AstFuncDecl : public Ast
{
    public:
	AstFuncDecl(Token fnIdent, const Type *type,
		    std::vector<Token> paramToken,
		    bool externFlag = false);

	const Token fnIdent;
	const Type * const type;
	const std::vector<Token> paramToken;
	const bool externFlag;

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

class AstFuncDef : public Ast
{
    public:
	AstFuncDef(Token fnIdent, const Type *type,
		   std::vector<Token> paramToken, bool externFlag = false);
	~AstFuncDef();

	const Token fnIdent;
	const Type * const type;
	const std::vector<Token> paramToken;
	std::vector<const char *> paramIdent;
	AstPtr body;
	const bool externFlag;

	void appendBody(AstPtr &&body);

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

class AstTypeDecl : public Ast
{
    public:
	AstTypeDecl(Token tyIdent, const Type *type);

	const Token tyIdent;
	const Type * const type;

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

class AstStructDecl : public Ast
{
    private:
	Type *createIncompleteStruct(Token ident);

    public:
	using AstOrType = std::variant<AstPtr, const Type *>;

	AstStructDecl(Token ident);
	void complete(std::vector<Token> &&member,
		      std::vector<AstOrType> &&memberAstOrType);

	const Token ident;
	Type *type;
	bool hasSize = false;
	AstList astList;

	void print(int indent) const override;
	void codegen() override;
	const Type *getType() const override;
};

using AstStructDeclPtr = std::unique_ptr<AstStructDecl>;

//------------------------------------------------------------------------------

class AstEnumDecl : public Ast
{
    private:
	Token ident;
	const Type *intType;
	Type *type;
	std::int64_t enumVal = 0;
	std::vector<UStr> enumIdent;
	std::vector<Token::Loc> enumIdentLoc;
	std::vector<ExprPtr> enumConstExpr;
	std::vector<std::int64_t> enumConstValue;

    public:
	AstEnumDecl(Token ident, const Type *intType);

	void add(Token enumIdent);
	void add(Token enumIdent, ExprPtr &&enumConstExpr);
	void complete();

	void print(int indent) const override;
	void codegen() override;
	const Type *getType() const override;
};

using AstEnumDeclPtr = std::unique_ptr<AstEnumDecl>;

#endif // AST_HPP
