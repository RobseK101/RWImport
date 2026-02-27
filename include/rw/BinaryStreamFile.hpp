#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <cstring>
#include <ren/FileHandle.hpp>
#include <ren/Exceptions.hpp>
#include <D3DCompatibility.h>

// provisionally, glm will be used immediately
#include <glm/glm.hpp>

    ///
    /// @file
    /// @brief This header file contains all the struct definitions necessary to parse 
    ///        certain RenderWare Binary Stream Files (*.dff, *.txd) and generate meaningful
    ///        data from that. It includes helper functions for handling of section names 
    ///        and conversion of library ID stamps.
    /// @details Since most of glm's definitions are compatible, I used it as an ad-hoc math 
    ///          library. glm is feature complete in a lot of ways and open source.
    ///          I'd like to thank the authors of the gtamods.com wiki for providing most 
    ///          of this information.
    /// 

namespace rw
{
    /// @name General type definitions
    /// @{
    typedef glm::vec3 RwV3d, TVector3F;
    typedef uint32_t RwRGBA;    // uint8_t r, g, b, a
    typedef glm::vec2 RwTexCoords;
    typedef glm::mat3 TMatrix3x3F;
    typedef int32_t bool32;
    constexpr bool32 RW_FALSE = 0;
    constexpr bool32 RW_TRUE = 1;
    /// @}
    
    struct RpTriangle
    {
        uint16_t vertex2;
        uint16_t vertex1;
        uint16_t materialID;
        uint16_t vertex3;
    };

    struct RwSphere
    {
        RwV3d center;
        float radius;
    };

    /// 
    /// @brief Parses the RenderWare library ID stamp for the version number.
    ///        Taken from https://gtamods.com/wiki/RenderWare
    /// @details <b>Understand this</b>: These hex nibbles in the unpacked version number are used to encode decimal digits. That's why 
    ///          the dotted version number matches the apparent "hex number". 
    ///          During packing bits 6 and 7 are discarded because in this scheme they would never be used.
    ///          (So the maximum revision number is 39 -> xx11 1001)
    ///
    inline constexpr uint32_t libraryIDUnpackVersion(uint32_t libid)
    {
        if(libid & 0xFFFF0000)
            return (libid>>14 & 0x3FF00) + 0x30000 |
                   (libid>>16 & 0x3F);
        return libid<<8;
    }

    ///
    /// @brief Parses the RenderWare library ID stamp for the version number. 
    ///        Taken from https://gtamods.com/wiki/RenderWare
    ///
    inline constexpr uint32_t libraryIDUnpackBuild(uint32_t libid)
    {
        if(libid & 0xFFFF0000)
            return libid & 0xFFFF;
        return 0;
    }

    ///
    /// @brief Builds the RenderWare library ID stamp from a version number and a build number. 
    ///        Taken from https://gtamods.com/wiki/RenderWare
    ///
    inline constexpr uint32_t libraryIDPack(uint32_t version, uint32_t build)
    {
        if(version <= 0x31000)
            return version>>8;
        return (version-0x30000 & 0x3FF00) << 14 | (version & 0x3F) << 16 |
               (build & 0xFFFF);
    }

    /// RenderWare version 3.1.0.0 (0x31000)
    constexpr uint32_t versionIII_PS2 = 0x31000;

    /// RenderWare version 3.3.0.2 (0x33002)
    constexpr uint32_t versionIII_PC =  0x33002;

    /// RenderWare version 3.3.0.2 (0x33002)
    constexpr uint32_t versionVC_PS2 =  0x33002;

    /// RenderWare version 3.4.0.3 (0x34003)
    constexpr uint32_t versionVC_PC =   0x34003;

    /// RenderWare version 3.6.0.3 (0x36003)
    constexpr uint32_t versionSA =      0x36003;

