#include <cassert>
#include <memory>
#include <sstream>
#include <tuple>
#include <unordered_map>

#include "structtype.hpp"

namespace abc {

static std::unordered_map<std::size_t, StructType> structSet;
static std::unordered_map<std::size_t, StructType> structConstSet;

//------------------------------------------------------------------------------

StructType::StructType(std::size_t id, UStr name, bool constFlag)
    : Type{constFlag, name}, id_{id}, isComplete_{false}
{
}

void 
StructType::init()
{
    structSet.clear();
    structConstSet.clear();
}

Type *
StructType::createIncomplete(UStr name)
{
    static std::size_t count;
    auto id = count++;

    structSet.emplace(id, StructType{id, name, false});
    structConstSet.emplace(id, StructType{id, name, true});

    return &structSet.at(id);
}

std::size_t
StructType::id() const
{
    return id_;
}

const Type *
StructType::getConst() const
{
    return &structConstSet.at(id());
}

const Type *
StructType::getConstRemoved() const
{
    return &structSet.at(id());
}

bool
StructType::hasSize() const
{
    return isComplete_;
}

bool
StructType::isStruct() const
{
    return true;
}

const Type *
StructType::complete(std::vector<UStr> &&memberName,
		     std::vector<std::size_t> &&memberIndex,
		     std::vector<const Type *> &&memberType)
{
    assert(memberName.size() == memberIndex.size());
    assert(memberName.size() == memberType.size());
    auto &constStructType = structConstSet.at(id());
    for (std::size_t i = 0; i < memberName.size(); ++i) {
	constStructType.memberName_.push_back(memberName[i]);	
	constStructType.memberIndex_.push_back(memberIndex[i]);	
	constStructType.memberType_.push_back(memberType[i]->getConst());	
    }
    constStructType.isComplete_ = true;

    memberName_ = std::move(memberName);
    memberIndex_ = std::move(memberIndex);
    memberType_ = std::move(memberType);
    isComplete_ = true;
    return this;
}

const std::vector<UStr> &
StructType::memberName() const
{
    return memberName_;
}

const std::vector<std::size_t> &
StructType::memberIndex() const
{
    return memberIndex_;
}

const std::vector<const Type *> &
StructType::memberType() const
{
    return memberType_;
}

std::size_t
StructType::aggregateSize() const
{
    assert(!memberIndex_.empty());
    return memberIndex_.back() + 1;
}

const Type *
StructType::aggregateType(std::size_t index) const
{
    for (std::size_t i = 0; i < memberIndex_.size(); ++i) {
	if (memberIndex_[i] == index) {
	    return memberType_[i];
	}
    }
    assert(0);
    return nullptr;
}

} // namespace abc
