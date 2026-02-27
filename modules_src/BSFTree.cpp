#include "rw/BSFTree.h"
#include <ren/Logging.h>
#include <sstream>
#include <ren/ImageFormats.hpp>

namespace rw
{    
    RWChunk::RWChunk(SectionID _sectionID, uint32_t _libraryID, RWChunk* _parent) : 
        p_sectionID(_sectionID), p_libID(_libraryID), p_parent(_parent) 
        {
            ren::Logging::log("%s(): Create section \"%s\"...\n", __FUNCTION__, rw::sectionName(_sectionID));
        }

    RWChunk::~RWChunk() {}

    SectionID RWChunk::sectionID() const
    {
        return p_sectionID;
    }

    const char* RWChunk::sectionName() const
    {
        return rw::sectionName(p_sectionID);
    }

    const char* RWChunk::sectionVendor() const
    {
        return rw::sectionVendor(p_sectionID);
    }

    uint32_t RWChunk::libID() const
    {
        return p_libID;
    }

    uint32_t RWChunk::versionNumber() const
    {
        return libraryIDUnpackVersion(p_libID);
    }

    uint32_t RWChunk::buildNumber() const
    {
        return libraryIDUnpackBuild(p_libID);
    }

    bool RWChunk::isContainer() const
    {
        return isContainer(p_sectionID);
    }

    RWChunk *RWChunk::parent() const
    {
        return p_parent;
    }

    // The interpretation of this is easy: if this returns true, next thing to expect after 
    // the chunk header is another chunk header, if not, it's to be interpreted as pure data.
    bool RWChunk::isContainer(SectionID _id) 
    {
        switch (_id) {
            case RW_CLUMP:
            case RW_FRAMELIST:
            case RW_GEOMETRYLIST:
            case RW_ATOMIC:
            case RW_EXTENSION:
            case RW_GEOMETRY:
            case RW_MATERIALLIST:
            case RW_MATERIAL:
            case RW_TEXTURE:
            case RW_TEXTUREDICTIONARY:
            case RW_RASTER:
            case REN_ID_FILE:
            {
                return true;
            }
            default:
            {
                return false;
            }
        }
    }

    std::string RWChunk::frameToStdString(const char *_rgFrame, size_t _rwSize)
    {
        return std::string(_rgFrame, _rwSize);
    }

    std::string RWChunk::rwStrToStdStr(const char *_rwString, size_t _rwSize)
    {
        size_t stringLength =  std::strlen(_rwString);
        return std::string(_rwString, stringLength);

        // Turns out a string initialized with several zeros at the end is a different 
        // string than one with just one!!!
    }

    CClumpStruct RWChunk::toClumpStruct(const char *_rwStruct, size_t _rwSize)
    {
        CClumpStruct result = {};
        if (_rwSize >= 4) {
            if (_rwSize == sizeof(CClumpStruct)) {
                std::memcpy(&result, _rwStruct, sizeof(result));
            }
            else {
                std::memcpy(&result, _rwStruct, sizeof(result.atomicCount));
            }
        }
        return result;
    }

    std::vector<CFrameListStructEntry> RWChunk::toFrameVector(const char *_rwStruct, size_t _rwSize)
    {
        CFrameListStructHeader header;
        if (_rwSize < 4) {
            ren::throwException<std::runtime_error>("%s(): Invalid Frame List Struct.", __FUNCTION__);
        }
        std::memcpy(&header, _rwStruct, sizeof(header));
        size_t arraySize = _rwSize - sizeof(CFrameListStructHeader);
        if (arraySize != header.frameCount * sizeof(CFrameListStructEntry)) { 
            ren::throwException<std::runtime_error>("%s(): Invalid Frame List Struct.", __FUNCTION__);
        }
        std::vector<CFrameListStructEntry> result(header.frameCount);
        std::memcpy(result.data(), _rwStruct + sizeof(CFrameListStructHeader), arraySize);
        return result;
    }

    CAtomicStruct RWChunk::toAtomicStruct(const char* _rwStruct, size_t _rwSize)
    {
        if (_rwSize != sizeof(CAtomicStruct)) {
            ren::throwException<std::runtime_error>("%s(): Invalid Atomic Struct.", __FUNCTION__);
        }
        CAtomicStruct result;
        std::memcpy(&result, _rwStruct, sizeof(result));
        return result;
    }

