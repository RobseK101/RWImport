#include <ren/TncMeshIndex.h>

namespace ren
{
    TncMeshIndex::TncMeshIndex(TransformNode* _owner) : 
        TNComponent(_owner), meshIndex(-1) {}

    TncMeshIndex::TncMeshIndex(const TncMeshIndex& _other, TransformNode* _newOwner) : 
        TNComponent(_other, _newOwner), meshIndex(_other.meshIndex) {}

    TNComponent* TncMeshIndex::clone(TransformNode* _newOwner) const {
        return new TncMeshIndex(*this, _newOwner);
    }
}