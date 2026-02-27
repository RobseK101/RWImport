#pragma once
#include <ren/Utility.hpp>

// provisionally, glm will be used immediately
#include <glm/glm.hpp>

/// @file
/// @brief This file contains structs and definitions needed to inspect the collision
///        file formats found in the GTA games.

namespace gta3d::COLL
{
    /// @typedef
    typedef glm::vec3 TVector;
    
    struct TBoundsV1
    {
        float radius;
        TVector center;
        TVector min, max;
    };

    struct TBoundsV2
    {
        TVector min, max;
        TVector center;
        float radius;
    };

    struct TSurface
    {
        uint8_t material, flags, brightness, light;
    };

    struct TSphereV1
    {
        float radius;
        TVector center;
        TSurface surface;
    };

    struct TSphereV2
    {
        TVector center;
        float radius;
        TSurface surface;
    };

    struct TBox
    {
        TVector min, max;
        TSurface surface;
    };

    struct TFaceGroup
    {
        TVector min, max;
        uint16_t startFace, endFace;
    };

    /// @typedef
    typedef TVector TVertexV1;

    struct TVertexV2
    {
        /// @brief fixed point numbers to float: divide by 128.
        int16_t x, y, z;

        TVertexV2() = default;

        TVertexV2(TVertexV1 _floatVertex) : 
            x(_floatVertex.x * 128.0f), y(_floatVertex.y * 128.0f), z(_floatVertex.z * 128.0f) {}

        operator TVertexV1() const {
            return {x / 128.0f, y / 128.0f, z / 128.0f};
        }
    };

    struct TFaceV1
    {
        uint32_t indices[3];
        TSurface surface;
    };

    struct TFaceV2
    {
        uint16_t indices[3];
        uint8_t material, light;
    };
    
    /// @name FourCCs associated with GTA collision files
    /// @{
    constexpr ren::FourCC fourCC_v1 = ren::makeFourCC("COLL");
    constexpr ren::FourCC fourCC_v2 = ren::makeFourCC("COL2");
    constexpr ren::FourCC fourCC_v3 = ren::makeFourCC("COL3");
    constexpr ren::FourCC fourCC_v4 = ren::makeFourCC("COL4");
    constexpr ren::FourCC threeCC_col = ren::makeFourCC("COL\0");
    constexpr ren::FourCC threeCC_mask = 0x00FFFFFF;
    /// @}

    struct ColHeader
    {
        ren::FourCC fourCC;
        uint32_t dataSize;

        /// @brief Size is counted from here!
        char name[22];
        int16_t modelId;
    };

    constexpr size_t v1DataOversize = sizeof(ColHeader::fourCC) + sizeof(ColHeader::dataSize);
    constexpr size_t v2BaseOffset = sizeof(ColHeader::fourCC);

    struct HeaderExtensionV2
    {
        uint16_t sphereCount;
        uint16_t boxCount;
        uint16_t triangleCount;
        uint16_t lineCount;
        uint32_t flags;
        uint32_t spheresOffset;
        uint32_t boxesOffset;
        uint32_t linesOffset;
        uint32_t verticesOffset;
        uint32_t trianglesOffset;
        uint32_t trianglePlanesOffset; // ???
    };

    struct HeaderExtensionV3
    {
        uint32_t shadowmeshTriangleCount;
        uint32_t shadowmeshVerticesOffset;
        uint32_t shadowmeshTrainglesOffset;

    };

    struct HeaderExtensionV4
    {
        uint32_t unknown;
    };
}