    std::vector<MaterialIndex> RWChunk::toMaterialListStruct(const char* _rwStruct, size_t _rwSize)
    {
        if (_rwSize < sizeof(CMaterialListStructHeader)) {
            ren::throwException<std::runtime_error>("%s(): Invalid Material List Struct.", __FUNCTION__);
        }
        CMaterialListStructHeader structHeader;
        std::memcpy(&structHeader, _rwStruct, sizeof(structHeader));
        size_t arraySize = _rwSize - sizeof(CMaterialListStructHeader);
        if (arraySize != structHeader.materialCount * sizeof(MaterialIndex)) {
            ren::throwException<std::runtime_error>("%s(): Invalid Material List Struct.", __FUNCTION__);
        }
        std::vector<MaterialIndex> result(structHeader.materialCount);
        std::memcpy(result.data(), _rwStruct + sizeof(CMaterialListStructHeader), arraySize);
        return result;
    }

    CMaterialStruct RWChunk::toMaterialStruct(const char* _rwStruct, size_t _rwSize)
    {
        CMaterialStruct result = { 0, 0, 0, 0, 1.0f, 1.0f, 1.0f };
        if (_rwSize == sizeof(CMaterialStruct)) {
            std::memcpy(&result, _rwStruct, sizeof(CMaterialStruct));
        }
        else if (_rwSize == sizeof(CMaterialStructOld)) {
            std::memcpy(&result, _rwStruct, sizeof(CMaterialStructOld));
        }
        else {
            ren::throwException<std::runtime_error>("%s(): Invalid Material Struct.", __FUNCTION__);
        }
        return result;
    }

    CTextureStruct RWChunk::toTextureStruct(const char* _rwStruct, size_t _rwSize)
    {
        if (_rwSize != sizeof(CTextureStruct)) {
            ren::throwException<std::runtime_error>("%s(): Invalid Texture Struct.", __FUNCTION__);
        }
        CTextureStruct result;
        std::memcpy(&result, _rwStruct, sizeof(result));
        return result;
    }

    GeometryStructResult RWChunk::toGeometryStruct(const char* _rwStruct, size_t _rwSize, uint32_t _libID)
    {
        uint32_t version = libraryIDUnpackVersion(_libID);
        size_t currentOffset = 0;
        size_t requiredSize = (version < 0x34000) ? sizeof(CGeometryStructHeader) : 
            sizeof(CGeometryStructHeaderNew);
        if (_rwSize < requiredSize) {
            ren::throwException<std::runtime_error>("%s(): Invalid Geometry Struct.", __FUNCTION__);
        }
        CGeometryStructHeader structHeader;
        if (version < 0x34000) {
            std::memcpy(&structHeader, _rwStruct, sizeof(CGeometryStructHeader));
        }
        else {
            std::memcpy(&structHeader, _rwStruct, sizeof(CGeometryStructHeaderNew));
            structHeader.ambient = 1.0f;
            structHeader.specular = 1.0f;
            structHeader.diffuse = 1.0f;
        }
        currentOffset += requiredSize;
        
        
        /* Apperently, the only place where that matters is the Bin Mesh PLG, but there it is stated 
         * again explicitly.
        if (structHeader.format.rpGEOMETRYTRISTRIP) {
            ren::throwException<std::runtime_error>("%s(): Triangle strip is unsupported!", __FUNCTION__);
            // implement later, seems fairly easy
        }*/

        if (structHeader.format.rpGEOMETRYNATIVE) {
            ren::throwException<std::runtime_error>("%s(): Native geometry is unsupported!", __FUNCTION__);
        }
        if (structHeader.morphTargetCount != 1) {
            ren::throwException<std::runtime_error>("%s(): Morph target count must be 1.", __FUNCTION__);
        }
        GeometryStructResult result;
        result.flags = structHeader.format;
        std::memcpy(&result.legacySurfaceParams, &structHeader.ambient, sizeof(result.legacySurfaceParams));
        if (structHeader.format.rpGEOMETRYPRELIT) {
            requiredSize = structHeader.vertexCount * sizeof(RwRGBA);
            result.prelitColors.resize(structHeader.vertexCount);
            std::memcpy(result.prelitColors.data(), _rwStruct + currentOffset, requiredSize);
            currentOffset += requiredSize;
        }
        ren::Logging::log("Prelit vertices:\n");
        for (size_t index = 0; index < result.prelitColors.size(); index++) {
            int cvalue = result.prelitColors[index];
            ren::Logging::log("%d %d %d %d\n", cvalue & 0xFF, (cvalue >> 8) & 0xFF, (cvalue >> 16) & 0xFF, (cvalue >> 24) & 0xFF);
        }

        int textureSetCount = getTextureSetCount(structHeader.format);
        if (textureSetCount) {
            size_t texCoordArraySize = structHeader.vertexCount * sizeof(RwTexCoords);
            requiredSize = texCoordArraySize * textureSetCount;
            result.uvs.resize(structHeader.vertexCount);
            std::memcpy(result.uvs.data(), _rwStruct + currentOffset, texCoordArraySize);
            currentOffset += requiredSize;
        }
        result.triangles.resize(structHeader.triangleCount);
        requiredSize = structHeader.triangleCount * sizeof(RpTriangle);
        std::memcpy(result.triangles.data(), _rwStruct + currentOffset, requiredSize);
        currentOffset += requiredSize;
        CGeometryStructVertexHeader vertexArrayHeader;
        std::memcpy(&vertexArrayHeader, _rwStruct + currentOffset, sizeof(vertexArrayHeader));
        currentOffset += sizeof(CGeometryStructVertexHeader);

        ren::Logging::log("%s(): VertexHeader: %f %f %f %f, %d, %d.\n", __FUNCTION__, 
            vertexArrayHeader.boundingSphere.center.x, vertexArrayHeader.boundingSphere.center.y,
            vertexArrayHeader.boundingSphere.center.z, vertexArrayHeader.boundingSphere.radius,
            vertexArrayHeader.hasVertices, vertexArrayHeader.hasNormals);

        if (vertexArrayHeader.hasVertices) {
            requiredSize = structHeader.vertexCount * sizeof(RwV3d);
            result.positions.resize(structHeader.vertexCount);
            std::memcpy(result.positions.data(), _rwStruct + currentOffset, requiredSize);
            currentOffset += requiredSize;
        }
        if (vertexArrayHeader.hasNormals) {
            // requiredSize stays the same
            result.normals.resize(structHeader.vertexCount);
            std::memcpy(result.normals.data(), _rwStruct + currentOffset, requiredSize);
            currentOffset += requiredSize;
        }
        return result;
    }