    /// 
    /// @brief Enum of RenderWare section IDs, including Rockstar's custom sections and some 
    ///        additional ones. Taken from https://gtamods.com/wiki/List_of_RW_section_IDs
    ///
    enum SectionID : uint32_t
    {
        // Core module 0x000000
        RW_PADDING = 0,
        RW_STRUCT = 1,              // A generic section that stores data for its parent. 
        RW_STRING = 2,              // Stores a 4-byte aligned ASCII string. 
        RW_EXTENSION = 3,           // A container for non-standard extensions of its parent 
                                    // section. 
        RW_CAMERA = 5,              // Contains a camera (unused in GTA games). 
        RW_TEXTURE = 6,             // Stores the sampler state of a texture. 
        RW_MATERIAL = 7,            // Defines a material to be used on a geometry. 
        RW_MATERIALLIST = 8,        // Container for a list of materials. 
        RW_ATOMICSECTION = 9,       // "Atomic Section"
        RW_PLANE = 0xA,
        RW_WORLD = 0xB,             // The root section of the level geometry. 
        RW_SPLINE = 0xC,
        RW_MATRIX = 0xD,
        RW_FRAMELIST = 0xE,         // Container for a list of frames. A frame holds the 
                                    // transformation that is applied to an Atomic. 
        RW_GEOMETRY = 0xF,          // A platform-independent container for meshes. 
        RW_CLUMP = 0x10,            // The root section for a 3D model. 
        RW_LIGHT = 0x12,            // Stores different dynamic lights. 
        RW_USTRING = 0x13,          // "Unicode String"
        RW_ATOMIC = 0x14,           // Defines the basic unit for the RenderWare graphics 
                                    // pipeline. Generally speaking, an Atomic can be 
                                    // directly converted to a single draw call. 
        RW_RASTER = 0x15,           // Stores a platform-dependent (i.e. native) texture image. 
        RW_TEXTUREDICTIONARY = 0x16,// A container for texture images (also called raster). 
        RW_ANIMDATABASE = 0x17, 
        RW_IMAGE = 0x18,            // An individual texture image. 
        RW_SKINANIMATION = 0x19, 
        RW_GEOMETRYLIST = 0x1A,     // A container for a list of geometries. 
        RW_ANIMANIMATION = 0x1B, 
        RW_TEAM = 0x1C, 
        RW_CROWD = 0x1D, 
        RW_DELTAMORPHANIM = 0x1E, 
        RW_RIGHTTORENDER = 0x1F,    // Stores the render pipeline the engine uses to draw an 
                                    // atomic or material. 
        RW_MULTITEXEFFECT = 0x20,   // MultiTexture Effect Native
        RW_MULTITEXEFFDIC = 0x21,   // MultiTexture Effect Dictionary
        RW_TEAMDICT = 0x22,         // Team Dictionary
        RW_PIDTEXDIC = 0x23,        // Platform Independent Texture Dictionary
        RW_TOC = 0x24,              // Table of Contents
        RW_PARTICLESTDGDATA = 0x25, // Particle Standard Global Data
        RW_ALTPIPE = 0x26, 
        RW_PIDPEDS = 0x27,          // Platform Independent Peds
        RW_PATCHMESH = 0x28,
        RW_CHUNKGROUPSTART = 0x29,
        RW_CHUNKGROUPEND = 0x2A,
        RW_UVANIMDICTIONARY = 0x2B, 
        RW_COLLTREE = 0x2C,
        
        // Toolkit module 0x000001
        RW_METRICS_PLG = 0x101, 
        RW_SPLINE_PLG = 0x102, 
        RW_STEREO_PLG = 0x103, 
        RW_VRML_PLG = 0x104, 
        RW_MORPH_PLG = 0x105, 
        RW_PVS_PLG = 0x106, 
        RW_MEMORYLEAK_PLG = 0x107, 
        RW_ANIMATION_PLG = 0x108, 
        RW_GLOSS_PLG = 0x109, 
        RW_LOGO_PLG = 0x10A, 
        RW_MEMORYINFO_PLG = 0x10B, 
        RW_RANDOM_PLG = 0x10C, 
        RW_PNGIMAGE_PLG = 0x10D, 
        RW_BONE_PLG = 0x10E, 
        RW_VRMLANIM_PLG = 0x10F, 
        RW_SKYMIPMAPVALUE = 0x110,  // Stores MipMap parameters for the PS2 version of the
                                    // engine (codenamed Sky).
        RW_MRM_PLG = 0x111, 
        RW_LODATOMIC_PLG = 0x112, 
        RW_ME_PLG = 0x113, 
        RW_LIGHTMAP_PLG = 0x114, 
        RW_REFINE_PLG = 0x115, 
        RW_SKIN_PLG = 0x116, 
        RW_LABEL_PLG = 0x117, 
        RW_PARTICLES_PLG = 0x118, 
        RW_GEOMTX_PLG = 0x119, 
        RW_SYNTHCORE_PLG = 0x11A, 
        RW_STQPP_PLG = 0x11B, 
        RW_PARTPP_PLG = 0x11C, 
        RW_COLLISION_PLG = 0x11D, 
        RW_HANIM_PLG = 0x11E, 
        RW_USERDATA_PLG = 0x11F, 
        RW_MATEFFECTS_PLG = 0x120, 
        RW_PARTYSTEM_PLG = 0x121,   // "Particle System PLG"
        RW_DELTAMORPH_PLG = 0x122, 
        RW_PATCH_PLG = 0x123, 
        RW_TEAM_PLG = 0x124, 
        RW_CROWDPP_PLG = 0x125, 
        RW_MIPSPLIT_PLG = 0x126, 
        RW_ANISOTROPY_PLG = 0x127,  // Stores the anisotropy for a texture filter.
        RW_GCNMATERIAL_PLG = 0x129, 
        RW_GEOMPVS_PLG = 0x12A,     // "Geometric PVS PLG"
        RW_XBOXMAT_PLG = 0x12B, 
        RW_MULTITEXTURE_PLG = 0x12C, 
        RW_CHAIN_PLG = 0x12D, 
        RW_TOON_PLG = 0x12E, 
        RW_PTANK_PLG = 0x12F, 
        RW_PARTICLESTD_PLG = 0x130, 
        RW_PDS_PLG = 0x131, 
        RW_PRTADV_PLG = 0x132, 
        RW_NORMALMAP_PLG = 0x133, 
        RW_ADC_PLG = 0x134, 
        RW_UVANIM_PLG = 0x135, 
        RW_CHARSET_PLG = 0x180,     // "Character Set PLG"
        RW_NOHSWORLD_PLG = 0x181, 
        RW_IMPORTUTIL_PLG = 0x182, 
        RW_SLERP_PLG = 0x183, 
        RW_OPTIM_PLG = 0x184, 
        RW_TLWORLD_PLG = 0x185, 
        RW_DATABASE_PLG = 0x186, 
        RW_RAYTRACE_PLG = 0x187, 
        RW_RAY_PLG = 0x188, 
        RW_LIBRARY_PLG = 0x189, 
        RW_2D_PLG = 0x190, 
        RW_TILERENDER_PLG = 0x191, 
        RW_JPEGIMAGE_PLG = 0x192, 
        RW_TGAIMAGE_PLG = 0x193, 
        RW_GIFIMAGE_PLG = 0x194, 
        RW_QUAT_PLG = 0x195, 
        RW_SPLINEPVS_PLG = 0x196, 
        RW_MIPMAP_PLG = 0x197, 
        RW_MIPMAPK_PLG = 0x198, 
        RW_2DFONT = 0x199, 
        RW_INTERSECTION_PLG = 0x19A, 
        RW_TIFFIMAGE_PLG = 0x19B, 
        RW_PICK_PLG = 0x19C, 
        RW_BMPIMAGE_PLG = 0x19D, 
        RW_RASIMAGE_PLG = 0x19E, 
        RW_SKINFX_PLG = 0x19F, 
        RW_VCAT_PLG = 0x1A0, 
        RW_2DPATH = 0x1A1, 
        RW_2DBRUSH = 0x1A2, 
        RW_2DOBJECT = 0x1A3, 
        RW_2DSHAPE = 0x1A4, 
        RW_2DSCENE = 0x1A5, 
        RW_2DPICKREGION = 0x1A6, 
        RW_2DOBJSTRING = 0x1A7, 
        RW_2DANIMATION_PLG = 0x1A8, 
        RW_2DANIMATION = 0x1A9, 
        RW_2DKEYFRAME = 0x1B0, 
        RW_2DMAESTRO = 0x1B1, 
        RW_BARYCENTRIC = 0x1B2, 
        RW_PIDTEXDIC_TK = 0x1B3,     // "PlatformIndependent Texture Dictionary TK" (Toolkit?)
        RW_TOC_TK = 0x1B4, 
        RW_TPL_TK = 0x1B5, 
        RW_ALTPIPE_TK = 0x1B6, 
        RW_ANIMATION_TK = 0x1B7, 
        RW_SKINSPLIT_TK = 0x1B8, 
        RW_COMPKEY_TK = 0x1B9,      // "Compressed Key TK"
        RW_GEOMCOND_PLG = 0x1BA,    // "Geometry Conditioning PLG"
        RW_WING_PLG = 0x1BB, 
        RW_GENPIPELINE_TK = 0x1BC,  // "Generic Pipeline TK"
        RW_LIGHTMAPCONV_TK = 0x1BD, // "Lightmap Conversion TK"
        RW_FILESYSTEM_PLG = 0x1BE, 
        RW_DICTIONARY_TK = 0x1BF, 
        RW_UVANIMLINEAR = 0x1C0, 
        RW_UVANIMPARAM = 0x1C1, 

