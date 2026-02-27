#include <ren/TransformNode.h>
#include <stack>
#include <ren/Stringtools.hpp>
#include <ren/Math.hpp>
// #include <ren/Logging.h>

namespace ren
{
    TransformNode::TransformNode() : 
        name(), 
        p_position(),
        p_rotation(0.0f, 0.0f, 0.0f, 1.0f),
        p_parent(nullptr),
        p_children(), 
        p_components() {}

    TransformNode::TransformNode(const TransformNode& _other) : 
        name(_other.name),
        p_position(_other.p_position),
        p_rotation(_other.p_rotation),
        p_parent(nullptr),
        p_children(_other.p_children.size()),
        p_components(_other.p_components.size()) {}

    void TransformNode::destroy() 
    {
        setParent(nullptr);
        delete this;
    }

    TransformNode* TransformNode::clone() const
    {
        TransformNode* clonedNode = new TransformNode(*this);

        for (size_t childIndex = 0; childIndex < p_children.size(); childIndex++) {
            TransformNode* clonedChild = p_children[childIndex]->clone();
            clonedChild->p_parent = clonedNode;
            clonedNode->p_children[childIndex] = clonedChild;
        }

        for (size_t componentIndex = 0; componentIndex < p_components.size(); componentIndex++) {
            clonedNode->p_components[componentIndex] = p_components[componentIndex]->clone(clonedNode);
        }
        
        return clonedNode;
    }

    TransformNode::~TransformNode()
    {
        for (size_t i = 0; i < p_components.size(); i++) {
            delete p_components[i];
        }

        for (size_t i = 0; i < p_children.size(); i++) {
            delete p_children[i];
        }
    }

    TransformNode* TransformNode::getParent() const 
    {
        return p_parent;
    }

    void TransformNode::setParent(TransformNode* _newParent)
    {
        if (_newParent == p_parent) {
            return;
        }

        if (p_parent != nullptr) {
            for (auto it = p_parent->p_children.begin(); it != p_parent->p_children.end(); it++) {
                if (*it == this) {
                    p_parent->p_children.erase(it);
                    break;
                }
            }
        }

        p_parent = _newParent;
        if (p_parent != nullptr) {
            p_parent->p_children.push_back(this);
        }
    }

    Vector3 TransformNode::getPosition() const
    {
        return p_position;
    }

    void TransformNode::setPosition(Vector3 _newPosition)
    {
        p_position = _newPosition;
    }

    Quaternion TransformNode::getRotation() const
    {
        return p_rotation;
    }

    void TransformNode::setRotation(Quaternion _newRotation)
    {
        p_rotation = _newRotation;
    }

    size_t TransformNode::childCount() const
    {
        return p_children.size();
    }

    TransformNode* TransformNode::getChild(size_t _index) const
    {
        if (_index < p_children.size()) {
            return p_children[_index];
        }
        else {
            return nullptr;
        }
    }

    int TransformNode::findChild(const TransformNode* _child) const
    {
        for (int i = 0; i < p_children.size(); i++) {
            if (_child == p_children[i]) {
                return i;
            }
        }
        return -1;
    }

    int TransformNode::findChild(const String& _name, int _startIndex) const 
    {
        for (int i = math::max(0, _startIndex); i < p_children.size(); i++) {
            if (p_children[i]->name == _name) {
                return i;
            }
        }
        return -1;
    }

    int TransformNode::findChildI(const String& _name, int _startIndex) const
    {
        String searchnameUpper = st::toUpper(_name);
        for (int i = math::max(0, _startIndex); i < p_children.size(); i++) {
            String nameUpper = st::toUpper(p_children[i]->name);
            if (nameUpper == searchnameUpper) {
                return i;
            }
        }
        return -1;
    }

    std::pair<int, int> TransformNode::findSubstring(const String& _substring, int _startIndex) const
    {
        for (int i = math::max(0, _startIndex); i < p_children.size(); i++) {
            size_t searchResult = p_children[i]->name.find(_substring);
            if (searchResult != String::npos) {
                return {i, (int)searchResult};
            }
        }
        return {-1, -1};
    }

    std::pair<int, int> TransformNode::findSubstringI(const String& _substring, int _startIndex) const
    {
        String substringUpper = st::toUpper(_substring);
        for (int i = math::max(0, _startIndex); i < p_children.size(); i++) {
            String nameUpper = st::toUpper(p_children[i]->name);
            size_t searchResult = nameUpper.find(substringUpper);
            if (searchResult != String::npos) {
                return {i, (int)searchResult};
            }
        }
        return {-1, -1};
    }