    BinMeshPLGResult RWChunk::toBinMeshPLG(const char *_rwStruct, size_t _rwSize)
    {
        size_t currentOffset = 0;
        CBinMeshStructHeader structHeader;
        if (_rwSize < sizeof(CBinMeshStructHeader)) {
            ren::throwException<std::runtime_error>("%s(): Invalid Bin Mesh PLG struct.", 
                __FUNCTION__);
        }
        std::memcpy(&structHeader, _rwStruct, sizeof(structHeader));
        currentOffset += sizeof(CBinMeshStructHeader);
        BinMeshPLGResult result;
        result.isTriangleStrip = structHeader.usesTriangleStrip;
        result.materialIndices.resize(structHeader.meshCount);
        result.meshes.resize(structHeader.meshCount);
        for (size_t meshIndex = 0; meshIndex < structHeader.meshCount; meshIndex++) {
            size_t requiredSize = sizeof(CBinMeshStructEntryHeader);
            if (_rwSize < currentOffset + requiredSize) {
                ren::throwException<std::runtime_error>("%s(): Invalid Bin Mesh PLG struct.", 
                    __FUNCTION__);
            }
            CBinMeshStructEntryHeader tableHeader;
            std::memcpy(&tableHeader, _rwStruct + currentOffset, sizeof(tableHeader));
            currentOffset += sizeof(CBinMeshStructEntryHeader);
            result.materialIndices[meshIndex] = tableHeader.materialIndex;
            requiredSize = tableHeader.indexCount * sizeof(uint32_t);
            if (_rwSize < currentOffset + requiredSize) {
                ren::throwException<std::runtime_error>("%s(): Invalid Bin Mesh PLG struct.", 
                    __FUNCTION__);
            }
            result.meshes[meshIndex].resize(tableHeader.indexCount);
            std::memcpy(result.meshes[meshIndex].data(), _rwStruct + currentOffset, requiredSize);
            currentOffset += requiredSize;
        }
        return result;
    }

