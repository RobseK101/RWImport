#pragma once
#include <cstdint>
#include "Loadthread.h"
#include <glm/vec3.hpp>
#include <ren/ModelData.hpp>
#include <ren/ModelDataDefinitions.h>

#ifdef _WIN32
	#ifdef _DLL
		#define RENLIB __declspec(dllexport)
	#else 
		#define RENLIB __declspec(dllimport)
	#endif
#else
	#define RENLIB
#endif

#define RENAPI __cdecl

///
/// @file
/// This header file describes the API for the RWImport DLL/dynamic library. 
/// The RWImport API provides the requested data in arrays of blittable, i.e. trivially copyable, structs. 
/// 

/// Descriptive return values of most RWI functions
enum renResultCode : int32_t
{
	REN_OK = 0,
	REN_HANDLE_DOES_NOT_EXIST = 1,
	REN_OPERATION_FAILED = 2,
	REN_PROCESSING = 3,
	REN_NOT_INITIALIZED = 4,
	REN_NULLPTR_CPY_TARGET = 5, 
	REN_OUT_OF_BOUNDS_MESH_INDEX = 6,
	REN_NULLPTR_STRUCT = 7,
	REN_DATA_FORMAT_MISMATCH = 8,
	REN_OUT_OF_BOUNDS_TEXTURE_INDEX = 9,
	REN_FILE_CANNOT_BE_OPENED = 10,
	REN_COLLFILES_ALREADY_CACHED = 11,
	REN_LOADTHREAD_ALREADY_RUNNING = 12,
	REN_LOADTHREAD_NOT_RUNNING = 13,
	REN_LOGGER_ALREADY_RUNNING = 14,
	REN_LOGGER_NOT_RUNNING = 15
};

typedef int32_t renJobHandle;

/// Properties of the queried meshmodel
struct MeshmodelDefinition
{
	/// Number of queryable meshes
	uint32_t meshCount;

	/// Number of copyable transforms
	uint32_t transformCount;
};

/// Provides a pointer to an array to which the rwiGetTransformSC() function will copy the serialized transforms.
struct TransformCopyTarget
{
	/// Target location to which rwiGetTransformSC() should copy the transforms
	ren::Transform_blittable* transforms;
};

/// Contains a pointer to the array from which the serialized transforms can be copied.
struct TransformCopyDefinition
{
	/// Location from which the transforms can be copied
	const ren::Transform_blittable* transforms;

	/// Size of the array
	uint32_t transformCount;

	int32_t padding;
};

/// Properties of the queried mesh.
struct MeshDefinition
{
	/// Length of each Vector array
	uint32_t vertexCount;

	/// Length of the Triangle/index array
	uint32_t triangleCount;

	/// Length of the Material array
	uint32_t materialCount;

	/// Length of the SubmeshDescriptor array
	uint32_t submeshCount;
};

/// Provides pointers to arrays to which the rwiGetMeshSC() function will copy the mesh data.
struct MeshCopyTarget
{
	/// Target location to which rwiGetMeshSC() will copy the raw Vector3 positions array
	ren::Vector3* vertices;

	/// Target location to which rwiGetMeshSC() will copy the raw Vector2 uv array
	ren::Vector2* UVs;

	/// Target location to which rwiGetMeshSC() will copy the raw Vector3 normals array
	ren::Vector3* normals;

	/// Target location to which rwiGetMeshSC() will copy the raw Vector4 colors array
	ren::Vector4* colors; // changed 2025-10-08 for completeness

	/// Target location to which rwiGetMeshSC() will copy the raw triangles array
	ren::ShortTriangle* triangles;

	/// Target location to which rwiGetMeshSC() will copy the raw MaterialDescriptor array
	ren::MaterialDescriptor_blittable* materials;

	/// Target location to which rwiGetMeshSC() will copy the raw SubmeshDescriptor array
	ren::SubmeshDescriptor* submeshes;
};

/// Contains pointers to the arrays from which the mesh data can be copied.
struct MeshCopyDefinition
{	
	/// Location from which the Vector3 positions can be copied
	const ren::Vector3* vertices;

	/// Location from which the Vector2 UVs can be copied
	const ren::Vector2* UVs;

