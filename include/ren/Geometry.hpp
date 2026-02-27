#pragma once
#include <cstdint>
#include <glm/vec4.hpp>

namespace ren
{
    class GLTriangle
    {
    public:
        GLTriangle() : indices() {}
        GLTriangle(uint16_t _index0, uint16_t _index1, uint16_t _index2) {
            indices[0] = _index0;
            indices[1] = _index1;
            indices[2] = _index2;
        }

        GLTriangle(unsigned int _index0, unsigned int _index1, unsigned int _index2) {
            indices[0] = _index0;
            indices[1] = _index1;
            indices[2] = _index2;
        }

        bool isDegenerate() const {
            return indices[0] == indices[1] || indices[1] == indices[2] || indices[2] == indices[0];
        }

        uint16_t indices[3];
    };

    typedef glm::vec4 GLSphere;
}