    RasterStructResult RWChunk::toRasterStruct(const char* _rwStruct, size_t _rwSize)
    {
        RasterStructResult result;
        size_t currentOffset = 0;
        size_t requiredSize = sizeof(result.header);
        if (_rwSize < requiredSize) {
            ren::throwException<std::runtime_error>("%s(): Invalid Raster struct.", 
                __FUNCTION__);
        }
        std::memcpy(&result.header, _rwStruct, sizeof(result.header));
        currentOffset += requiredSize;
        // if palette exists, copy it
        if (result.header.rasterFormat.format.extendedFormat & RASTEREXT_PAL8) {
            result.paletteData.resize(256);
            requiredSize = 256 * sizeof(uint32_t);
            if (_rwSize - currentOffset < requiredSize) {
                ren::throwException<std::runtime_error>("%s(): Invalid Raster struct in \"%s\" (Not enough space for the announced palette [256 entries]).", 
                    __FUNCTION__, result.header.textureFormat.name);
            }
            std::memcpy(result.paletteData.data(), _rwStruct + currentOffset, requiredSize);
            currentOffset += requiredSize;
        }
        else if (result.header.rasterFormat.format.extendedFormat & RASTEREXT_PAL4) {
            result.paletteData.resize(16);
            requiredSize = 16 * sizeof(uint32_t);
            if (_rwSize - currentOffset < requiredSize) {
                ren::throwException<std::runtime_error>("%s(): Invalid Raster struct in \"%s\" (Not enough space for the announced palette [16 entries]).", 
                    __FUNCTION__, result.header.textureFormat.name);
            }
            std::memcpy(result.paletteData.data(), _rwStruct + currentOffset, requiredSize);
            currentOffset += requiredSize;
        }
        uint32_t imageDataSize;
        requiredSize = sizeof(uint32_t);
        if (_rwSize - currentOffset < requiredSize) {
            ren::throwException<std::runtime_error>("%s(): Invalid Raster struct in \"%s\" (Not enough space for the raster data header).", 
                __FUNCTION__, result.header.textureFormat.name);
        }
        std::memcpy(&imageDataSize, _rwStruct + currentOffset, sizeof(uint32_t));
        currentOffset += requiredSize;
        requiredSize = imageDataSize;
        if (_rwSize - currentOffset < requiredSize) {
            ren::throwException<std::runtime_error>("%s(): Invalid Raster struct in \"%s\" (Not enough space for the announced raster data [%d bytes]).", 
                __FUNCTION__, result.header.textureFormat.name, (int)imageDataSize);
        }
        result.imageData.resize(imageDataSize);
        std::memcpy(result.imageData.data(), _rwStruct + currentOffset, result.imageData.size());

        return result;
    }

    CTextureDictionaryStruct RWChunk::toTextureDictionaryStruct(const char* _rwStruct, size_t _rwSize)
    {
        CTextureDictionaryStruct result;
        if (_rwSize < sizeof(CTextureDictionaryStruct)) {
            ren::throwException<std::runtime_error>("%s(): Invalid Texture Dictionary struct. (Too small)", 
                __FUNCTION__);
        }
        std::memcpy(&result, _rwStruct, sizeof(result));
        return result;
    }

    RWContainer::RWContainer(SectionID _sectionID, uint32_t _libraryID, RWChunk* _parent) : RWChunk(_sectionID, _libraryID, _parent) {}

    RWContainer::~RWContainer() {}

    int RWContainer::findFirstOf(SectionID _sectionID, int _startIndex) const
    {
        size_t childCount = childrenCount();
        for (size_t index = _startIndex; index < childCount; index++) {
            if (operator[](index).sectionID() == _sectionID) return index;
        }
        return -1;
    }

    RWContainerS::RWContainerS(SectionID _sectionID, uint32_t _libraryID, RWChunk *_parent, const char *_rwContainer, size_t _rwSize) : RWContainer(_sectionID, _libraryID, _parent), p_data(_rwSize)
    {
        ren::Logging::log("%s(): Create container of size %d.\n", __FUNCTION__, (int)_rwSize);
        std::memcpy(p_data.data(), _rwContainer, _rwSize);
    }

    RWContainerS::~RWContainerS()
    {
        for (size_t i = 0; i < p_children.size(); i++) {
            delete p_children[i];
        }
    }

    RWChunk& RWContainerS::operator[](size_t _childIndex)
    {
        return *(p_children[_childIndex]);
    }

    const RWChunk& RWContainerS::operator[](size_t _childIndex) const
    {
        return *(p_children[_childIndex]);
    }