	/// Location from which the Vector3 normals can be copied
	const ren::Vector3* normals;

	/// Location from which the Vector4 colors can be copied
	const ren::Vector4* colors; // changed 2025-10-08 for completeness

	/// Location from which the Triangles can be copied
	const ren::ShortTriangle* triangles;

	/// Location from which the MaterialsDescriptors can be copied
	const ren::MaterialDescriptor_blittable* materials;

	/// Location from which the SubmeshDescriptors can be copied
	const ren::SubmeshDescriptor* submeshes;

	/// Length of each Vector array
	uint32_t vertexCount;

	/// Length of the Triangle/index array
	uint32_t triangleCount;

	/// Length of the Material array
	uint32_t materialCount;

	/// Length of the SubmeshDescriptor array
	uint32_t submeshCount;
};

/// Properties of the texture set.
struct TextureSetDefinition // Changed 2025-10-08
{
	uint32_t textureCount;
};

/*
struct TextureNamePtrsCopyTarget // Added 2025-10-08
{
	char** textureNames;
};

struct TextureNamePtrsCopyDefinition // Added 2025-10-08
{
	const char** textureNames;
	uint32_t nameCount;
	uint32_t padding;
};*/

/// Properties of a single texture.
struct TextureDefinition
{
	/// Null-terminated ASCII string of the texture's name. Note that this pointer is only 
	/// valid as long as the respective handle is alive.
	const char* nameASCII;

	/// Texture data format
	ren::MDTextureType type;

	/// Width in pixels
	uint32_t width;

	/// Height in pixels
	uint32_t height;

	uint32_t _padding_;
};

/// Provides a pointer to the array to which the rwiGetTextureSC() function will copy the raw texture data.
struct TextureCopyTarget
{
	/// Target location to which rwiGetTextureSC() will copy the raw image data
	int8_t* data;
};

/// Contains a pointer to the arrays from which the raw texture data can be copied.
struct TextureCopyDefinition
{
	/// Location from which the raw texture data can be copied. 
	int8_t* data;

	/// Texture data format
	ren::MDTextureType type;

	/// Width in pixels
	uint32_t width;

	/// Height in pixels
	uint32_t height;

	int32_t padding;
};

/// Properties of the queried collision model.
struct CollisionDefinition
{
	/// Null-terminated ASCII string of the collision model's name. Note that this pointer is only 
	/// valid as long as the respective handle is alive.
	const char* nameASCII;
	
	uint32_t meshCount;
	uint32_t boxCount;
	uint32_t sphereCount;
	ren::CenteredBoundingBox bounds;
	int32_t padding;
};

/// Properties of the queried single collision mesh.
struct CollmeshDefinition
{
	uint32_t vertexCount;
	uint32_t triangleCount;
	int32_t materialID;
};

/// Provides pointers to the arrays to which the rwiGetCollmeshSC() function will copy the collision mesh data.
struct CollmeshCopyTarget
{
	ren::Vector3* vertices;
	ren::ShortTriangle* triangles;
};

/// Contains pointers to the arrays from which the collision mesh data can be copied.
struct CollmeshCopyDefinition
{
	const ren::Vector3* vertices;
	const ren::ShortTriangle* triangles;
	uint32_t vertexCount;
	uint32_t triangleCount;
	uint32_t materialID;
	int32_t padding;
};

/// Provides pointers to the arrays to which the rwiGetCollprimitivesSC() function will copy the collision primitives data.
struct CollprimitivesCopyTarget
{
	ren::CollBox* boxes;
	ren::CollSphere* spheres;
};

/// Contains pointers to the arrays from which the collision primitives data can be copied.
struct CollprimitivesCopyDefinition
{
	const ren::CollBox* boxes;
	const ren::CollSphere* spheres;
	uint32_t boxCount;
	uint32_t sphereCount;
};

// extern Loadthread* loadthreadObject;

