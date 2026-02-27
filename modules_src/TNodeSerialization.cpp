#include <ren/TNodeSerialization.h>

namespace ren
{
    void serializeTNode(const TransformNode* _node, std::vector<Transform>& _targetVector, int _parentIndex)
    {
        if (_node == nullptr) {
            return;
        }

        int currentIndex = _targetVector.size();
        int meshIndex = -1;
        TncMeshIndex* compMeshIndex = _node->getComponent<TncMeshIndex>();
        if (compMeshIndex != nullptr) {
            meshIndex = compMeshIndex->meshIndex;
        }
        _targetVector.push_back({
            _node->name,
            _node->getPosition(),
            _node->getRotation(),
            _parentIndex,
            meshIndex
        });

        for (int childIndex = 0; childIndex < _node->childCount(); childIndex++) {
            serializeTNode(_node->getChild(childIndex), _targetVector, currentIndex);
        }
    }

    std::vector<Transform> serializeTNodeToTransformList(const TransformNode* _rootNode)
    {
        std::vector<Transform> result;
        if (_rootNode != nullptr) {
            for (int rootChildIndex = 0; rootChildIndex < _rootNode->childCount(); rootChildIndex++) {
                serializeTNode(_rootNode->getChild(rootChildIndex), result, -1);
            }
        }
        return result;
    }

    TransformNode* rebuildTNodeHierarchy(const std::vector<Transform>& _vector)
    {
        TransformNode* root = new TransformNode();
        root->name = TransformNode::rootNameDefault;

        // This vector is basically just scaffolding:
        std::vector<TransformNode*> nodes(_vector.size());
        for (int nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++) {
            nodes[nodeIndex] = new TransformNode();
            nodes[nodeIndex]->name = _vector[nodeIndex].name;
            nodes[nodeIndex]->setPosition(_vector[nodeIndex].position);
            nodes[nodeIndex]->setRotation(_vector[nodeIndex].rotation);
            if (_vector[nodeIndex].modelIndex >= 0) {
                TncMeshIndex* meshIndexComponent = nodes[nodeIndex]->addComponent<TncMeshIndex>();
                meshIndexComponent->meshIndex = _vector[nodeIndex].modelIndex;
            }
            if (_vector[nodeIndex].parentIndex == -1 || _vector[nodeIndex].parentIndex >= nodeIndex) {
                nodes[nodeIndex]->setParent(root);
            }
            else {
                nodes[nodeIndex]->setParent(nodes[_vector[nodeIndex].parentIndex]);
            }
        }
        return root;
    }
}