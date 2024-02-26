#include <iostream>

#include "error.hpp"
#include "gen.hpp"
#include "type.hpp"
#include "symtab.hpp"

const Type *
makeStruct1(UStr name)
{
    auto incompleteStructTy = Type::createIncompleteStruct(name);

    std::vector<UStr> ident;
    std::vector<const Type *> type;

    ident.push_back(UStr::create("first"));
    type.push_back(Type::getUnsignedInteger(8));

    ident.push_back(UStr::create("second"));
    type.push_back(Type::getArray(Type::getConst(Type::getUnsignedInteger(16)), 4));

    auto ty = incompleteStructTy->complete(std::move(ident), std::move(type));

    if (!ty) {
	error::out() << " struct '" << name.c_str() << "' already defined\n";
	error::fatal();
    }
    return ty;
}

const Type *
makeStruct2(UStr name)
{
    return Type::createIncompleteStruct(name);
}

const Type *
makeStruct3(UStr name, const Type *tm1, const Type *tm3)
{
    auto incompleteStructTy = Type::createIncompleteStruct(name);

    std::vector<UStr> ident;
    std::vector<const Type *> type;

    ident.push_back(UStr::create("first"));
    type.push_back(tm1);

    ident.push_back(UStr::create("second"));
    type.push_back(Type::getUnsignedInteger(16));

    ident.push_back(UStr::create("third"));
    type.push_back(tm3);

    auto ty = incompleteStructTy->complete(std::move(ident), std::move(type));

    if (!ty) {
	error::out() << " struct '" << name.c_str() << "' already defined\n";
	error::fatal();
    }
    return ty;
}


int
main(void)
{
    gen::init();
    Symtab::openScope();

    auto ty1 = makeStruct1(UStr::create("Foo"));
    std::cout << "ty1 = " << ty1 << ", &ty1 = " << (void *) ty1 << std::endl;

    auto constTy1 = Type::getConst(ty1);
    std::cout << "constTy1 = " << constTy1
	<< ", &constTy1 = " << (void *) constTy1 << std::endl;

    auto nonConstTy1 = Type::getConstRemoved(constTy1);
    std::cout << "nonConstTy1 = " << nonConstTy1
	<< ", &nonConstTy1 = " << (void *) nonConstTy1 << std::endl;


    auto ty2 = makeStruct2(UStr::create("Bar"));
    std::cout << "ty2 = " << ty2 << ", &ty2 = " << (void *) ty2 << std::endl;

    ty2 = makeStruct2(UStr::create("Bar"));
    std::cout << "ty2 = " << ty2 << ", &ty2 = " << (void *) ty2 << std::endl;


    auto ty3 = makeStruct3(UStr::create("FooBar"), ty1, ty2);
    std::cout << "ty3 = " << ty3 << ", &ty3 = " << (void *) ty3 << std::endl;

    Symtab::closeScope();
}