        // World module 0x000005 
        RW_BINMESH_PLG = 0x50E, 
        RW_NATIVEDATA_PLG = 0x510, 

        // ZModeler 0x0000F2
        ZM_LOCK = 0xF21E,           // Unofficial extension that stores a password that 
                                    // protects the file from being modified when opened 
                                    // in ZModeler. Ignored by other applications like 
                                    // RWAnalyze and the GTA games.

        // Rockstar Games 0x0253F2
        RG_ATOMICVISDIS = 0x253F200,// "Atomic Visibility Distance"
        RG_CLUMPVISDIS = 0x253F201, // "Clump Visibility Distance"
        RG_FRAMEVISDIS = 0x253F202, // "Frame Visibility Distance"
        RG_PIPELINESET = 0x253F2F3, // Stores the render pipeline used to draw objects with
                                    // Rockstar-specific plug-ins.
        RG_TEXDICTLINK = 0x253F2F5, // "TexDictionary Link"
        RG_SPECULARMAT = 0x253F2F6, // Stores a specularity map.
        RG_2DEFFECT = 0x253F2F8,    // Used to attach various GTA-specific effects to models, for
                                    // example to enable script interaction or particle 
                                    // effects.
        RG_EXTRAVERTCOL = 0x253F2F9,// Stores an additional array of vertex colors, that 
                                    // are used in GTA during night-time to simulate some 
                                    // effects of dynamic global lighting. 
        RG_COLLMODEL = 0x253F2FA,   // Stores a collision model.
        RG_GTAHANIM = 0x253F2FB,    // "GTA HAnim"
        RG_REFLECTMAT = 0x253F2FC,  // Enables advanced environment mapping.
        RG_BREAKABLE = 0x253F2FD,   // Contains a mesh that is used to render objects that 
                                    // are breakable (like windows or tables). 
        RG_FRAME = 0x253F2FE,       // Stores the name of a frame within a Frame List.

        // custom internal REN IDs
        REN_ID_FILE = 0xFFFFFF01,   // Container
        REN_ID_INVALID = 0xFFFFFF02
    };

