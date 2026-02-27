#pragma once
#include <ren/ModelData.hpp>
#include <utility>
#include <ren/TNComponent.h>

namespace ren
{
    class TransformNode
    {
    public:
        using String = std::string;
        static constexpr String::value_type rootNameDefault[] = {'$', '$', 'r', 'o', 'o', 't', 0};

        TransformNode();
        TransformNode(TransformNode&&) = delete;

        virtual void destroy();
        virtual TransformNode* clone() const;

        TransformNode* getParent() const;
        void setParent(TransformNode* _newParent);

        Vector3 getPosition() const;
        void setPosition(Vector3 _newPosition);

        Vector3 getPositionGlobal() const;

        Quaternion getRotation() const;
        void setRotation(Quaternion _newRotation);

        Quaternion getRotationGlobal() const;

        size_t childCount() const;
        TransformNode* getChild(size_t _index) const;
        int findChild(const TransformNode* _child) const;
        int findChild(const String& _name, int _startIndex = 0) const;
        int findChildI(const String& _name, int _startIndex = 0) const;
        std::pair<int, int> findSubstring(const String& _substring, int _startIndex = 0) const;
        std::pair<int, int> findSubstringI(const String& _substring, int _startIndex = 0) const;
        void eraseChild(size_t _index);

        int getSiblingIndex() const;

        bool isAncestorOrSelf(const TransformNode* _node) const;

        bool hasComponents() const;
        bool isEmpty() const;
        void prune();

        template <class T>
        T* getComponent() const;

        template <class T>
        T* addComponent();

        bool removeComponent(TNComponent* _component);

        template <class T>
        void removeComponent();

        template <class T>
        T* findComponent() const;

        String name;

    private:
        TransformNode(const TransformNode& _other);
        virtual ~TransformNode();

        // void serializeNode(std::vector<Transform>& _targetVector, int _parentIndex) const;

        void update();

        Vector3 p_position;
        Quaternion p_rotation;
        TransformNode* p_parent;
        std::vector<TransformNode*> p_children;
        std::vector<TNComponent*> p_components;

        glm::mat4 p_matLocalCached = {};
        glm::mat4 p_matGlobalCached = {};
        glm::vec3 p_posGlobalCached = {};
        glm::quat p_rotGlobalCached = {};
        bool p_localMatCached = false;
        bool p_globalMatCached = false;
        bool p_globalPosCached = false;
        bool p_globalRotCached = false;

    // Static methods
    public: 
        // static TransformNode* rebuildHierarchy(const std::vector<Transform>& _vector);
        // static std::vector<Transform> serializeToFlatList(const TransformNode* _rootNode);
    };

    template <class T>
    T* TransformNode::getComponent() const
    {
        for (size_t i = 0; i < p_components.size(); i++) {
            if (typeid(*p_components[i]) == typeid(T)) {
                return static_cast<T*>(p_components[i]);
            }
        }
        return nullptr;
    }

    template <class T>
    T* TransformNode::addComponent()
    {
        if (getComponent<T>() != nullptr) {
            return nullptr;
        }
        TNComponent* component = new T(this);
        p_components.push_back(component);
        return static_cast<T*>(component);
    }

    template <class T>
    void TransformNode::removeComponent() 
    {
        for (size_t i = 0; i < p_components.size(); i++) {
            if (typeid(*p_components[i]) == typeid(T)) {
                delete p_components[i];
                p_components.erase(p_components.begin() + i);
                return;
            }
        }
    }

    template <class T>
    T* TransformNode::findComponent() const
    {
        if (T* component = getComponent<T>()) {
            return component;
        }
        else if (p_parent) {
            return p_parent->findComponent<T>();
        }
        else {
            return nullptr;
        }
    }
}