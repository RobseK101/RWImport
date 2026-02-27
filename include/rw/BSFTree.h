#pragma once

#include <rw/BinaryStreamFile.hpp>
#include <string>

    /// @file
    /// @brief This header file contains classes implementing RenderWare Binary Stream File nodes. 
    ///        See the source code for a description of the inheritance tree.

    /* This header file contains classes implementing RenderWare Binary Stream File nodes. 
     * The following inheritance structure is used: 
     * RWChunk ~~~~~~~~~~~~~~~~~~~ Implements the section ID and the library ID. 
     * |                         ~ Can be queried for both and implements a helper function
     * |                         ~ to identify if it's a container or not.
     * |-- RWContainer ~~~~~~~~~~~ Receives a char pointer to a data array that can later be used to "unfold" 
     * |   |                     ~ the tree, i.e. create children from the data.
     * |   |                     ~ Those children can then be accessed directly through the 
     * |   |                     ~ array accessor operator.
     * |   |--> Implementations 
     * |
     * |-- RWData ~~~~~~~~~~~~~~~~ Also receives a char pointer to a data array that is, however, meant to 
     * |   |                     ~ be used in conjunction with data conversion functions 
     * |   |                     ~ implemented statically in the RWChunk namespace.
     * |   |--> Implementations
     * |
     * |-- RWInvalid ~~~~~~~~~~~~~ Meant to be used if an invalid chunk structure is detected.
     *     |                     ~ Similar to RWData, but the data array shall contain the 
     *     |                     ~ supposed "header" data.
     *     |--> Implementations
     */

namespace rw
{
    /// @brief Interface / Base class for all deserialized RenderWare chunks.
    class RWChunk
    {
    public:
        RWChunk() = delete;
        RWChunk(SectionID _sectionID, uint32_t _libraryID, RWChunk* _parent);
        virtual ~RWChunk();

        SectionID sectionID() const;
        const char* sectionName() const;
        const char* sectionVendor() const;

        uint32_t libID() const;
        uint32_t versionNumber() const;
        uint32_t buildNumber() const;
        
        bool isContainer() const;

        RWChunk* parent() const;
        
        static bool isContainer(SectionID _id);
        
        /// @brief use RWData::data()
        static std::string rwStrToStdStr(const char* _rwString, size_t _rwSize);
        
        /// @brief use RWData::data()
        static std::string frameToStdString(const char* _rgFrame, size_t _rwSize);
        
        /// @brief use RWData::data()
        static CClumpStruct toClumpStruct(const char* _rwStruct, size_t _rwSize);
        
        /// @brief use RWData::data()
        static std::vector<CFrameListStructEntry>toFrameVector(const char* _rwStruct, size_t _rwSize);
        
        /// @brief use RWData::data()
        static CAtomicStruct toAtomicStruct(const char* _rwStruct, size_t _rwSize);
        
        /// @brief use RWData::data()
        static std::vector<MaterialIndex> toMaterialListStruct(const char* _rwStruct, size_t _rwSize);
        
        /// @brief use RWData::data()
        static CMaterialStruct toMaterialStruct(const char* _rwStruct, size_t _rwSize);
        
        /// @brief use RWData::data()
        static CTextureStruct toTextureStruct(const char* _rwStruct, size_t _rwSize);
        
        /// @brief use RWData::data()
        static GeometryStructResult toGeometryStruct(const char* _rwStruct, size_t _rwSize, uint32_t _libID);
        
        /// @brief use RWData::data()
        static BinMeshPLGResult toBinMeshPLG(const char* _rwStruct, size_t _rwSize);
        
        /// @brief use RWData::data()
        static RasterStructResult toRasterStruct(const char* _rwStruct, size_t _rwSize);
        
        /// @brief use RWData::data()
        static CTextureDictionaryStruct toTextureDictionaryStruct(const char* _rwStruct, size_t _rwSize);

    protected:
        SectionID p_sectionID;
        uint32_t p_libID;
        RWChunk* p_parent;
    };

