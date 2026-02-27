#pragma once
#include <typeinfo>

namespace ren
{
    class TransformNode;

    class TNComponent
    {
    public:        
        TransformNode* getNode() const;

        virtual void update();

        template <class T>
        bool isOfType() const;

    protected:
        friend class TransformNode;

        TNComponent() = delete;
        TNComponent(TransformNode* _owner);
        TNComponent(const TNComponent& _other) = delete;
        TNComponent(const TNComponent& _other, TransformNode* _newOwner);
        TNComponent(TNComponent&&) = delete;
        virtual ~TNComponent() = default;

        virtual TNComponent* clone(TransformNode* _newOwner) const = 0;

        TransformNode* p_node;
    };

    template <class T>
    bool TNComponent::isOfType() const 
    {
        if (typeid(*this) == typeid(T)) {
            return true;
        }
        else {
            return false;
        }
    }
}