    ///
    /// @brief Returns the clear text string for each section ID.
    /// @param _id The section ID.
    /// @returns An ANSI C string of the section name.
    ///
    inline const char* sectionName(SectionID _id)
    {
        switch (_id) {
            case RW_PADDING:                return "Padding";
            case RW_STRUCT:                 return "Struct";
            case RW_STRING:                 return "String";
            case RW_EXTENSION:              return "Extension";
            case RW_CAMERA:                 return "Camera";
            case RW_TEXTURE:                return "Texture";
            case RW_MATERIAL:               return "Material";
            case RW_MATERIALLIST:           return "Material List";
            case RW_ATOMICSECTION:          return "Atomic Section";
            case RW_PLANE:                  return "Plane Section";
            case RW_WORLD:                  return "World";
            case RW_SPLINE:                 return "Spline";
            case RW_MATRIX:                 return "Matrix";
            case RW_FRAMELIST:              return "Frame List";
            case RW_GEOMETRY:               return "Geometry";
            case RW_CLUMP:                  return "Clump";
            case RW_LIGHT:                  return "Light";
            case RW_USTRING:                return "Unicode String";
            case RW_ATOMIC:                 return "Atomic";
            case RW_RASTER:                 return "Raster";
            case RW_TEXTUREDICTIONARY:      return "Texture Dictionary";
            case RW_ANIMDATABASE:           return "Animation Database";
            case RW_IMAGE:                  return "Image";
            case RW_SKINANIMATION:          return "Skin Animation";
            case RW_GEOMETRYLIST:           return "Geometry List";
            case RW_ANIMANIMATION:          return "Anim Animation";
            case RW_TEAM:                   return "Team";
            case RW_CROWD:                  return "Crowd";
            case RW_DELTAMORPHANIM:         return "Delta Morph Animation";
            case RW_RIGHTTORENDER:          return "Right to Render";
            case RW_MULTITEXEFFECT:         return "MultiTexture Effect Native";
            case RW_MULTITEXEFFDIC:         return "MultiTexture Effect Dictionary";
            case RW_TEAMDICT:               return "Team Dictionary";
            case RW_PIDTEXDIC:              return "Platform Independent Texture Dictionary";
            case RW_TOC:                    return "Table of Contents";
            case RW_PARTICLESTDGDATA:       return "Particle Standard Global Data";
            case RW_ALTPIPE:                return "AltPipe";
            case RW_PIDPEDS:                return "Platform Independent Peds";
            case RW_PATCHMESH:              return "Patch Mesh";
            case RW_CHUNKGROUPSTART:        return "Chunk Group Start";
            case RW_CHUNKGROUPEND:          return "Chunk Group End";            
            case RW_UVANIMDICTIONARY:       return "UV Animation Dictionary";
            case RW_COLLTREE:               return "Coll Tree";
            case RW_METRICS_PLG:            return "Metrics PLG";
            case RW_SPLINE_PLG:             return "Spline PLG";
            case RW_STEREO_PLG:             return "Stereo PLG";
            case RW_VRML_PLG:               return "VRML PLG";
            case RW_MORPH_PLG:              return "Morph PLG";
            case RW_PVS_PLG:                return "PVS PLG";
            case RW_MEMORYLEAK_PLG:         return "Memory Leak PLG";
            case RW_ANIMATION_PLG:          return "Animation PLG";
            case RW_GLOSS_PLG:              return "Gloss PLG";
            case RW_LOGO_PLG:               return "Logo PLG";
            case RW_MEMORYINFO_PLG:         return "Memory Info PLG";
            case RW_RANDOM_PLG:             return "Random PLG";
            case RW_PNGIMAGE_PLG:           return "PNG Image PLG";
            case RW_BONE_PLG:               return "Bone PLG";
            case RW_VRMLANIM_PLG:           return "VRML Anim PLG";
            case RW_SKYMIPMAPVALUE:         return "Sky Mipmap Val";
            case RW_MRM_PLG:                return "MRM PLG";
            case RW_LODATOMIC_PLG:          return "LOD Atomic PLG";
            case RW_ME_PLG:                 return "ME PLG";
            case RW_LIGHTMAP_PLG:           return "Lightmap PLG";
            case RW_REFINE_PLG:             return "Refine PLG";
            case RW_SKIN_PLG:               return "Skin PLG";
            case RW_LABEL_PLG:              return "Label PLG";
            case RW_PARTICLES_PLG:          return "Particles PLG";
            case RW_GEOMTX_PLG:             return "GeomTX PLG";
            case RW_SYNTHCORE_PLG:          return "Synth Core PLG";
            case RW_STQPP_PLG:              return "STQPP PLG";
            case RW_PARTPP_PLG:             return "PartPP PLG";
            case RW_COLLISION_PLG:          return "Collision PLG";
            case RW_HANIM_PLG:              return "HAnim PLG";
            case RW_USERDATA_PLG:           return "User Data PLG";
            case RW_MATEFFECTS_PLG:         return "Material Effects PLG";
            case RW_PARTYSTEM_PLG:          return "Particle System PLG";
            case RW_DELTAMORPH_PLG:         return "Delta Morph PLG";
            case RW_PATCH_PLG:              return "Patch PLG";
            case RW_TEAM_PLG:               return "Team PLG";
            case RW_CROWDPP_PLG:            return "Crowd PP PLG";
            case RW_MIPSPLIT_PLG:           return "Mip Split PLG";
            case RW_ANISOTROPY_PLG:         return "Anisotropy PLG";
            case RW_GCNMATERIAL_PLG:        return "GCN Material PLG";
            case RW_GEOMPVS_PLG:            return "Geometric PVS PLG";
            case RW_XBOXMAT_PLG:            return "XBOX Material";
            case RW_MULTITEXTURE_PLG:       return "Multi Texture PLG";
            case RW_CHAIN_PLG:              return "Chain PLG";
            case RW_TOON_PLG:               return "Toon PLG";
            case RW_PTANK_PLG:              return "PTank PLG";
            case RW_PARTICLESTD_PLG:        return "Particle Standard PLG";
            case RW_PDS_PLG:                return "PDS PLG";
            case RW_PRTADV_PLG:             return "PrtAdv PLG";
            case RW_NORMALMAP_PLG:          return "Normal Map PLG";
            case RW_ADC_PLG:                return "ADC PLG";
            case RW_UVANIM_PLG:             return "UV Animation PLG";
            case RW_CHARSET_PLG:            return "Character Set PLG";
            case RW_NOHSWORLD_PLG:          return "NOHS World PLG";
            case RW_IMPORTUTIL_PLG:         return "Import Util PLG";
            case RW_SLERP_PLG:              return "Slerp PLG";
            case RW_OPTIM_PLG:              return "Optim PLG";
            case RW_TLWORLD_PLG:            return "TL World PLG";
            case RW_DATABASE_PLG:           return "Database PLG";
            case RW_RAYTRACE_PLG:           return "Raytrace PLG";
            case RW_RAY_PLG:                return "Ray PLG";
            case RW_LIBRARY_PLG:            return "Library PLG";
            case RW_2D_PLG:                 return "2D PLG";
            case RW_TILERENDER_PLG:         return "Tile Renderer PLG";
            case RW_JPEGIMAGE_PLG:          return "JPEG Image PLG";
            case RW_TGAIMAGE_PLG:           return "TGA Image PLG";
            case RW_GIFIMAGE_PLG:           return "GIF Image PLG";
            case RW_QUAT_PLG:               return "Quat PLG";
            case RW_SPLINEPVS_PLG:          return "Spline PVS PLG";
            case RW_MIPMAP_PLG:             return "Mipmap PLG";
            case RW_MIPMAPK_PLG:            return "MipmapK PLG";
            case RW_2DFONT:                 return "2D Font";
            case RW_INTERSECTION_PLG:       return "Intersection PLG";
            case RW_TIFFIMAGE_PLG:          return "TIFF Image PLG";
            case RW_PICK_PLG:               return "Pick PLG";
            case RW_BMPIMAGE_PLG:           return "BMP Image PLG";
            case RW_RASIMAGE_PLG:           return "RAS Image PLG";
            case RW_SKINFX_PLG:             return "Skin FX PLG";
            case RW_VCAT_PLG:               return "VCAT PLG";
            case RW_2DPATH:                 return "2D Path";
            case RW_2DBRUSH:                return "2D Brush";
            case RW_2DOBJECT:               return "2D Object";
            case RW_2DSHAPE:                return "2D Shape";
            case RW_2DSCENE:                return "2D Scene";
            case RW_2DPICKREGION:           return "2D Pick Region";
            case RW_2DOBJSTRING:            return "2D Object String";
            case RW_2DANIMATION_PLG:        return "2D Animation PLG";
            case RW_2DANIMATION:            return "2D Animation";
            case RW_2DKEYFRAME:             return "2D Keyframe";
            case RW_2DMAESTRO:              return "2D Maestro";
            case RW_BARYCENTRIC:            return "Barycentric";
            case RW_PIDTEXDIC_TK:           return "Platform Independent Texture Dictionary TK";
            case RW_TOC_TK:                 return "TOC TK";
            case RW_TPL_TK:                 return "TPL TK";
            case RW_ALTPIPE_TK:             return "AltPipe TK";
            case RW_ANIMATION_TK:           return "Animation TK";
            case RW_SKINSPLIT_TK:           return "Skin Split Toolkit";
            case RW_COMPKEY_TK:             return "Compressed Key TK";
            case RW_GEOMCOND_PLG:           return "Geometry Conditioning PLG";
            case RW_WING_PLG:               return "Wing PLG";
            case RW_GENPIPELINE_TK:         return "Generic Pipeline TK";
            case RW_LIGHTMAPCONV_TK:        return "Lightmap Conversion TK";
            case RW_FILESYSTEM_PLG:         return "Filesystem PLG";
            case RW_DICTIONARY_TK:          return "Dictionary TK";
            case RW_UVANIMPARAM:            return "UV Animation Parameter";
            case RW_BINMESH_PLG:            return "Bin Mesh PLG";
            case RW_NATIVEDATA_PLG:         return "Native Data PLG";
            case ZM_LOCK:                   return "ZModeler Lock";
            case RG_ATOMICVISDIS:           return "Atomic Visibility Distance";
            case RG_CLUMPVISDIS:            return "Clump Visibility Distance";
            case RG_FRAMEVISDIS:            return "Frame Visibility Distance";
            case RG_PIPELINESET:            return "Pipeline Set";
            case RG_TEXDICTLINK:            return "TexDictionary Link";
            case RG_SPECULARMAT:            return "Specular Material";
            case RG_2DEFFECT:               return "2D Effect";
            case RG_EXTRAVERTCOL:           return "Extra Vert Colour";
            case RG_COLLMODEL:              return "Collision Model";
            case RG_GTAHANIM:               return "GTA HAnim";
            case RG_REFLECTMAT:             return "Reflection Material";
            case RG_BREAKABLE:              return "Breakable";
            case RG_FRAME:                  return "Frame";

            // Internal uses:
            case REN_ID_FILE:               return "RW Stream File";
            case REN_ID_INVALID:            return "Invalid";

            default:                        return "Unknown";
        }
    }

