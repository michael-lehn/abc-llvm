// TEST: T0301
#include "gen/gen.hpp"

#include "check.hpp"

// The driver gets told about invalid IR.
int
main(int, char *argv[])
{
    gen::init("test_gen");
    CHECK(gen::verifyModule());

    // a function with a block without terminator
    auto type = llvm::FunctionType::get(
        llvm::Type::getVoidTy(*gen::llvmContext), false);
    auto fn = llvm::Function::Create(type, llvm::Function::ExternalLinkage, "f",
                                     *gen::llvmModule);
    auto bb = llvm::BasicBlock::Create(*gen::llvmContext, "entry", fn);
    CHECK(!gen::verifyModule());

    llvm::ReturnInst::Create(*gen::llvmContext, bb);
    CHECK(gen::verifyModule());

    return check::summary(argv[0]);
}
