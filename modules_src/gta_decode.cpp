#include <ren/gta_decode.h>
#include <ren/Exceptions.hpp>
#include <gta3d/Collfile.hpp>
#include <vector>
#include <map>
#include <ren/Utility.hpp>
#include <ren/Logging.h>

namespace ren
{
    void flipTriangles(std::vector<ShortTriangle>& _triangles)
    {
        for (size_t i = 0; i < _triangles.size(); i++) {
            ShortTriangle currentTriangle = _triangles[i];
            _triangles[i] = {currentTriangle.indices[0], currentTriangle.indices[2], currentTriangle.indices[1]};
        }
    }

    void swapYZ(std::vector<Vector3>& _vertices)
    {
        for (size_t i = 0; i < _vertices.size(); i++) {
            Vector3 currentVertex = _vertices[i];
            _vertices[i] = {currentVertex.x, currentVertex.z, currentVertex.y};
        }
    }

    inline int32_t gtacollSurfaceV1ToIndex(gta3d::COLL::TSurface _surface)
    {
        // provisionally
        return _surface.material;
    }

    inline int32_t gtacollMaterialV2ToIndex(uint8_t _materialID)
    {
        // provisionally
        return _materialID;
    }

    // Converts COLL v1 (GTA 3 and VC) vertices and triangles into a list of per-material meshes.
    // Each mesh contains a deduplicated position buffer and local triangle indices (i.e. referring to the deduplicated position buffer).
    std::vector<CollMeshData> toCollMeshDataVector(const std::vector<gta3d::COLL::TVertexV1>& _vertices, const std::vector<gta3d::COLL::TFaceV1>& _triangles, ren_process_flags _processFlags) 
    {
        using namespace gta3d::COLL;

        // Group triangles by surface/material ID:
        std::map<int32_t,std::vector<TFaceV1>> sortedTriangles;
        for (size_t i = 0; i < _triangles.size(); i++) {
            TFaceV1 currentTriangle = _triangles[i];
            sortedTriangles[gtacollSurfaceV1ToIndex(currentTriangle.surface)].push_back(currentTriangle);
        }

        // Build a CollMeshData for each material:
        std::vector<CollMeshData> result(sortedTriangles.size());
        size_t currentMeshDataIndex = 0;
        for (auto it_triList = sortedTriangles.begin(); it_triList != sortedTriangles.end(); it_triList++) {
            std::vector<TFaceV1>& currentTriangleList = it_triList->second;
            CollMeshData& currentMeshData = result[currentMeshDataIndex];
            currentMeshData.triangles.resize(currentTriangleList.size());
            currentMeshData.material = it_triList->first;

            // Ensure each mesh has a compact, deduplicated vertex buffer:
            for (size_t triangleIndex = 0; triangleIndex < currentTriangleList.size(); triangleIndex++) {
                TFaceV1 currentTriangle = currentTriangleList[triangleIndex];
                glm::vec3 currentVertices[3] = {_vertices[currentTriangle.indices[0]], _vertices[currentTriangle.indices[1]], _vertices[currentTriangle.indices[2]]};
                ShortTriangle newTriangle = {(uint16_t) - 1, (uint16_t)-1, (uint16_t)-1};
                for (int index = 0; index < 3; index++) {
                    intmax_t foundIndex = ren::findUnsorted(currentMeshData.positions, currentVertices[index]);
                    if (foundIndex >= 0) {
                        newTriangle.indices[index] = foundIndex;
                    }
                    else {
                        size_t newIndex = currentMeshData.positions.size();
                        currentMeshData.positions.push_back(currentVertices[index]);
                        newTriangle.indices[index] = newIndex;
                    }
                }
                currentMeshData.triangles[triangleIndex] = newTriangle;
            }
            // Apply the processing flags. If postprocessing turns out to be too slow, this may 
            // be replaced by an in-place method.
            if (_processFlags & REN_COLLISION_SWAP_YZ) {
                swapYZ(currentMeshData.positions);
            }
            if (_processFlags & REN_COLLISION_FLIP_TRIANGLES) {
                flipTriangles(currentMeshData.triangles);
            }
            currentMeshDataIndex++;
        }
        return result;
    }