    ///
    /// @brief Returns a clear text string for the section vendor.
    /// @param _id The section ID.
    /// @returns An ANSI C string of the vendor name.
    ///
    inline const char* sectionVendor(SectionID _id)
    {
        switch ((_id >> 8) & 0xFFFFFF) {
            case 0x0:           return "RW Core";
            case 0x1:           return "RW Toolkit";
            case 0x5:           return "RW World";
            case 0xEA:          return "EA Redwood Shores";
            case 0xF2:          return "ZModeler";
            case 0xCAFE:        return "THQ";
            case 0x253F2:       return "Rockstar North";
            case 0xFFFFFF:      return "Built-in";
            default:            return "Unknown";
        }
    }

    /// Header of a RenderWare Binary Stream File Chunk, i.e. of every single section.
    struct ChunkHeader
    {
        SectionID type; 

        /// @brief Size in bytes *after* the chunk header.
        uint32_t size;

        /// @brief Library ID Stamp.
        uint32_t libIDStamp;
    };

    /// @brief Struct section of a Clump section until version 3.3.0.0.
    struct CClumpStructOld
    {
        uint32_t atomicCount;
    };

    /// @brief Struct section of a Clump section after version 3.3.0.0.
    struct CClumpStruct
    {
        int32_t atomicCount;
        int32_t lightCount;

