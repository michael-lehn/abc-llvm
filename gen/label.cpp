#include "function.hpp"
#include "instruction.hpp"
#include "label.hpp"

namespace gen {

Label
getLabel(const char *name)
{
    assert(llvmContext);
    return llvm::BasicBlock::Create(*llvmContext, name);
}

void
defineLabel(Label label)
{
    assert(llvmContext);
    assert(functionBuildingInfo.fn);
    if (!functionBuildingInfo.bbClosed) {
	jumpInstruction(label);
    }

    functionBuildingInfo.fn->insert(functionBuildingInfo.fn->end(), label);
    llvmBuilder->SetInsertPoint(label);
    functionBuildingInfo.bbClosed = false;
}

} // namespace gen