    size_t RWContainerS::childrenCount() const
    {
        return p_children.size();
    }

    bool RWContainerS::isFolded() const
    {
        return p_data.size();
    }

    bool RWContainerS::isValid() const
    {
        return !(isFolded() && childrenCount());
    }

    void RWContainerS::unfold()
    {
        if (p_data.size() && p_children.size() == 0) {
            ren::Logging::log("%s(): Unfold container of type \"%s\".\n", __FUNCTION__, rw::sectionName(p_sectionID));
            size_t currentOffset = 0;
            while (currentOffset < p_data.size()) {
                ChunkHeader currentChunk;
                if (currentOffset + sizeof(ChunkHeader) <= p_data.size()) {
                    std::memcpy(&currentChunk, p_data.data() + currentOffset, sizeof(ChunkHeader));
                    currentOffset += sizeof(ChunkHeader);
                }
                else {
                    ren::Logging::log("%s(): Possibly invalid chunk detected (1) at internal offset %d.\n", __FUNCTION__, (int)currentOffset);
                    RWChunk* invalidChildChunk = new RWInvalidS(this, p_data.data() + currentOffset, p_data.size() - currentOffset);
                    p_children.push_back(invalidChildChunk);
                    currentOffset = p_data.size();
                    break;
                }
                if (currentChunk.libIDStamp == RW_PADDING) break;
                if (currentOffset + currentChunk.size <= p_data.size()) {
                    RWChunk* childChunk = nullptr;
                    if (isContainer(currentChunk.type)) {
                        childChunk = new RWContainerS(currentChunk.type, currentChunk.libIDStamp, this, p_data.data() + currentOffset, currentChunk.size);
                    }                   
                    else {
                        childChunk = new RWDataS(currentChunk.type, currentChunk.libIDStamp, this, p_data.data() + currentOffset, currentChunk.size);
                    }
                    currentOffset += currentChunk.size;
                    p_children.push_back(childChunk);
                }
                else {
                    ren::Logging::log("%s(): Possibly invalid chunk detected (2) at internal offset %d.\n", __FUNCTION__, (int)currentOffset);
                    RWChunk* invalidChildChunk = new RWInvalidS(this, p_data.data() + currentOffset - sizeof(ChunkHeader), p_data.size() + sizeof(ChunkHeader) - currentOffset);
                    p_children.push_back(invalidChildChunk);
                    currentOffset = p_data.size();
                    break;
                }
            }
            p_data.clear();
            ren::Logging::log("%s(): Unfolding done, children count = %d. Unfolding children...\n", __FUNCTION__, (int)p_children.size());
            for (size_t i = 0; i < p_children.size(); i++) {
                if (p_children[i]->isContainer()) {
                    static_cast<RWContainerS*>(p_children[i])->unfold();
                }
            }
        }
    }

    void RWContainerS::fold()
    {
        if (p_children.size()) {
            ren::Logging::log("%s(): Folding container \"%s\".\n", __FUNCTION__, rw::sectionName(p_sectionID));
            p_data.clear();

            // fold all foldable children
            for (size_t i = 0; i < p_children.size(); i++) {
                RWChunk* currentChild = p_children[i];
                if (currentChild->isContainer()) {
                    static_cast<RWContainerS*>(currentChild)->fold();
                }
            }

            for (size_t i = 0; i < p_children.size(); i++) {
                size_t newDataOffset = p_data.size();
                RWChunk* currentChild = p_children[i];
                ChunkHeader currentHeader = {currentChild->sectionID(), 0, currentChild->libID()};
                if (currentChild->isContainer()) {
                    RWContainerS* child = static_cast<RWContainerS*>(currentChild);
                    currentHeader.size = child->p_data.size();
                    p_data.resize(p_data.size() + sizeof(ChunkHeader) + child->p_data.size());
                    std::memcpy(p_data.data() + newDataOffset, &currentHeader, sizeof(ChunkHeader));
                    newDataOffset += sizeof(ChunkHeader);
                    std::memcpy(p_data.data() + newDataOffset, child->p_data.data(), child->p_data.size());
                }
                else if (currentChild->sectionID() == REN_ID_INVALID) {
                    RWInvalidS* invalidChild = static_cast<RWInvalidS*>(currentChild);
                    p_data.resize(p_data.size() + invalidChild->dataSize());
                    std::memcpy(p_data.data() + newDataOffset, invalidChild->data(), invalidChild->dataSize());
                    // -> i.e. just restore whatever garble was present before
                }
                else {
                    RWDataS* child = static_cast<RWDataS*>(currentChild);
                    currentHeader.size = child->dataSize();
                    p_data.resize(p_data.size() + sizeof(ChunkHeader) + child->dataSize());
                    std::memcpy(p_data.data() + newDataOffset, &currentHeader, sizeof(ChunkHeader));
                    newDataOffset += sizeof(ChunkHeader);
                    std::memcpy(p_data.data() + newDataOffset, child->data(), child->dataSize());
                }
                delete currentChild;
                p_children[i] = nullptr;
            }
            p_children.clear();
        }
        else {
            ren::Logging::log("%s(): Container \"%s\" has no children.\n", __FUNCTION__, rw::sectionName(p_sectionID));
        }
    }