        /// @brief Zero in GTA games (unused).
        int32_t cameraCount;
    };


    /// @brief Header of the struct section of a Frame List section.
    /// @details A "frame" in this context is essentially what's referred to as a 
    ///          "transform" in other environments (e.g. Unity game engine).
    struct CFrameListStructHeader
    {
        int32_t frameCount;
    };

    /// @brief Single entry of a frame list.
    /// @details References to other transforms (the parent) cannot be 
    ///          forward-declaring. In other words, parents must always 
    ///          come before their children.
    struct CFrameListStructEntry
    {
        TMatrix3x3F rotationMatrix;
        TVector3F position;

        /// @brief The parent index in this list. -1 means no parent.
        int32_t parentIndex;

        /// @brief Unused in GTA games. 
        int32_t matrixFlags;
    };

    /// @brief The struct section of a Geometry List struct.
    struct CGeometryListStruct
    {
        int32_t geometryCount;
    };

    /// @name Format flags of the geometry struct.
    /// @{

    /// @brief Is triangle strip (if disabled it will be an triangle list).
    constexpr uint32_t rpGEOMETRYTRISTRIP = 0x00000001;

    /// @brief Has vertex positions.
    constexpr uint32_t rpGEOMETRYPOSITIONS = 0x00000002;

    /// @brief Has texture coordinates
    constexpr uint32_t rpGEOMETRYTEXTURED = 0x00000004;

    /// @brief Has vertex colors.
    constexpr uint32_t rpGEOMETRYPRELIT = 0x00000008;

    /// @brief Has  vertex normals.
    constexpr uint32_t rpGEOMETRYNORMALS = 0x00000010;

    /// @brief Geometry is lit (dynamic and static). 
    constexpr uint32_t rpGEOMETRYLIGHT = 0x00000020;

    /// @brief Modulate material color.
    constexpr uint32_t rpGEOMETRYMODULATEMATERIALCOLOR = 0x00000040; 

    /// @brief Texture coordinates 2.
    constexpr uint32_t rpGEOMETRYTEXTURED2 = 0x00000080;

    /// @brief Native Geometry. 
    constexpr uint32_t rpGEOMETRYNATIVE = 0x01000000;

    /// @brief Bitmask for the count of texture coordinate sets.
    /// @details Bits 16-23 (mask 0x00FF0000) in the format description give the number of texture 
    ///          coordinate sets (numTexSets). If the value in the file is zero, rpGEOMETRYTEXTURED 
    ///          means there is one set of texture coordinates, rpGEOMETRYTEXTURED2 means there are two. 
    constexpr uint32_t textureCoordSetCountMask = 0x00FF0000;

    /// @brief Shift value to obtain a valid integer of the texture set count.
    constexpr uint32_t textureCoordSetCountRShift = 16;
    /// @}
    
    /// @brief Struct representation of the geometry format flags.
    /// @details Note that any of the 1 bit wide entries essentially functions as 
    ///          a bool value.
    struct CGeometryFormat
    {
        /// @brief Is triangle strip (if disabled it will be an triangle list).
        uint32_t rpGEOMETRYTRISTRIP : 1;

        /// @brief Has vertex positions.
        uint32_t rpGEOMETRYPOSITIONS : 1;

        /// @brief Has texture coordinates
        uint32_t rpGEOMETRYTEXTURED : 1;

        /// @brief Has vertex colors.
        uint32_t rpGEOMETRYPRELIT : 1;

        /// @brief Has  vertex normals.
        uint32_t rpGEOMETRYNORMALS : 1;

        /// @brief Geometry is lit (dynamic and static). 
        uint32_t rpGEOMETRYLIGHT : 1;

        /// @brief Modulate material color.
        uint32_t rpGEOMETRYMODULATEMATERIALCOLOR : 1;

        /// @brief Texture coordinates 2.
        uint32_t rpGEOMETRYTEXTURED2 : 1;
        uint32_t unused1 : 8;

        /// @brief Number of additional texture coordinate sets.
        uint32_t textureCoordinateSets : 8;

        /// @brief Native Geometry. 
        uint32_t rpGEOMETRYNATIVE : 1;
        uint32_t unused2 : 7;
    };

    static_assert(sizeof(CGeometryFormat) == 4, "Error, CGeometryFormat is not compiled correctly!");

