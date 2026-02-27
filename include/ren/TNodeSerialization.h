#pragma once
#include <ren/TransformNode.h>
#include <ren/ModelData.hpp>
#include <ren/TncMeshIndex.h>
#include <vector>

namespace ren
{
    void serializeTNode(const TransformNode* _node, std::vector<Transform>& _targetVector, int _parentIndex);
    std::vector<Transform> serializeTNodeToTransformList(const TransformNode* _rootNode);
    TransformNode* rebuildTNodeHierarchy(const std::vector<Transform>& _vector);
}