    void TransformNode::eraseChild(size_t _index)
    {
        // Logging::log("%s() entered on node \"%s\".\n", __FUNCTION__, name.c_str());
        if (_index >= p_children.size()) {
            return;
        }
        auto it = p_children.begin() + _index;
        // Logging::log("%s(): Attempting to delete the node named... ", __FUNCTION__);
        // Logging::log("%s.\n", (*it)->name.c_str());
        delete *it;
        // Logging::log("Node deleted!\n");
        p_children.erase(it);
        // Logging::log("Pointer erased!\n");
    }

    int TransformNode::getSiblingIndex() const
    {
        if (p_parent != nullptr) {
            for (int siblingIndex = 0; siblingIndex < p_parent->childCount(); siblingIndex++) {
                if (p_parent->getChild(siblingIndex) == this) {
                    return siblingIndex;
                }
            }
        }
        return -1;
    }

    bool TransformNode::isAncestorOrSelf(const TransformNode* _node) const
    {
        const TransformNode* currentAncestor = this;
        while (currentAncestor != nullptr) {
            if (currentAncestor == _node) {
                return true;
            }
            currentAncestor = currentAncestor->p_parent;
        }
        return false;
    }

    bool TransformNode::hasComponents() const
    {
        if (p_components.size()) {
            return true;
        }
        else {
            return false;
        }
    }

    bool TransformNode::isEmpty() const
    {
        if (p_children.size() == 0 && !hasComponents()) {
            return true;
        }
        else {
            return false;
        }
    }

    void TransformNode::prune()
    {
        // Logging::log("%s() entered on node \"%s\".\n", __FUNCTION__, name.c_str());
        /*Logging::log("Available children:\n");
        for (int childIndex = p_children.size() - 1; childIndex >= 0; childIndex--) {
            Logging::log("\"%s\" at index %d\n", p_children[childIndex]->name.c_str(), childIndex);
        }*/
        for (int childIndex = p_children.size() - 1; childIndex >= 0; childIndex--) {
            p_children[childIndex]->prune();
            if (p_children[childIndex]->isEmpty()) {
                eraseChild(childIndex);
            }
        }
        // Logging::log("Leaving %s() for \"%s\".\n", __FUNCTION__, name.c_str());
    }

    bool TransformNode::removeComponent(TNComponent* _component)
    {
        for (size_t i = 0; i < p_components.size(); i++) {
            if (_component == p_components[i]) {
                delete p_components[i];
                p_components.erase(p_components.begin() + i);
                return true;
            }
        }
        return false;
    }

    void TransformNode::update()
    {
        p_localMatCached = false;
        p_globalMatCached = false;
        p_globalPosCached = false;
        p_globalRotCached = false;
        for (size_t i = 0; i < p_components.size(); i++) {
            p_components[i]->update();
        }
        for (size_t i = 0; i < p_children.size(); i++) {
            p_children[i]->update();
        }
    }

/*

    void TransformNode::serializeNode(std::vector<Transform>& _targetVector, int _parentIndex) const
    {
        int currentIndex = _targetVector.size();
        _targetVector.push_back({
            name, 
            getPosition(),
            getRotation(),
            _parentIndex,
            modelIndex});
        
        for (int childIndex = 0; childIndex < p_children.size(); childIndex++) {
            p_children[childIndex]->serializeNode(_targetVector, currentIndex);
        }
    }

    TransformNode* TransformNode::rebuildHierarchy(const std::vector<Transform>& _vector)
    {
        TransformNode* root = new TransformNode();
        root->name = rootNameDefault;

        // This vector is basically just scaffolding:
        std::vector<TransformNode*> nodes(_vector.size());
        for (int nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++) {
            nodes[nodeIndex] = new TransformNode();
            nodes[nodeIndex]->name = _vector[nodeIndex].name;
            nodes[nodeIndex]->p_position = _vector[nodeIndex].position;
            nodes[nodeIndex]->p_rotation = _vector[nodeIndex].rotation;
            nodes[nodeIndex]->modelIndex = _vector[nodeIndex].modelIndex;
            if (_vector[nodeIndex].parentIndex == -1 || _vector[nodeIndex].parentIndex >= nodeIndex) {
                nodes[nodeIndex]->setParent(root);
            }
            else {
                nodes[nodeIndex]->setParent(nodes[_vector[nodeIndex].parentIndex]);
            }
        }

        return root;
    }

    std::vector<Transform> TransformNode::serializeToFlatList(const TransformNode* _rootNode)
    {
        std::vector<Transform> result;

        if (_rootNode != nullptr) {
            for (int rootChildIndex = 0; rootChildIndex < _rootNode->childCount(); rootChildIndex++) {
                _rootNode->getChild(rootChildIndex)->serializeNode(result, -1);
            }
        }

        return result;
    }
*/
}