    /// @brief Header of the struct section of the Geometry struct before RW Version 3.4.0.0.
    struct CGeometryStructHeader
    {
        /// @brief Format flags in their struct representation.
        CGeometryFormat format;
        int32_t triangleCount;
        int32_t vertexCount;

        /// @brief Always 1 in GTA.
        int32_t morphTargetCount;
        float ambient;
        float specular;
        float diffuse;
    };

    /// @brief Header of the struct section of the Geometry struct since RW Version 3.4.0.0.
    struct CGeometryStructHeaderNew
    {
        /// @brief Format flags in their struct representation.
        CGeometryFormat format;
        int32_t triangleCount;
        int32_t vertexCount;

        /// @brief Always 1 in GTA.
        int32_t morphTargetCount;
    };

    /// @brief Header of the vertex list within the geometry section's struct section.
    struct CGeometryStructVertexHeader
    {
        RwSphere boundingSphere;
        bool32 hasVertices;
        bool32 hasNormals;
    };

    /// @brief Header of the struct section of the Material List section.
    struct CMaterialListStructHeader
    {
        int32_t materialCount;      // includes material instances
    };

    /// @brief Single entry of a Material List. -1 means original material, 
    ///        otherwise instance referencing and original material (with index).
    typedef int32_t MaterialIndex;

    /// @brief Struct section of a Material section; until version 3.0.4.0.
    struct CMaterialStructOld
    {
        /// @brief Unused in GTA.
        uint32_t flags;
        RwRGBA color;
        int32_t unused;
        bool32 isTextured;
    };

    /// @brief Struct section of a Material section; after version 3.0.4.0.
    struct CMaterialStruct
    {
        /// @brief Unused in GTA.
        uint32_t flags;
        RwRGBA color;
        int32_t unused;
        bool32 isTextured;
        float ambient;
        float specular;
        float diffuse;
    };

    /// @brief Struct section of a Texture section.
    struct CTextureStruct
    {
        /// @brief Texture filtering (Texture filtering modes)
        uint32_t filtering : 8;

        /// @brief U-addressing
        uint32_t u : 4;

        /// @brief V-addressing
        uint32_t v : 4;

        /// @brief Does texture use mip levels?
        uint32_t mipmaps : 1;
        
        uint32_t padding : 15;
    };


    /// @name Texture filtering modes
    /// @{

    /// @brief Filtering is disabled.
    constexpr uint32_t FILTERNAFILTERMODE = 0;

    /// @brief Point sampled
    constexpr uint32_t FILTERNEAREST = 1;

    /// @brief Bilinear
    constexpr uint32_t FILTERLINEAR = 2;

    /// @brief Point sampled per pixel mip map
    constexpr uint32_t FILTERMIPNEAREST = 3;

    /// @brief Bilinear per pixel mipmap
    constexpr uint32_t FILTERMIPLINEAR = 4;

    /// @brief MipMap interp point sampled
    constexpr uint32_t FILTERLINEARMIPNEAREST = 5;

    /// @brief Trilinear
    constexpr uint32_t FILTERLINEARMIPLINEAR = 6;
    /// @}

    /// @name Texture addressing modes 
    /// @{

    /// @brief No tiling
    constexpr uint32_t TEXTUREADDRESSNATEXTUREADDRESS = 0;

    /// @brief Tile in U or V direction
    constexpr uint32_t TEXTUREADDRESSWRAP = 1;

    /// @brief Mirror in U or V direction
    constexpr uint32_t TEXTUREADDRESSMIRROR = 2;

    constexpr uint32_t TEXTUREADDRESSCLAMP = 3;
    constexpr uint32_t TEXTUREADDRESSBORDER = 4;
    /// @}

    /// @brief An atomic is basically a drawcall definition
    struct CAtomicStruct 
    {
        /// @brief Index of the frame within the clump's frame list.
        int32_t frameIndex;

        /// @brief Index of geometry within the clump's geometry list.
        int32_t geometryIndex;

        /// @brief See "Atomic flags". 
        uint32_t flags;

        /// @brief Typically zero.
        uint32_t unused;
    };

    /// @name Atomic flags 
    /// @{

    /// @brief A generic collision flag to indicate that the atomic should be considered 
    ///        in collision tests. It wasn't used in GTA games since they don't use RW 
    ///        collision system.
    constexpr uint32_t rpATOMICCOLLISIONTEST = 0x01;

    /// @brief The atomic is rendered if it is in the view frustum. It's set to TRUE 
    ///        for all models by default.
    constexpr uint32_t rpATOMICRENDER = 0x04;
    /// @}

    /// @brief Header of the Bin Mesh PLG's struct section.
    struct CBinMeshStructHeader
    {
        bool32 usesTriangleStrip;
        int32_t meshCount;
        int32_t totalIndexCount;
    };

    struct CBinMeshStructEntryHeader
    {
        int32_t indexCount;

        /// @brief Reference to the Geometry's material list
        int32_t materialIndex;
    };

    // then for each of those a list of actual vertex indices (!!!) just like in a primitive 
    // index list

    struct CWorldStructHeader
    {
        int32_t unknown1;
        float unknown2;
        float unknown3;
        float unknown4;
        float unknown5;
        float unknown6;
        float unknown7;
        int32_t unknown8;
        int32_t unknown9;
        int32_t unknown10;
        int32_t unknown11;
        int32_t unknown12;
        int32_t unknown13;
    };

