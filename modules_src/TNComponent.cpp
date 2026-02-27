#include <ren/TNComponent.h>

namespace ren
{
    TNComponent::TNComponent(TransformNode* _owner) : 
        p_node(_owner) {}

    TNComponent::TNComponent(const TNComponent& _other, TransformNode* _newOwner) : 
        p_node(_newOwner) {}

    TransformNode* TNComponent::getNode() const
    {
        return p_node;
    }

    void TNComponent::update() {}
}