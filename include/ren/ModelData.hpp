#pragma once
#include <stdint.h>
#include <glm/vec4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>

/// @file 
/// @brief A collection of data formats involved in model, texture and 
///        collision data conversion.
/// @details These definitions use the glm header-only library.

namespace ren
{
    /// @typedef 
    typedef glm::vec2 Vector2;

    /// @typedef 
    typedef glm::vec3 Vector3;

    /// @typedef 
    typedef glm::vec4 Vector4;

    /// @typedef 
    typedef glm::quat Quaternion;

    /// @typedef 
    typedef Vector4 Sphere;

    struct BoundingObject
    {
        Sphere sphere;
        Vector3 min, max;
    };

    struct CenteredBoundingBox
    {
        glm::vec3 center;
        glm::vec3 dimensions;
    };

    /// @brief Job types used to request certain kinds of data.
    enum MDType : int32_t
    {
        MD_RWMODEL = 0,
        MD_3DMODEL = 0,
        MD_RWTEXSET = 1,
        MD_TEXSET = 1,
        MD_GTACOLLISION = 2,
        MD_COLLISION = 2
    };

    /// @brief Base class for any data that RWImport may provide. 
    /// @details This class is used internally by the Loadthread class to store results. Unless you tinker with the 
    ///          internals, you will not encounter it.
    class MDataContainer
    {
    public:
        MDataContainer() = delete;
        MDataContainer(MDType _type) : type(_type) {};
        virtual ~MDataContainer() {}

        const MDType type;
    };

    /// @brief Internally used struct for storing material properties.
    struct MaterialDescriptor
    {
        MaterialDescriptor() = default;
        MaterialDescriptor(const MaterialDescriptor&) = default;
        MaterialDescriptor(MaterialDescriptor&&) = default;

        std::string textureName;
        uint32_t isTextured;
        Vector3 color;
        float ambient;
        float specular;
        float diffuse;
    };

    /// @brief Blittable version of MaterialDescriptor. 
    /// @details <b>Important:</b> Note that the C strings are only valid as long 
    ///          as your handle is alive! This is the struct you will encounter even if you're just using 
    ///          the dynamic library. 
    struct MaterialDescriptor_blittable
    {
        MaterialDescriptor_blittable() = default;
        MaterialDescriptor_blittable(const MaterialDescriptor_blittable& _other) = default;

        MaterialDescriptor_blittable(const MaterialDescriptor& _md) noexcept :
            textureName(_md.textureName.c_str()),
            isTextured(_md.isTextured),
            color(_md.color),
            ambient(_md.ambient),
            specular(_md.specular),
            diffuse(_md.diffuse),
            padding(0) {}

        MaterialDescriptor_blittable& operator=(const MaterialDescriptor_blittable&) = default;

        MaterialDescriptor_blittable& operator=(const MaterialDescriptor& _md) {
            textureName = _md.textureName.c_str();
            isTextured = _md.isTextured;
            color = _md.color;
            ambient = _md.ambient;
            specular = _md.specular;
            diffuse = _md.diffuse;
            return *this;
        }

        const char* textureName;
        uint32_t isTextured;
        Vector3 color;
        float ambient;
        float specular;
        float diffuse;
        int32_t padding;
    };

    /// @brief Associates triangle index ranges with material indices.
    struct SubmeshDescriptor
	{
		SubmeshDescriptor() : trianglesStart(0), trianglesCount(0), materialIndex(0) {}
		SubmeshDescriptor(uint32_t _trianglesStart, uint32_t _trianglesCount, uint32_t _materialIndex) :
			trianglesStart(_trianglesStart), trianglesCount(_trianglesCount), materialIndex(_materialIndex) {}

		uint32_t trianglesStart;
		uint32_t trianglesCount;
		uint32_t materialIndex;
	};

    template <typename T>
    struct Triangle
    {
        Triangle() : indices() {}
        Triangle(T _index0, T _index1, T _index2) {
            indices[0] = _index0;
            indices[1] = _index1;
            indices[2] = _index2;
        }

        /// @brief Degenerate = do at least two of the indices refer to the same vertex?
        bool isDegenerate() const {
            return indices[0] == indices[1] || indices[1] == indices[2] || indices[2] == indices[0];
        }

        /// @brief Swaps two indices.
        void flip() {
            T buf2 = indices[2];
            indices[2] = indices[1];
            indices[1] = buf2;
        }

        /// @brief Returns a new triangle with two indices swapped. 
        Triangle flipped() const {
            return Triangle(indices[0], indices[2], indices[1]);
        }

        T indices[3];
    };

    /// @typedef
    typedef Triangle<uint16_t> ShortTriangle;

    /// @typedef
    typedef Triangle<uint32_t> LongTriangle;

    struct SurfaceContainer
	{
        SurfaceContainer() = default;
		SurfaceContainer(const std::vector<ShortTriangle>& _triangles, const std::vector<SubmeshDescriptor>& _submeshes) :
			triangles(_triangles), submeshes(_submeshes) {}
		SurfaceContainer(std::vector<ShortTriangle>&& _triangles, std::vector<SubmeshDescriptor>&& _submeshes) noexcept :
			triangles(std::move(_triangles)), submeshes(std::move(_submeshes)) {}
		SurfaceContainer(size_t _trianglesCount, size_t _submeshCount) : triangles(_trianglesCount), submeshes(_submeshCount) {}