    RWData::RWData(SectionID _sectionID, uint32_t _libraryID, RWChunk* _parent) : RWChunk(_sectionID, _libraryID, _parent) {}

    RWData::~RWData() {} 

    RWDataS::RWDataS(SectionID _sectionID, uint32_t _libraryID, RWChunk* _parent, const char* _rwData, size_t _rwSize) : 
        RWData(_sectionID, _libraryID, _parent), p_data(_rwSize)
    {
        ren::Logging::log("%s(): Create data section of type \"%s\"; size = %d.\n", __FUNCTION__, rw::sectionName(_sectionID), (int)_rwSize);
        std::memcpy(p_data.data(), _rwData, _rwSize);
    }

    RWDataS::~RWDataS() {}

    char &RWDataS::operator[](size_t _index)
    {
        return p_data[_index];
    }

    char *RWDataS::data()
    {
        return p_data.data();
    }

    const char* RWDataS::data() const 
    {
        return p_data.data();
    }

    size_t RWDataS::dataSize() const
    {
        return p_data.size();
    }

    RWInvalid::RWInvalid(SectionID _sectionID, uint32_t _libraryID, RWChunk* _parent) : RWChunk(_sectionID, _libraryID, _parent) {}

    RWInvalid::~RWInvalid() {}

    RWInvalidS::RWInvalidS(RWChunk *_parent, const char *_rwData, size_t _rwSize) : 
        RWInvalid(REN_ID_INVALID, 0, _parent), p_data(_rwSize)
    {
        ren::Logging::log("%s(): Creating invalid section; size = %d.\n", __FUNCTION__, (int)_rwSize);
        std::memcpy(p_data.data(), _rwData, _rwSize);
    }

    RWInvalidS::~RWInvalidS() {}

    char &RWInvalidS::operator[](size_t _index)
    {
        return p_data[_index];
    }

    char* RWInvalidS::data()
    {
        return p_data.data();
    }

    const char* RWInvalidS::data() const 
    {
        return p_data.data();
    }

    size_t RWInvalidS::dataSize() const 
    {
        return p_data.size();
    }
}

std::string rw::report(const RWDataS* _input) 
{
    switch (_input->sectionID()) {
        case RW_STRUCT: {
            break;
        }
        case RW_STRING: {
            return RWChunk::rwStrToStdStr(_input->data(), _input->dataSize());
        }
        case RW_BINMESH_PLG: {
            BinMeshPLGResult result = RWChunk::toBinMeshPLG(
                _input->data(), _input->dataSize());
            return report(result);
        }
        case RG_FRAME: {
            return RWChunk::frameToStdString(_input->data(), _input->dataSize());
        }
        default: {
            return "Unknown data section";
        }
    }
    if (_input->parent() == nullptr) {
        return "Invalid struct (no parent chunk)";
    }
    switch (_input->parent()->sectionID()) {
        case RW_GEOMETRY: {
            GeometryStructResult result = RWChunk::toGeometryStruct(
                _input->data(), _input->dataSize(), _input->libID());
                return report(result);
        }
        default: {
            return "Unknown struct";
        }
    }
}