    /// @brief Interface / Base class for RenderWare chunks that can contain child sections. 
    ///        Inherits from RWChunk.
    class RWContainer : public RWChunk
    {
    public:
        RWContainer() = delete;
        RWContainer(SectionID _sectionID, uint32_t _libraryID, RWChunk* _parent);
        virtual ~RWContainer();

        virtual RWChunk& operator[](size_t _childIndex) = 0;
        virtual const RWChunk& operator[](size_t _childIndex) const = 0;
        virtual size_t childrenCount() const = 0;
        int findFirstOf(SectionID _sectionID, int _startIndex = 0) const;

        virtual bool isFolded() const = 0;
        virtual bool isValid() const = 0;

        virtual void unfold() = 0;
        virtual void fold() = 0;
    };

    /// @brief Implementation of RWContainer that holds its own data (S = self).
    class RWContainerS : public RWContainer
    {
    public:
        RWContainerS() = delete;
        RWContainerS(SectionID _sectionID, uint32_t _libraryID, RWChunk* _parent, const char* _rwContainer, size_t _rwSize);
        virtual ~RWContainerS();

        virtual RWChunk& operator[](size_t _childIndex) override;
        virtual const RWChunk& operator[](size_t _childIndex) const override;
        virtual size_t childrenCount() const override;

        virtual bool isFolded() const override;
        virtual bool isValid() const override;

        virtual void unfold() override;
        virtual void fold() override;

    protected:
        std::vector<char> p_data;               // temporary storage
        std::vector<RWChunk*> p_children;
    };

    /// @brief Interface / base class for RenderWare chunks cannot have child sections but 
    ///        instead hold data. Inherits from RWChunk.
    class RWData : public RWChunk
    {
    public:
        RWData() = delete;
        RWData(SectionID _sectionID, uint32_t _libraryID, RWChunk* _parent);
        virtual ~RWData();

        virtual char& operator[](size_t _index) = 0;
        virtual char* data() = 0;
        virtual const char* data() const = 0;
        virtual size_t dataSize() const = 0;
    };

    /// @brief Implementation of RWData that holds its own data (S = self).
    class RWDataS : public RWData
    {
    public:
        RWDataS() = delete;
        RWDataS(SectionID _sectionID, uint32_t _libraryID, RWChunk* _parent, const char* _rwData, size_t _rwSize);
        virtual ~RWDataS();

        char& operator[](size_t _index) override;
        virtual char* data() override;
        virtual const char* data() const override;
        virtual size_t dataSize() const override;

    protected:
        std::vector<char> p_data;               // indefinite storage
    };

    /// @brief Special data specialization of RWChunk that includes the "header" in 
    ///        the data if the "header" was found to be invalid.
    class RWInvalid : public RWChunk
    {
    public:
        RWInvalid() = delete;
        RWInvalid(SectionID _sectionID, uint32_t _libraryID, RWChunk* _parent);
        virtual ~RWInvalid();

        virtual char& operator[](size_t _index) = 0;
        virtual char* data() = 0;
        virtual const char* data() const = 0;
        virtual size_t dataSize() const = 0;
    };

    /// @brief Implementation of RWInvalid that holds its own data.
    class RWInvalidS : public RWInvalid
    {
    public:
        RWInvalidS() = delete;
        RWInvalidS(RWChunk* _parent, const char* _rwData, size_t _rwSize);
        virtual ~RWInvalidS();

        virtual char& operator[](size_t _index) override;
        virtual char* data() override;
        virtual const char* data() const override;
        virtual size_t dataSize() const override;

    private:
        std::vector<char> p_data;
    };

    inline int getTextureSetCount(CGeometryFormat _flagSet)
    {
        int texCountFlag = _flagSet.textureCoordinateSets;
        if (texCountFlag) return texCountFlag;
        else {
            return _flagSet.rpGEOMETRYTEXTURED + _flagSet.rpGEOMETRYTEXTURED2;
        }
    }

    std::string report(const RWDataS* _input);
    std::string report(const GeometryStructResult& _input);
    std::string report(const BinMeshPLGResult& _input);
}