    // Converts COLL v2 - v4 (GTA SA) vertices and triangles into a list of per-material meshes.
    // Each mesh contains a deduplicated position buffer and local triangle indices (i.e. referring to the deduplicated position buffer).
    std::vector<CollMeshData> toCollMeshDataVector(const std::vector<gta3d::COLL::TVertexV2>& _vertices, const std::vector<gta3d::COLL::TFaceV2>& _triangles, ren_process_flags _processFlags)
    {
        using namespace gta3d::COLL;

        // Group triangles by surface ID:
        std::map<int32_t,std::vector<TFaceV2>> sortedTriangles;
        for (size_t i = 0; i < _triangles.size(); i++) {
            TFaceV2 currentTriangle = _triangles[i];
            sortedTriangles[gtacollMaterialV2ToIndex(currentTriangle.material)].push_back(currentTriangle);
        }

        // Build a CollMeshData for each material:
        std::vector<CollMeshData> result(sortedTriangles.size());
        size_t currentMeshDataIndex = 0;
        for (auto it_triList = sortedTriangles.begin(); it_triList != sortedTriangles.end(); it_triList++) {
            std::vector<TFaceV2>& currentTriangleList = it_triList->second;
            CollMeshData& currentMeshData = result[currentMeshDataIndex];
            currentMeshData.triangles.resize(currentTriangleList.size());
            currentMeshData.material = it_triList->first;

            // Ensure each mesh has a compact, deduplicated vertex buffer:
            for (size_t triangleIndex = 0; triangleIndex < currentTriangleList.size(); triangleIndex++) {
                TFaceV2 currentTriangle = currentTriangleList[triangleIndex];
                TVector currentVertices[3] = {_vertices[currentTriangle.indices[0]], _vertices[currentTriangle.indices[1]], _vertices[currentTriangle.indices[2]]};
                ShortTriangle newTriangle = { (uint16_t)-1, (uint16_t)-1, (uint16_t)-1};
                for (int index = 0; index < 3; index++) {
                    intmax_t foundIndex = ren::findUnsorted(currentMeshData.positions, currentVertices[index]);
                    if (foundIndex >= 0) {
                        newTriangle.indices[index] = foundIndex;
                    }
                    else {
                        size_t newIndex = currentMeshData.positions.size();
                        currentMeshData.positions.push_back(currentVertices[index]);
                        newTriangle.indices[index] = newIndex;
                    }
                }
                currentMeshData.triangles[triangleIndex] = newTriangle;
            }
            // Apply the processing flags. If postprocessing turns out to be too slow, this may 
            // be replaced by an in-place method.
            if (_processFlags & REN_COLLISION_SWAP_YZ) {
                swapYZ(currentMeshData.positions);
            }
            if (_processFlags & REN_COLLISION_FLIP_TRIANGLES) {
                flipTriangles(currentMeshData.triangles);
            }
            currentMeshDataIndex++;
        }
        return result;
    }

