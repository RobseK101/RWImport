#pragma once
#include <ren/TNComponent.h>

namespace ren
{
    class TncMeshIndex : public TNComponent
    {
    public:
        int meshIndex;

    private:
        friend class TransformNode;

        TncMeshIndex() = delete;
        TncMeshIndex(TransformNode* _owner);
        TncMeshIndex(const TncMeshIndex&) = delete;
        TncMeshIndex(const TncMeshIndex& _other, TransformNode* _newOwner);
        TncMeshIndex(TncMeshIndex&&) = delete;
        virtual ~TncMeshIndex() = default;

        virtual TNComponent* clone(TransformNode* _newOwner) const override;
    };
}