		std::vector<ShortTriangle> triangles;
		std::vector<SubmeshDescriptor> submeshes;
	};

    struct MeshData
    {
        MeshData() = default;
        MeshData(const MeshData&) = default;

        MeshData(MeshData&& _other) noexcept :
            positions(std::move(_other.positions)),
            UVs(std::move(_other.UVs)),
            normals(std::move(_other.normals)),
            colors(std::move(_other.colors)),
            triangles(std::move(_other.triangles)),
            materials(std::move(_other.materials)),
            materials_blittable(materials.size()),
            submeshes(std::move(_other.submeshes)),
            boundingSphere(_other.boundingSphere) 
        {
            for (size_t i = 0; i < materials.size(); i++) {
                materials_blittable[i] = materials[i];
            }
        }

        MeshData& operator=(const MeshData&) = default;

        std::vector<Vector3> positions;
        std::vector<Vector2> UVs;
        std::vector<Vector3> normals;
        std::vector<Vector4> colors; // changed 2025-10-08 for completeness
        std::vector<ShortTriangle> triangles;
        std::vector<MaterialDescriptor> materials;
        std::vector<MaterialDescriptor_blittable> materials_blittable;
        std::vector<SubmeshDescriptor> submeshes;
        Sphere boundingSphere;
    };

    /// @brief Serialized form of the transform of a graph node.
    struct Transform
    {
        std::string name;
        Vector3 position;
        Quaternion rotation;
        int32_t parentIndex = -1;
        int32_t modelIndex = -1;
    };

    /// @brief Blittable version of Transform. 
    /// @details <b>Important:</b> Note that any C strings are only valid as long 
    ///          as your handle is still alive! Parent indices cannot refer forward, i.e. parents
    ///          <b>must</b> appear before their children in the list. 
    struct Transform_blittable
    {
        Transform_blittable() = default;
        Transform_blittable(const Transform_blittable&) = default;

        Transform_blittable(const Transform& _other) :
            name(_other.name.c_str()), 
            position(_other.position),
            rotation(_other.rotation),
            parentIndex(_other.parentIndex),
            modelIndex(_other.modelIndex),
            padding(0) {}

        /// @brief <b>Important:</b> Note that this pointer is only 
	    ///        valid as long as the respective handle is alive.
        const char* name;
        Vector3 position;
        Quaternion rotation;
        int32_t parentIndex;
        
        /// @brief Refers to a MeshData instance in the vector/array.
        int32_t modelIndex;

        int32_t padding;
    };

    /// @brief A "Meshmodel" is a multimesh object with a transform hierarchy. 
    ///        In the context of GTA and RenderWare, this is really the memory equivalent of 
    ///        a DFF file.
    class MeshmodelData : public MDataContainer
    {
    public:
        MeshmodelData() : MDataContainer(MD_RWMODEL) {}
        ~MeshmodelData() {}

        std::vector<MeshData> meshes;
        std::vector<Transform> transforms;
        std::vector<Transform_blittable> transforms_blittable;  // NEU!!!!!
    };

    enum MDTextureType : int32_t
    {
        TYPE_RGB =                      0,
        TYPE_RGBA =                     1,
        TYPE_BGR =                      2,
        TYPE_BGRA =                     3,
        TYPE_DXT1 =                     4,
        TYPE_DXT3 =                     5,
        TYPE_DXT5 =                     6
    };

    struct TextureData
    {
        MDTextureType type;
        uint32_t width;
        uint32_t height;

        /// @brief Raw data to be interpreted according to its MDTextureType.
        std::vector<char> data;
    };

    /// @brief A "texture set" is a multi texture container. In the context of GTA and RenderWare, 
    ///        this is really the memory equivalent of a TXD file.
    class TextureSetData : public MDataContainer
    {
    public:
        TextureSetData() : MDataContainer(MD_RWTEXSET) {} 
        TextureSetData(TextureSetData&& _other) noexcept : 
            MDataContainer(MD_RWTEXSET), 
            textures(std::move(_other.textures)), 
            names(std::move(_other.names)) {}

        ~TextureSetData() {}

        void addTexture(TextureData&& _td, const std::string& _name) {
            textures.push_back(std::move(_td));
            names.push_back(_name);
        }

        std::vector<TextureData> textures;
        std::vector<std::string> names;
    };

    struct CollMeshData
    {
        std::vector<Vector3> positions;
        std::vector<ShortTriangle> triangles;
        int32_t material;
    };

    struct CollBox
    {
        Vector3 center;
        Vector3 dimensions;
        Quaternion rotation;
        int32_t material;
    };

    struct CollSphere
    {
        Vector3 center;
        float radius;
        int32_t material;
    };

    /// @brief A single collision object.
    /// @details I decided to go with multiple collision meshes for one object in order 
    ///          to allow for multiple collision materials in engines such as Unity3D.
    ///          Practically, if a GTA collision mesh is converted, one mesh is generated 
    ///          per material.
    class CollisionData : public MDataContainer
    {
    public:
        CollisionData() : MDataContainer(MD_GTACOLLISION) {}
        ~CollisionData() = default;

    public:
        std::string name;
        std::vector<CollMeshData> meshes;
        std::vector<CollBox> boxes;
        std::vector<CollSphere> spheres;
        BoundingObject bounds;
    };
}