extern "C" {
	///
	/// Queries the thread status.
	/// \returns REN_OK or REN_NOT_INITIALIZED
	/// 
	RENLIB renResultCode RENAPI rwiThreadStatus();

	///
	/// Commissions a load job. Note that any file you want to convert needs to be in the virtual
	/// filesystem.
	/// \param[out] handle The handle needed to retrieve the results. This handle must be freed when it's no longer needed.
	/// \param[in] filename The name of the file to be loaded and converted.
	/// \param type The type of the requested conversion.
	/// \param jobFlags Conversion flags.
	/// \returns REN_OK if the function succeeds.
	/// 
	RENLIB renResultCode RENAPI rwiEnqueueFile(renJobHandle* handle, const char* filename, ren::MDType type, ren_process_flags jobFlags);

	///
	/// Retrieves the current status of the handle.
	/// \param handle The handle received when the respective load job was commissioned.
	/// \returns REN_OK if the load job is finished and the data can be retrieved.
	/// 
	RENLIB renResultCode RENAPI rwiQueryStatus(renJobHandle handle);

	///
	/// Get a "meshmodel definition", i.e. a description of the meshmodel data. 
	/// \param handle The handle received when the respective load job was commissioned.
	/// \param[out] meshmodelDefOut Pointer to the struct that will receive the description.
	/// \returns REN_OK if the load job is finished and the data can be retrieved.
	/// 
	RENLIB renResultCode RENAPI rwiQueryMeshmodel(renJobHandle handle, MeshmodelDefinition* meshmodelDefOut);

	///
	/// Get a "mesh definition", i.e. a description of the mesh data. 
	/// \param handle The handle received when the respective load job was commissioned.
	/// \param meshIndex Index of the mesh in the meshmodel. 
	/// \param[out] meshDefOut Pointer to the struct that will receive the description.
	/// \returns REN_OK if the load job is finished and the data can be retrieved.
	/// 
	RENLIB renResultCode RENAPI rwiQueryMesh(renJobHandle handle, int32_t meshIndex, MeshDefinition* meshDefOut);

	///
	/// Retrieve all the mesh definitions in one go. This function has not been tested. 
	/// \param handle The handle received when the respective load job was commissioned.
	/// \param[out] outArray Pointer to the array of structs that will receive the mesh descriptions. 
	/// \returns REN_OK if the load job is finished and the data can be retrieved.
	/// 
	RENLIB renResultCode RENAPI rwiQueryMeshesAll(renJobHandle handle, MeshDefinition* outArray);

	///
	/// "SC" = "server copy" - You provide the pointer to an array to which the function copies the transform list.
	/// \param handle The handle received when the respective load job was commissioned.
	/// \param[in] copyTarget Pointer to the struct that cointains the destination pointer.
	/// \returns REN_OK if the function succeeds.
	/// 
	RENLIB renResultCode RENAPI rwiGetTransformsSC(renJobHandle handle, const TransformCopyTarget* copyTarget);

	///
	/// "CC" = "client copy" - You receive a pointer to an array from which you can copy the transform list.
	/// \param handle The handle received when the respective load job was commissioned.
	/// \param[out] copyDefinition Pointer to the struct that will receive the array pointer.
	/// \returns REN_OK if the function succeeds.
	/// 
	RENLIB renResultCode RENAPI rwiGetTransformsCC(renJobHandle handle, TransformCopyDefinition* copyDefinition);

	///
	/// "SC" = "server copy" - You provide the pointers to arrays to which the function copies the mesh data.
	/// \param handle The handle received when the respective load job was commissioned.
	/// \param[in] copyTarget Pointer to the struct that cointains the destination pointers.
	/// \returns REN_OK if the function succeeds.
	/// 
	RENLIB renResultCode RENAPI rwiGetMeshSC(renJobHandle handle, int32_t meshIndex, const MeshCopyTarget* copyTarget);

	///
	/// "CC" = "client copy" - You receive pointers to the arrays with the mesh data
	/// \param handle The handle received when the respective load job was commissioned.
	/// \param[out] copyDefinition Pointer to the struct that will receive the mesh data pointers (arrays).
	/// \returns REN_OK if the function succeeds.
	/// 
	RENLIB renResultCode RENAPI rwiGetMeshCC(renJobHandle handle, int32_t meshIndex, MeshCopyDefinition* copyDefinition);

	///
	/// Get a "texture set definition", i.e. a description of the texture set. 
	/// \param handle The texture set handle received when the respective load job was commissioned.
	/// \param[out] meshmodelDefOut Pointer to the struct that will receive the description.
	/// \returns REN_OK if the load job is finished and the data can be retrieved.
	/// 
	RENLIB renResultCode RENAPI rwiQueryTextureSet(renJobHandle handle, TextureSetDefinition* textureSetDefinition);

	///
	/// Get a "texture definition", i.e. the texture's properties. 
	/// \param handle The texture set handle received when the respective load job was commissioned.
	/// \param textureIndex The index of the texture to be queried. 
	/// \param[out] textureDefinition Pointer to the struct that will receive the the properties.
	/// \returns REN_OK if the load job is finished and the data can be retrieved.
	/// 
	RENLIB renResultCode RENAPI rwiQueryTexture(renJobHandle handle, int32_t textureIndex, TextureDefinition* textureDefinition);

	///
	/// Retrieve all the texture definitions of a texture set in one go. This function has not been tested. 
	/// \param handle The handle received when the respective load job was commissioned.
	/// \param[out] outArray Pointer to the array of structs that will receive the texture descriptions. 
	/// \returns REN_OK if the load job is finished and the data can be retrieved.
	/// 
	RENLIB renResultCode RENAPI rwiQueryTexturesAll(renJobHandle handle, TextureDefinition* outArray);

	///
	/// "SC" = "server copy" - You provide the pointers to arrays to which the function copies the mesh data.
	/// \param handle The handle received when the respective load job was commissioned.
	/// \param[in] copyTarget Pointer to the struct that cointains the pointers.
	/// \returns REN_OK if the function succeeds.
	/// 
	RENLIB renResultCode RENAPI rwiGetTextureSC(renJobHandle handle, int32_t textureIndex, const TextureCopyTarget* copyTarget);

	///
	/// "CC" = "client copy" - You receive a pointer to the raw texture data. 
	/// \param handle The handle received when the respective load job was commissioned.
	/// \param[out] copyDefinition Pointer to the struct that will receive the raw texture data pointer.
	/// \returns REN_OK if the function succeeds.
	/// 
	RENLIB renResultCode RENAPI rwiGetTextureCC(renJobHandle handle, int32_t textureIndex, TextureCopyDefinition* copyDefinition); 

	///
	/// Get a "collision definition", i.e. a description of the collision model. 
	/// \param handle The handle received when the respective load job was commissioned.
	/// \param[out] collisionDefOut Pointer to the struct that will receive the description.
	/// \returns REN_OK if the load job is finished and the data can be retrieved.
	/// 
	RENLIB renResultCode RENAPI rwiQueryCollision(renJobHandle handle, CollisionDefinition* collisionDefOut);

	///
	/// Get a "collision mesh definition", i.e. a description of one specific collision mesh. 
	/// \param handle The handle received when the respective load job was commissioned.
	/// \param meshIndex The index of the mesh within the collision model.
	/// \param[out] collmeshDefOut Pointer to the struct that will receive the description.
	/// \returns REN_OK if the load job is finished and the data can be retrieved.
	/// 
	RENLIB renResultCode RENAPI rwiQueryCollmesh(renJobHandle handle, int32_t meshIndex, CollmeshDefinition* collmeshDefOut);
	
	///
	/// "SC" = "server copy" - You provide the pointers to arrays to which the function copies the collision primitive lists.
	/// \param handle The handle received when the respective load job was commissioned.
	/// \param[in] copyTarget Pointer to the struct that cointains the destination pointers.
	/// \returns REN_OK if the function succeeds.
	/// 
	RENLIB renResultCode RENAPI rwiGetCollprimitivesSC(renJobHandle handle, const CollprimitivesCopyTarget* copyTarget);

	///
	/// "CC" = "client copy" - You receive pointers to arrays from which you can copy the collision primitive lists.
	/// \param handle The handle received when the respective load job was commissioned.
	/// \param[out] copyDefinition Pointer to the struct that will receive the array pointers.
	/// \returns REN_OK if the function succeeds.
	/// 
	RENLIB renResultCode RENAPI rwiGetCollprimitivesCC(renJobHandle handle, CollprimitivesCopyDefinition* copyDefinition);

	///
	/// "SC" = "server copy" - You provide the pointers to arrays to which the function copies the data of one specific collision mesh.
	/// \param handle The handle received when the respective load job was commissioned.
	/// \param meshIndex Index of the collision mesh within the collision model.
	/// \param[in] copyTarget Pointer to the struct that cointains the destination pointers.
	/// \returns REN_OK if the function succeeds.
	/// 
	RENLIB renResultCode RENAPI rwiGetCollmeshSC(renJobHandle handle, int32_t meshIndex, const CollmeshCopyTarget* copyTarget);

	///
	/// "CC" = "client copy" - You receive pointers to arrays from which you can copy the data of one specific collision mesh.
	/// \param handle The handle received when the respective load job was commissioned.
	/// \param meshIndex Index of the collision mesh within the collision model.
	/// \param[out] copyDefinition Pointer to the struct that will receive the array pointers.
	/// \returns REN_OK if the function succeeds.
	/// 
	RENLIB renResultCode RENAPI rwiGetCollmeshCC(renJobHandle handle, int32_t meshIndex, CollmeshCopyDefinition* copyDefinition);

	/// Cancels all load jobs that are not yet processing or have not yet been processed.
	RENLIB renResultCode RENAPI rwiClearQueue();

	/// 
	/// Frees a handle and the associated resources of a load job. This can only be done once a load job has been completed. Note that failed jobs have to be freed as well!
	/// \param handle The handle. 
	/// \returns REN_OK or REN_OPERATION_FAILED
	/// 
	RENLIB renResultCode RENAPI rwiFree(renJobHandle handle);

	RENLIB int32_t RENAPI rwiQueueLength();
	RENLIB int32_t RENAPI rwiFinishedJobs();

	/// 
	/// Outputs a C string to an error message that is created whenever a load job fails.
	/// \param handle The handle. 
	/// \returns Pointer to the ANSI C string containing the error message. This message is available as long as the handle is alive. 
	/// 
	RENLIB const char* RENAPI rwiGetFailMessage(renJobHandle handle);

	///
	/// Adds an IMG archive and therefore all files within it to the virtual filesystem.
	/// \param[in] filename The path to the IMG archive.
	/// \returns REN_OK or REN_FILE_CANNOT_BE_OPENED
	/// 
	RENLIB renResultCode RENAPI rwiAddArchive(const char* filename);

	///
	/// Adds a single file directly to the virtual filesystem.
	/// \param[in] filename The path to the file.
	/// \returns REN_OK or REN_FILE_CANNOT_BE_OPENED
	/// 
	RENLIB renResultCode RENAPI rwiAddFilepath(const char* filename);

	///
	/// Scans the virtual filesystem for collision models and adds them to the collision model list. This needs to be done before any collision load jobs can be commissioned. 
	/// \returns REN_OK or REN_COLLFILES_ALREADY_CACHED
	/// 
	RENLIB renResultCode RENAPI rwiCacheCollfiles();

	///
	/// Starts the conversion queue. This needs to be done before any file can be enqueued. 
	/// \returns REN_OK or REN_LOADTHREAD_ALREADY_RUNNING
	/// 
	RENLIB renResultCode RENAPI rwiStartLoadthread();

	///
	/// Schedules termination of the conversion queue. Any remaining jobs will be finished first.
	/// \returns REN_OK or REN_LOADTHREAD_NOT_RUNNING
	/// 
	RENLIB renResultCode RENAPI rwiStopLoadthread();

	///
	/// Enables the logger. 
	/// \param[in] logfileName The filename of the logfile to be created or appended. 
	/// 
	RENLIB void RENAPI rwiEnableLogging(const char* logfileName);

	/// Disables the logger once it has been active.
	RENLIB void RENAPI rwiDisableLogging();

	/// Effectively emulates the behaviour of stderr globally: Every log entry is flushed out immediately. 
	/// Really useful whenever there is a crash within the DLL.
	RENLIB void RENAPI rwiForceLogFlush();

	/// Reverses the setting of rwiForceLogFlush().
	RENLIB void RENAPI rwiNoforceLogFlush();
}