    struct CPlaneSectionStructHeader
    {
        int32_t unknown1;
        float unknown2;
        int32_t unknown3;
        int32_t unknown4;
        float unknown5;
        float unknown6;
    };

    struct CAtomicSectionStructHeader
    {
        int32_t unknown1;
        int32_t unknown2;           // Triangle Count?
        int32_t unknown3;           // Vertex Count?
        float unknown4;
        float unknown5;
        float unknown6;
        float unknown7;
        float unknown8;
        float unknown9;
        int32_t unknown10;
        int32_t unknown11;
    };

    /// @brief Struct section of the Texture Dictionary section - before RW version 3.6 (Pre-SA)
    struct CTextureDictionaryStructOld 
    {
        uint32_t textureCount;
    };

    /// @brief Struct section of the Texture Dictionary section - since RW version 3.6 (SA)
    struct CTextureDictionaryStruct
    {
        uint16_t textureCount;
        uint16_t deviceID;
    };

    /// @enum
    typedef uint32_t D3DFORMAT_R;

    struct CRasterFormat 
    {
        uint32_t unknown : 8;
        uint32_t baseFormat : 4;
        uint32_t extendedFormat : 4;
        uint32_t padding : 16;
    };

    /// @name CRasterFormat::baseFormat values
    /// @{
    constexpr uint32_t RASTERBASE_DEFAULT = 0x0;

    /// @brief 1 bit alpha, RGB 5 bits each; also used for DXT1 with alpha
    constexpr uint32_t RASTERBASE_1555 = 0x1;

    /// @brief 5 bits red, 6 bits green, 5 bits blue; also used for DXT1 without alpha
    constexpr uint32_t RASTERBASE_565 = 0x2;

    /// @brief RGBA 4 bits each; also used for DXT3
    constexpr uint32_t RASTERBASE_4444 = 0x3;

    /// @brief Greyscale, D3DFMT_L8
    constexpr uint32_t RASTERBASE_LUM8 = 0x4;

    /// @brief RGBA 8 bits each
    constexpr uint32_t RASTERBASE_8888 = 0x5;

    /// @brief RGB 8 bits each, D3DFMT_X8R8G8B8
    constexpr uint32_t RASTERBASE_888 = 0x6;

    /// @brief RGB 5 bits each - rare, use 565 instead, D3DFMT_X1R5G5B5
    constexpr uint32_t RASTERBASE_555 = 0xA;
    /// @}
    
    /// @name CRasterFormat::extendedFormat values
    /// @{

    /// @brief RW generates mipmaps
    constexpr uint32_t RASTEREXT_AUTOMIPMAP = 0x1;

    /// @brief 2^8 = 256 palette colors
    constexpr uint32_t RASTEREXT_PAL8 = 0x2;

    /// @brief 2^4 = 16 palette colors
    constexpr uint32_t RASTEREXT_PAL4 = 0x4;

    /// @brief mipmaps included
    constexpr uint32_t RASTEREXT_MIPMAP = 0x8;
    /// @}

    /// @details Effective size is 88 bytes
    struct CRasterStructHeaderPC
    {
        struct {
            uint32_t platformID;

            /// @brief Texture filtering (Texture filtering modes)
            uint32_t filtering : 8;

            /// @brief U-addressing
            uint32_t u : 4;

            /// @brief V-addressing
            uint32_t v : 4;

            /// @brief Best zeroed out!
            uint32_t padding : 16;
            char name[32];
            char maskName[32];
        } textureFormat;
        
        struct {
            CRasterFormat format;
            union {
                /// @brief GTA3 & VC
                bool32 hasAlpha;

                /// @brief SA
                d3dc::D3DFORMAT d3dFormat;
            };
            uint16_t width;
            uint16_t height;
            uint8_t bitDepth;
            uint8_t mipmapLevelCount;
            uint8_t rasterType;
            union {
                /// @brief GTA3 & VC
                uint8_t compression;

                /// @brief SA
                struct {
                    uint8_t alpha : 1;
                    uint8_t cubeTexture : 1;
                    uint8_t autoMipmaps : 1;
                    uint8_t compressed : 1;

                    /// @brief Best zeroed out!
                    uint8_t padding : 4;
                };
            };
        } rasterFormat;
    };

    // Actual image data starts after the header. 
    // Indexed Images have a palette before the actual bitmap. I discovered 4 bytes per entry.
    // 4 Bytes is probably a default value, independent of supposed color format.
    // The actual image data is preceded by a size stamp. 
    // DXT1 Image data is preceded by a size stamp with the raw data following.

    /// @typedef
    typedef uint32_t RWPaletteEntry;

    /// @name Generic structs sufficient to hold the data extracted from the chunks
    /// @{

    struct GeometryStructResult
    {
        CGeometryFormat flags;
        struct 
        {
            float ambient;
            float specular;
            float diffuse;
        } legacySurfaceParams;
        std::vector<RwRGBA> prelitColors;
        std::vector<RwTexCoords> uvs;
        std::vector<RpTriangle> triangles;
        RwSphere boundingSphere;
        std::vector<TVector3F> positions;
        std::vector<TVector3F> normals;
    };

    struct BinMeshPLGResult
    {
        bool isTriangleStrip = false;
        std::vector<uint32_t> materialIndices;
        std::vector<std::vector<uint32_t>> meshes;
    };

    struct RasterStructResult
    {
        CRasterStructHeaderPC header;
        std::vector<RWPaletteEntry> paletteData;
        std::vector<char> imageData;
    };
    /// @}
}