    // Converts an isolated (i.e. single instance) GTA3-VC-SA collision file into an instance of CollisionData
    CollisionData* gtacollToCollisionData(const FileInputHandle* _singleCollfile, ren_process_flags _processFlags)
    {
        Logging::log("Entered " __FUNCTION__ "()...\n");
        if (_singleCollfile == nullptr) {
            throwException<std::runtime_error>("%s(): Invalid file input handle (nullptr).", __FUNCTION__);
        }

        using namespace gta3d::COLL;

        ColHeader header;

        size_t currentOffset = 0;
        if (currentOffset + sizeof(header) > _singleCollfile->filesize()) {
            throwException<std::runtime_error>("%s(): Invalid collision file (no header).", __FUNCTION__);
        }
        _singleCollfile->read(currentOffset, reinterpret_cast<char*>(&header), sizeof(header));
        currentOffset += sizeof(header);
        Logging::log("Header read...\n");

        CollisionData* result = nullptr;

        // Choose the appropriate decoder:
        if (header.fourCC == fourCC_v1) {
            Logging::log("Chose decoder V1...\n");
            result = new CollisionData();
            result->name = header.name;

            TBoundsV1 tBounds;
            if (currentOffset + sizeof(tBounds) > _singleCollfile->filesize()) {
                delete result;
                throwException<std::runtime_error>("%s(): v1 collision file ended prematurely in bounding object section.", __FUNCTION__);
            }
            _singleCollfile->read(currentOffset, reinterpret_cast<char*>(&tBounds), sizeof(tBounds));
            currentOffset += sizeof(tBounds);
            result->bounds.sphere = {tBounds.center, tBounds.radius};
            result->bounds.min = tBounds.min;
            result->bounds.max = tBounds.max;   
            Logging::log("TBounds read...\n");

            // Read and convert the collision sphere array:
            if (currentOffset + sizeof(uint32_t) > _singleCollfile->filesize()) {
                delete result;
                throwException<std::runtime_error>("%s(): v1 collision file ended prematurely before sphere section.", __FUNCTION__);
            }
            uint32_t sphereCount;
            _singleCollfile->read(currentOffset, reinterpret_cast<char*>(&sphereCount), sizeof(sphereCount));
            currentOffset += sizeof(sphereCount);
            if (currentOffset + sphereCount * sizeof(TSphereV1) > _singleCollfile->filesize()) {
                delete result;
                throwException<std::runtime_error>("%s(): v1 collision file ended prematurely in sphere section.", __FUNCTION__);
            }
            std::vector<TSphereV1> spheres(sphereCount);
            _singleCollfile->read(currentOffset, reinterpret_cast<char*>(spheres.data()), sphereCount * sizeof(TSphereV1));
            currentOffset += sphereCount * sizeof(TSphereV1);       
            result->spheres.resize(sphereCount);     
            for (size_t i = 0; i < sphereCount; i++) {
                result->spheres[i] = {spheres[i].center, spheres[i].radius, gtacollSurfaceV1ToIndex(spheres[i].surface)};
            }
            spheres.clear();
            Logging::log("Spheres read and converted...\n");

            // Read the count marker of the "Unknown" array and discard it; if this is nonzero this is a problem
            // because we have no knowledge of the structure of the data contained in it:
            if (currentOffset + sizeof(uint32_t) > _singleCollfile->filesize()) {
                delete result;
                throwException<std::runtime_error>("%s(): v1 collision file ended prematurely before \"Unknown\" section).", __FUNCTION__);
            }
            uint32_t unknownCount;
            _singleCollfile->read(currentOffset, reinterpret_cast<char*>(&unknownCount), sizeof(unknownCount));
            currentOffset += sizeof(unknownCount);
            if (unknownCount != 0) {
                delete result;
                throwException<std::runtime_error>("%s(): Unknown section in v1 collision file appears to contain data.", __FUNCTION__);
            }
            Logging::log("Unknown section skipped...\n");

            // Read and convert the collistion box array:
            if (currentOffset + sizeof(uint32_t) > _singleCollfile->filesize()) {
                delete result;
                throwException<std::runtime_error>("%s(): v1 collision file ended prematurely before box section).", __FUNCTION__);
            }
            uint32_t boxCount;
            _singleCollfile->read(currentOffset, reinterpret_cast<char*>(&boxCount), sizeof(boxCount));
            currentOffset += sizeof(boxCount);
            if (currentOffset + boxCount * sizeof(TBox) > _singleCollfile->filesize()) {
                delete result;
                throwException<std::runtime_error>("%s(): v1 collision file ended prematurely in box section.", __FUNCTION__);
            }
            std::vector<TBox> boxes(boxCount);
            _singleCollfile->read(currentOffset, reinterpret_cast<char*>(boxes.data()), boxCount * sizeof(TBox));
            currentOffset += boxCount * sizeof(TBox);
            result->boxes.resize(boxCount);
            for (size_t i = 0; i < boxCount; i++) {
                result->boxes[i].dimensions = boxes[i].max - boxes[i].min;
                result->boxes[i].center = (boxes[i].min + boxes[i].max) * 0.5f;
                result->boxes[i].rotation = {1.0f, 0.0f, 0.0f, 0.0f}; 
                result->boxes[i].material = gtacollSurfaceV1ToIndex(boxes[i].surface);
            }
            boxes.clear();
            Logging::log("Boxes read and converted...\n");

            // Read the vertex array:
            if (currentOffset + sizeof(uint32_t) > _singleCollfile->filesize()) {
                delete result;
                throwException<std::runtime_error>("%s(): v1 collision file ended prematurely before vertex section.", __FUNCTION__);
            }
            uint32_t vertexCount;
            _singleCollfile->read(currentOffset, reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount));
            currentOffset += sizeof(vertexCount);
            if (currentOffset + vertexCount * sizeof(TVertexV1) > _singleCollfile->filesize()) {
                delete result;
                throwException<std::runtime_error>("%s(): v1 collision file ended prematurely in vertex section.", __FUNCTION__);
            }
            std::vector<TVertexV1> vertices(vertexCount);
            _singleCollfile->read(currentOffset, reinterpret_cast<char*>(vertices.data()), vertexCount * sizeof(TVertexV1));
            currentOffset += vertexCount * sizeof(TVertexV1);
            Logging::log("Vertices read...\n");
            
            // Read the triangle (or "Face") array:
            if (currentOffset + sizeof(uint32_t) > _singleCollfile->filesize()) {
                delete result;
                throwException<std::runtime_error>("%s(): v1 collision file ended prematurely before faces section.", __FUNCTION__);
            }
            uint32_t triangleCount;
            _singleCollfile->read(currentOffset, reinterpret_cast<char*>(&triangleCount), sizeof(triangleCount));
            currentOffset += sizeof(triangleCount);
            if (currentOffset + triangleCount * sizeof(TFaceV1) > _singleCollfile->filesize()) {
                delete result;
                throwException<std::runtime_error>("%s(): v1 collision file ended prematurely in faces section.", __FUNCTION__);
            }
            std::vector<TFaceV1> triangles(triangleCount);
            _singleCollfile->read(currentOffset, reinterpret_cast<char*>(triangles.data()), triangleCount * sizeof(TFaceV1));
            currentOffset += triangleCount * sizeof(TFaceV1);
            Logging::log("Faces read...\n");

            // Convert and assign the collision mesh:
            result->meshes = toCollMeshDataVector(vertices, triangles, _processFlags);
            Logging::log("Data converted...\n");
        }
        else if (header.fourCC == fourCC_v2 || header.fourCC == fourCC_v3 || header.fourCC == fourCC_v4) {
            Logging::log("Chose decoder V2-V4...\n");
            result = new CollisionData();
            result->name = header.name;

            TBoundsV2 tBounds;
            if (currentOffset + sizeof(tBounds) > _singleCollfile->filesize()) {
                delete result;
                throwException<std::runtime_error>("%s(): v2+ collision file ended prematurely in bounding object section.", __FUNCTION__);
            }
            _singleCollfile->read(currentOffset, reinterpret_cast<char*>(&tBounds), sizeof(tBounds));
            currentOffset += sizeof(tBounds);
            result->bounds.sphere = {tBounds.center, tBounds.radius};
            result->bounds.min = tBounds.min;
            result->bounds.max = tBounds.max;            

            HeaderExtensionV2 headerExtension;
            if (currentOffset + sizeof(headerExtension) > _singleCollfile->filesize()) {
                delete result;
                throwException<std::runtime_error>("%s() v2+ collision file ended prematurely in header extension section.", __FUNCTION__);
            }
            _singleCollfile->read(currentOffset, reinterpret_cast<char*>(&headerExtension), sizeof(headerExtension));
            currentOffset += sizeof(headerExtension);

            // For now, face groups and the shadow mesh are ignored.

            // Read and convert the collision sphere array:
            if (v2BaseOffset + headerExtension.spheresOffset + headerExtension.sphereCount * sizeof(TSphereV2) > _singleCollfile->filesize()) {
                delete result;
                throwException<std::runtime_error>("%s(): v2+ collision file ended prematurely before or in spheres section.", __FUNCTION__);
            }
            std::vector<TSphereV2> spheres(headerExtension.sphereCount);
            _singleCollfile->read(v2BaseOffset + headerExtension.spheresOffset, reinterpret_cast<char*>(spheres.data()), headerExtension.sphereCount * sizeof(TSphereV2));
            result->spheres.resize(headerExtension.sphereCount);
            for (size_t i = 0; i < spheres.size(); i++) {
                result->spheres[i] = {spheres[i].center, spheres[i].radius, gtacollSurfaceV1ToIndex(spheres[i].surface)};
            }
            spheres.clear();

            // Read and convert the collision box array:
            if (v2BaseOffset + headerExtension.boxesOffset + headerExtension.boxCount * sizeof(TBox) > _singleCollfile->filesize()) {
                delete result;
                throwException<std::runtime_error>("%s(): v2+ collision file ended prematurely before or in boxes section.", __FUNCTION__);
            }
            std::vector<TBox> boxes(headerExtension.boxCount);
            _singleCollfile->read(v2BaseOffset + headerExtension.boxesOffset, reinterpret_cast<char*>(boxes.data()), headerExtension.boxCount * sizeof(TBox));
            result->boxes.resize(headerExtension.boxCount);
            for (size_t i = 0; i < boxes.size(); i++) {
                result->boxes[i].dimensions = boxes[i].max - boxes[i].min;
                result->boxes[i].center = (boxes[i].min + boxes[i].max) * 0.5f;
                result->boxes[i].rotation = {1.0f, 0.0f, 0.0f, 0.0f}; 
                result->boxes[i].material = gtacollSurfaceV1ToIndex(boxes[i].surface);
            }
            boxes.clear();

            // Read the triangle array:
            if (v2BaseOffset + headerExtension.trianglesOffset + headerExtension.triangleCount * sizeof(TFaceV2) > _singleCollfile->filesize()) {
                delete result;
                throwException<std::runtime_error>("%s(): v2+ collision file ended prematurely before or in triangles section.", __FUNCTION__);
            }
            std::vector<TFaceV2> triangles(headerExtension.triangleCount);
            _singleCollfile->read(v2BaseOffset + headerExtension.trianglesOffset, reinterpret_cast<char*>(triangles.data()), headerExtension.triangleCount * sizeof(TFaceV2));

            // Read the vertex array:
            size_t vertexCount = (headerExtension.trianglesOffset - headerExtension.verticesOffset) / sizeof(TVertexV2);
            std::vector<TVertexV2> vertices(vertexCount);

            // Convert and assign the collision mesh:
            result->meshes = toCollMeshDataVector(vertices, triangles, _processFlags);
        }
        else {
            uint64_t fourCCstr = 0xFFFFFFFF & header.fourCC;
            throwException<std::runtime_error>("%s(): Invalid collision file (Invalid FourCC \"%s\").", __FUNCTION__, &fourCCstr);
        }
        
        Logging::log("Return result...\n");
        return result;
    }
}