std::string rw::report(const GeometryStructResult &_input)
{
    std::stringstream outstream;
    outstream << "GeometryStructResult report:\n";
    outstream << "rpGEOMETRYSTRIP: " << (_input.flags.rpGEOMETRYTRISTRIP ? "true" : "false") << std::endl;
    outstream << "rpGEOMETRYPOSITIONS: " << (_input.flags.rpGEOMETRYPOSITIONS ? "true" : "false") << std::endl;
    outstream << "rpGEOMETRYTEXTURED: " << (_input.flags.rpGEOMETRYTEXTURED ? "true" : "false") << std::endl;
    outstream << "rpGEOMETRYPRELIT: " << (_input.flags.rpGEOMETRYPRELIT ? "true" : "false") << std::endl;
    outstream << "rpGEOMETRYNORMALS: " << (_input.flags.rpGEOMETRYNORMALS ? "true" : "false") << std::endl;
    outstream << "rpGEOMETRYLIGHT: " << (_input.flags.rpGEOMETRYLIGHT ? "true" : "false") << std::endl;
    outstream << "rpGEOMETRYMODULATEMATERIALCOLOR: " << (_input.flags.rpGEOMETRYMODULATEMATERIALCOLOR ? "true" : "false") << std::endl;
    outstream << "rpGEOMETRYTEXTURED2: " << (_input.flags.rpGEOMETRYTEXTURED2 ? "true" : "false") << std::endl;
    outstream << "unused1: " << _input.flags.unused1 << std::endl;
    outstream << "textureCoordinateSets: " << _input.flags.textureCoordinateSets << std::endl;
    outstream << "rpGEOMETRYNATIVE: " << (_input.flags.rpGEOMETRYNATIVE ? "true" : "false") << std::endl;
    outstream << "unused2: " << _input.flags.unused2 << std::endl;

    outstream   << "amb: " << _input.legacySurfaceParams.ambient 
                << ", spec: " << _input.legacySurfaceParams.specular 
                << ", diff: " << _input.legacySurfaceParams.diffuse << std::endl;
    
    outstream << "Prelit colors - vertex count: " << _input.prelitColors.size() << std::endl;
    for (size_t i = 0; i < _input.prelitColors.size(); i++) {
        outstream << "Vertex " << i << ": " << _input.prelitColors[i] << '\n';
    }

    outstream << "UVs - vertex count: " << _input.uvs.size() << std::endl;
    for (size_t i = 0; i < _input.uvs.size(); i++) {
        outstream << "Vertex " << i << ": " << _input.uvs[i].x << ',' << _input.uvs[i].y << '\n';
    }

    outstream << "Triangles - count: " << _input.triangles.size() << std::endl;
    for (size_t i = 0; i < _input.triangles.size(); i++) {
        outstream << "Triangle " << i << ": " 
            << _input.triangles[i].vertex1 << ',' << _input.triangles[i].vertex2 << ',' 
            << _input.triangles[i].vertex3 << ',' << _input.triangles[i].materialID << '\n';
    }

    outstream << "Bounding sphere - pos: " << _input.boundingSphere.center.x 
        << ',' << _input.boundingSphere.center.y << ',' << _input.boundingSphere.center.z 
        << "; radius: " << _input.boundingSphere.radius << std::endl;
    
    outstream << "Positions - vertex count: " << _input.positions.size() << std::endl;
    for (size_t i = 0; i < _input.positions.size(); i++) {
        outstream << "Vertex " << i << ": " << _input.positions[i].x 
            << ',' << _input.positions[i].y << ',' << _input.positions[i].z << '\n';
    }

    outstream << "Positions - vertex count: " << _input.normals.size() << std::endl;
    for (size_t i = 0; i < _input.normals.size(); i++) {
        outstream << "Vertex " << i << ": " << _input.normals[i].x 
            << ',' << _input.normals[i].y << ',' << _input.normals[i].z << '\n';
    }

    return outstream.str();
}

std::string rw::report(const BinMeshPLGResult &_input)
{
    std::stringstream outstream;

    outstream   << "Header data:\n"
                << "Triangle strip: " << (_input.isTriangleStrip ? "true" : "false") << '\n' 
                << "Mesh count: " << _input.materialIndices.size() << std::endl;

    outstream << "Material indices - count: " << _input.materialIndices.size() << std::endl;
    for (size_t i = 0; i < _input.materialIndices.size(); i++) {
        outstream << "Material index " << i << ": " << _input.materialIndices[i] << '\n';
    }
    outstream << "Mesh count: " << _input.meshes.size() << std::endl;
    for (size_t i = 0; i < _input.meshes.size(); i++) {
        outstream << "Mesh " << i << '\n';
        for (size_t j = 0; j < _input.meshes[i].size(); j++) {
            outstream << "Index " << j << ": " << _input.meshes[i][j] << '\n';
        }
    }

    return outstream.str();
}