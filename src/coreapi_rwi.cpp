#include "pch.h"
#include "coreapi_rwi.h"
#include <cstring>
#include <ren/Logging.h>

Loadthread* loadthreadObject = nullptr;

RENLIB renResultCode RENAPI rwiThreadStatus()
{
    if (loadthreadObject) {
        return REN_OK;
    }
    else {
        return REN_NOT_INITIALIZED;
    }
}

RENLIB renResultCode RENAPI rwiEnqueueFile(renJobHandle* handle, const char* filename, ren::MDType type, ren_process_flags jobFlags)
{
    *handle = loadthreadObject->enqueueFile(filename, type, jobFlags);
    return REN_OK;
}

RENLIB renResultCode RENAPI rwiQueryStatus(renJobHandle handle)
{
    if (loadthreadObject->isFinished(handle)) {
        return REN_OK;
    }
    else if (loadthreadObject->isProcessing(handle)) {
        return REN_PROCESSING;
    }
    else if (loadthreadObject->hasFailed(handle)) {
        return REN_OPERATION_FAILED;
    }
    else {
        return REN_HANDLE_DOES_NOT_EXIST;
    }
}

RENLIB renResultCode RENAPI rwiQueryMeshmodel(renJobHandle handle, MeshmodelDefinition* meshmodelDefOut)
{
    if (meshmodelDefOut == nullptr) {
        return REN_NULLPTR_CPY_TARGET;
    }

    ren::MDataContainer* container = loadthreadObject->getItem(handle);
    if (container == nullptr) {
        return REN_HANDLE_DOES_NOT_EXIST;
    }

    ren::MeshmodelData* meshmodel;
    if (container->type == ren::MD_RWMODEL) {
        meshmodel = static_cast<ren::MeshmodelData*>(container);
    }
    else {
        return REN_DATA_FORMAT_MISMATCH;
    }
    
    meshmodelDefOut->meshCount = meshmodel->meshes.size();
    meshmodelDefOut->transformCount = meshmodel->transforms.size();
    return REN_OK;
}

RENLIB renResultCode RENAPI rwiQueryMesh(renJobHandle handle, int32_t meshIndex, MeshDefinition* meshDefOut)
{
    if (meshDefOut == nullptr) {
        return REN_NULLPTR_CPY_TARGET;
    }

    ren::MDataContainer* container = loadthreadObject->getItem(handle);
    if (container == nullptr) {
        return REN_HANDLE_DOES_NOT_EXIST;
    }

    ren::MeshmodelData* meshmodel;
    if (container->type == ren::MD_RWMODEL) {
        meshmodel = static_cast<ren::MeshmodelData*>(container);
    }
    else {
        return REN_DATA_FORMAT_MISMATCH;
    }

    if (meshIndex >= meshmodel->meshes.size()) {
        return REN_OUT_OF_BOUNDS_MESH_INDEX;
    }

    ren::MeshData* mesh = &meshmodel->meshes[meshIndex];
    meshDefOut->vertexCount = mesh->positions.size();
    meshDefOut->triangleCount = mesh->triangles.size();
    meshDefOut->materialCount = mesh->materials.size();
    meshDefOut->submeshCount = mesh->submeshes.size();
    return REN_OK;
}

RENLIB renResultCode RENAPI rwiQueryMeshesAll(renJobHandle handle, MeshDefinition* outArray)
{
    if (outArray == nullptr) {
        return REN_NULLPTR_CPY_TARGET;
    }

    ren::MDataContainer* container = loadthreadObject->getItem(handle);
    if (container == nullptr) {
        return REN_HANDLE_DOES_NOT_EXIST;
    }

    ren::MeshmodelData* meshmodel;
    if (container->type == ren::MD_RWMODEL) {
        meshmodel = static_cast<ren::MeshmodelData*>(container);
    }
    else {
        return REN_DATA_FORMAT_MISMATCH;
    }

    for (size_t meshIndex = 0; meshIndex < meshmodel->meshes.size(); meshIndex++) {
        ren::MeshData* currentMesh = &meshmodel->meshes[meshIndex];
        MeshDefinition* currentMeshDef = &outArray[meshIndex];

        currentMeshDef->vertexCount = currentMesh->positions.size();
        currentMeshDef->triangleCount = currentMesh->triangles.size();
        currentMeshDef->materialCount = currentMesh->materials.size();
        currentMeshDef->submeshCount = currentMesh->submeshes.size();
    }
    return REN_OK;
}

RENLIB renResultCode RENAPI rwiGetTransformsSC(renJobHandle handle, const TransformCopyTarget* copyTarget)
{
    if (copyTarget == nullptr) {
        return REN_NULLPTR_STRUCT;
    }

    ren::MDataContainer* container = loadthreadObject->getItem(handle);
    if (container == nullptr) {
        return REN_HANDLE_DOES_NOT_EXIST;
    }

    ren::MeshmodelData* meshmodel;
    if (container->type == ren::MD_RWMODEL) {
        meshmodel = static_cast<ren::MeshmodelData*>(container);
    }
    else {
        return REN_DATA_FORMAT_MISMATCH;
    }

    std::memcpy(copyTarget->transforms, meshmodel->transforms_blittable.data(), meshmodel->transforms_blittable.size() * sizeof(ren::Transform_blittable));
    return REN_OK;
}

RENLIB renResultCode RENAPI rwiGetTransformsCC(renJobHandle handle, TransformCopyDefinition* copyDefinition)
{
    if (copyDefinition == nullptr) {
        return REN_NULLPTR_CPY_TARGET;
    }

    ren::MDataContainer* container = loadthreadObject->getItem(handle);
    if (container == nullptr) {
        return REN_HANDLE_DOES_NOT_EXIST;
    }

    ren::MeshmodelData* meshmodel;
    if (container->type == ren::MD_RWMODEL) {
        meshmodel = static_cast<ren::MeshmodelData*>(container);
    }
    else {
        return REN_DATA_FORMAT_MISMATCH;
    }

    copyDefinition->transformCount = meshmodel->transforms.size();
    copyDefinition->transforms = meshmodel->transforms_blittable.data();
    return REN_OK;
}

RENLIB renResultCode RENAPI rwiGetMeshSC(renJobHandle handle, int32_t meshIndex, const MeshCopyTarget* copyTarget)
{
    if (copyTarget == nullptr) {
        return REN_NULLPTR_STRUCT;
    }

    ren::MDataContainer* container = loadthreadObject->getItem(handle);
    if (container == nullptr) {
        return REN_HANDLE_DOES_NOT_EXIST;
    }

    ren::MeshmodelData* meshmodel;
    if (container->type == ren::MD_RWMODEL) {
        meshmodel = static_cast<ren::MeshmodelData*>(container);
    }
    else {
        return REN_DATA_FORMAT_MISMATCH;
    }

    if (meshIndex >= meshmodel->meshes.size()) {
        return REN_OUT_OF_BOUNDS_MESH_INDEX;
    }

    ren::MeshData* mesh = &meshmodel->meshes[meshIndex];
    size_t vertexCount = mesh->positions.size();
    std::memcpy(copyTarget->vertices, mesh->positions.data(), vertexCount * sizeof(ren::Vector3));
    std::memcpy(copyTarget->UVs, mesh->UVs.data(), vertexCount * sizeof(ren::Vector2));
    std::memcpy(copyTarget->normals, mesh->normals.data(), vertexCount * sizeof(ren::Vector3));
    std::memcpy(copyTarget->colors, mesh->colors.data(), vertexCount * sizeof(ren::Vector4)); // changed 2025-10-08 for completeness
    std::memcpy(copyTarget->triangles, mesh->triangles.data(), mesh->triangles.size() * sizeof(ren::ShortTriangle));
    std::memcpy(copyTarget->materials, mesh->materials_blittable.data(), mesh->materials_blittable.size() * sizeof(ren::MaterialDescriptor_blittable));
    std::memcpy(copyTarget->submeshes, mesh->submeshes.data(), mesh->submeshes.size() * sizeof(ren::SubmeshDescriptor));
    return REN_OK;
}

RENLIB renResultCode RENAPI rwiGetMeshCC(renJobHandle handle, int32_t meshIndex, MeshCopyDefinition* copyDefinition)
{
    if (copyDefinition == nullptr) {
        return REN_NULLPTR_CPY_TARGET;
    }

    ren::MDataContainer* container = loadthreadObject->getItem(handle);
    if (container == nullptr) {
        return REN_HANDLE_DOES_NOT_EXIST;
    }

    ren::MeshmodelData* meshmodel;
    if (container->type == ren::MD_RWMODEL) {
        meshmodel = static_cast<ren::MeshmodelData*>(container);
    }
    else {
        return REN_DATA_FORMAT_MISMATCH;
    }

    if (meshIndex >= meshmodel->meshes.size()) {
        return REN_OUT_OF_BOUNDS_MESH_INDEX;
    }

    ren::MeshData* mesh = &meshmodel->meshes[meshIndex];
    copyDefinition->vertexCount = mesh->positions.size();
    copyDefinition->vertices = mesh->positions.data();
    copyDefinition->UVs = mesh->UVs.data();
    copyDefinition->normals = mesh->normals.data();
    copyDefinition->colors = mesh->colors.data();
    copyDefinition->triangleCount = mesh->triangles.size();
    copyDefinition->triangles = mesh->triangles.data();
    copyDefinition->materialCount = mesh->materials_blittable.size();
    copyDefinition->materials = mesh->materials_blittable.data();
    copyDefinition->submeshCount = mesh->submeshes.size();
    copyDefinition->submeshes = mesh->submeshes.data();
    return REN_OK;
}

RENLIB renResultCode RENAPI rwiQueryTextureSet(renJobHandle handle, TextureSetDefinition* textureSetDefinition)
{
    if (textureSetDefinition == nullptr) {
        return REN_NULLPTR_CPY_TARGET;
    }

    ren::MDataContainer* container = loadthreadObject->getItem(handle);
    if (container == nullptr) {
        return REN_HANDLE_DOES_NOT_EXIST;
    }

    ren::TextureSetData* textureSet;
    if (container->type == ren::MD_RWTEXSET) {
        textureSet = static_cast<ren::TextureSetData*>(container);
    }
    else {
        return REN_DATA_FORMAT_MISMATCH;
    }

    textureSetDefinition->textureCount = textureSet->textures.size();
    return REN_OK;
}

RENLIB renResultCode RENAPI rwiQueryTexture(renJobHandle handle, int32_t textureIndex, TextureDefinition* textureDefinition)
{
    if (textureDefinition == nullptr) {
        return REN_NULLPTR_CPY_TARGET;
    }

    ren::MDataContainer* container = loadthreadObject->getItem(handle);
    if (container == nullptr) {
        return REN_HANDLE_DOES_NOT_EXIST;
    }

    ren::TextureSetData* textureSet;
    if (container->type == ren::MD_RWTEXSET) {
        textureSet = static_cast<ren::TextureSetData*>(container);
    }
    else {
        return REN_DATA_FORMAT_MISMATCH;
    }

    if (textureIndex >= textureSet->textures.size()) {
        return REN_OUT_OF_BOUNDS_TEXTURE_INDEX;
    }

    ren::TextureData* texture = &textureSet->textures[textureIndex];
    textureDefinition->nameASCII = textureSet->names[textureIndex].c_str();
    textureDefinition->type = texture->type;
    textureDefinition->width = texture->width;
    textureDefinition->height = texture->height;
    
    return REN_OK;
}

RENLIB renResultCode RENAPI rwiQueryTexturesAll(renJobHandle handle, TextureDefinition* outArray)
{
    if (outArray == nullptr) {
        return REN_NULLPTR_CPY_TARGET;
    }

    ren::MDataContainer* container = loadthreadObject->getItem(handle);
    if (container == nullptr) {
        return REN_HANDLE_DOES_NOT_EXIST;
    }

    ren::TextureSetData* textureSet;
    if (container->type == ren::MD_RWTEXSET) {
        textureSet = static_cast<ren::TextureSetData*>(container);
    }
    else {
        return REN_DATA_FORMAT_MISMATCH;
    }

    for (size_t textureIndex = 0; textureIndex < textureSet->textures.size(); textureIndex++) {
        ren::TextureData* currentTexture = &textureSet->textures[textureIndex];
        TextureDefinition* currentTexDef = &outArray[textureIndex];

        currentTexDef->nameASCII = textureSet->names[textureIndex].c_str();
        currentTexDef->type = currentTexture->type;
        currentTexDef->width = currentTexture->width;
        currentTexDef->height = currentTexture->height;
    }

    return REN_OK;
}

RENLIB renResultCode RENAPI rwiGetTextureSC(renJobHandle handle, int32_t textureIndex, const TextureCopyTarget* copyTarget)
{
    if (copyTarget == nullptr) {
        return REN_NULLPTR_STRUCT;
    }

    ren::MDataContainer* container = loadthreadObject->getItem(handle);
    if (container == nullptr) {
        return REN_HANDLE_DOES_NOT_EXIST;
    }

    ren::TextureSetData* textureSet;
    if (container->type == ren::MD_RWTEXSET) {
        textureSet = static_cast<ren::TextureSetData*>(container);
    }
    else {
        return REN_DATA_FORMAT_MISMATCH;
    }

    if (textureIndex >= textureSet->textures.size()) {
        return REN_OUT_OF_BOUNDS_TEXTURE_INDEX;
    }

    ren::TextureData* texture = &textureSet->textures[textureIndex];
    std::memcpy(copyTarget->data, texture->data.data(), texture->data.size());
    return REN_OK;
}

RENLIB renResultCode RENAPI rwiGetTextureCC(renJobHandle handle, int32_t textureIndex, TextureCopyDefinition* copyDefinition)
{
    if (copyDefinition == nullptr) {
        return REN_NULLPTR_CPY_TARGET;
    }

    ren::MDataContainer* container = loadthreadObject->getItem(handle);
    if (container == nullptr) {
        return REN_HANDLE_DOES_NOT_EXIST;
    }

    ren::TextureSetData* textureSet;
    if (container->type == ren::MD_RWTEXSET) {
        textureSet = static_cast<ren::TextureSetData*>(container);
    }
    else {
        return REN_DATA_FORMAT_MISMATCH;
    }

    if (textureIndex >= textureSet->textures.size()) {
        return REN_OUT_OF_BOUNDS_MESH_INDEX;
    }

    ren::TextureData* texture = &textureSet->textures[textureIndex];
    copyDefinition->width = texture->width;
    copyDefinition->height = texture->height;
    copyDefinition->type = texture->type;
    copyDefinition->data = reinterpret_cast<int8_t*>(texture->data.data());
    return REN_OK;
}

RENLIB renResultCode RENAPI rwiQueryCollision(renJobHandle handle, CollisionDefinition* collisionDefOut)
{
    if (collisionDefOut == nullptr) {
        return REN_NULLPTR_CPY_TARGET;
    }

    ren::MDataContainer* container = loadthreadObject->getItem(handle);
    if (container == nullptr) {
        return REN_HANDLE_DOES_NOT_EXIST;
    }

    ren::CollisionData* collisionData;
    if (container->type == ren::MD_GTACOLLISION) {
        collisionData = static_cast<ren::CollisionData*>(container);
    }
    else {
        return REN_DATA_FORMAT_MISMATCH;
    }

    collisionDefOut->nameASCII = collisionData->name.c_str();
    collisionDefOut->meshCount = collisionData->meshes.size();
    collisionDefOut->boxCount = collisionData->boxes.size();
    collisionDefOut->sphereCount = collisionData->spheres.size();
    collisionDefOut->bounds = { (collisionData->bounds.max + collisionData->bounds.min) * 0.5f, (collisionData->bounds.max - collisionData->bounds.min) };
    return REN_OK;
}

RENLIB renResultCode RENAPI rwiQueryCollmesh(renJobHandle handle, int32_t meshIndex, CollmeshDefinition* collmeshDefOut)
{
    if (collmeshDefOut == nullptr) {
        return REN_NULLPTR_CPY_TARGET;
    }

    ren::MDataContainer* container = loadthreadObject->getItem(handle);
    if (container == nullptr) {
        return REN_HANDLE_DOES_NOT_EXIST;
    }

    ren::CollisionData* collisionData;
    if (container->type == ren::MD_GTACOLLISION) {
        collisionData = static_cast<ren::CollisionData*>(container);
    }
    else {
        return REN_DATA_FORMAT_MISMATCH;
    }

    if (meshIndex >= collisionData->meshes.size()) {
        return REN_OUT_OF_BOUNDS_MESH_INDEX;
    }

    ren::CollMeshData* collmesh = &collisionData->meshes[meshIndex];
    collmeshDefOut->vertexCount = collmesh->positions.size();
    collmeshDefOut->triangleCount = collmesh->triangles.size();
    collmeshDefOut->materialID = collmesh->material;
    return REN_OK;
}

RENLIB renResultCode RENAPI rwiGetCollprimitivesSC(renJobHandle handle, const CollprimitivesCopyTarget* copyTarget)
{
    if (copyTarget == nullptr) {
        return REN_NULLPTR_STRUCT;
    }

    ren::MDataContainer* container = loadthreadObject->getItem(handle);
    if (container == nullptr) {
        return REN_HANDLE_DOES_NOT_EXIST;
    }

    ren::CollisionData* collisionData;
    if (container->type == ren::MD_GTACOLLISION) {
        collisionData = static_cast<ren::CollisionData*>(container);
    }
    else {
        return REN_DATA_FORMAT_MISMATCH;
    }

    std::memcpy(copyTarget->boxes, collisionData->boxes.data(), collisionData->boxes.size() * sizeof(ren::CollBox));
    std::memcpy(copyTarget->spheres, collisionData->spheres.data(), collisionData->spheres.size() * sizeof(ren::CollSphere));

    return REN_OK;
}

RENLIB renResultCode RENAPI rwiGetCollprimitivesCC(renJobHandle handle, CollprimitivesCopyDefinition* copyDefinition)
{
    if (copyDefinition == nullptr) {
        return REN_NULLPTR_CPY_TARGET;
    }

    ren::MDataContainer* container = loadthreadObject->getItem(handle);
    if (container == nullptr) {
        return REN_HANDLE_DOES_NOT_EXIST;
    }

    ren::CollisionData* collisionData;
    if (container->type == ren::MD_GTACOLLISION) {
        collisionData = static_cast<ren::CollisionData*>(container);
    }
    else {
        return REN_DATA_FORMAT_MISMATCH;
    }

    copyDefinition->boxes = collisionData->boxes.data();
    copyDefinition->spheres = collisionData->spheres.data();
    copyDefinition->boxCount = collisionData->boxes.size();
    copyDefinition->sphereCount = collisionData->spheres.size();

    return REN_OK;
}

RENLIB renResultCode RENAPI rwiGetCollmeshSC(renJobHandle handle, int32_t meshIndex, const CollmeshCopyTarget* copyTarget)
{
    if (copyTarget == nullptr) {
        return REN_NULLPTR_STRUCT;
    }

    ren::MDataContainer* container = loadthreadObject->getItem(handle);
    if (container == nullptr) {
        return REN_HANDLE_DOES_NOT_EXIST;
    }

    ren::CollisionData* collisionData;
    if (container->type == ren::MD_GTACOLLISION) {
        collisionData = static_cast<ren::CollisionData*>(container);
    }
    else {
        return REN_DATA_FORMAT_MISMATCH;
    }

    if (meshIndex >= collisionData->meshes.size()) {
        return REN_OUT_OF_BOUNDS_MESH_INDEX;
    }

    ren::CollMeshData* collmeshData = &collisionData->meshes[meshIndex];
    std::memcpy(copyTarget->vertices, collmeshData->positions.data(), collmeshData->positions.size() * sizeof(ren::Vector3));
    std::memcpy(copyTarget->triangles, collmeshData->triangles.data(), collmeshData->triangles.size() * sizeof(ren::ShortTriangle));
    
    return REN_OK;
}

RENLIB renResultCode RENAPI rwiGetCollmeshCC(renJobHandle handle, int32_t meshIndex, CollmeshCopyDefinition* copyDefinition)
{
    if (copyDefinition == nullptr) {
        return REN_NULLPTR_CPY_TARGET;
    }

    ren::MDataContainer* container = loadthreadObject->getItem(handle);
    if (container == nullptr) {
        return REN_HANDLE_DOES_NOT_EXIST;
    }

    ren::CollisionData* collisionData;
    if (container->type == ren::MD_GTACOLLISION) {
        collisionData = static_cast<ren::CollisionData*>(container);
    }
    else {
        return REN_DATA_FORMAT_MISMATCH;
    }

    if (meshIndex >= collisionData->meshes.size()) {
        return REN_OUT_OF_BOUNDS_MESH_INDEX;
    }

    ren::CollMeshData* collmeshData = &collisionData->meshes[meshIndex];
    copyDefinition->vertices = collmeshData->positions.data();
    copyDefinition->triangles = collmeshData->triangles.data();
    copyDefinition->vertexCount = collmeshData->positions.size();
    copyDefinition->triangleCount = collmeshData->triangles.size();
    copyDefinition->materialID = collmeshData->material;

    return REN_OK;
}

RENLIB renResultCode RENAPI rwiClearQueue()
{
    loadthreadObject->clearQueue();
    return REN_OK;
}

RENLIB renResultCode RENAPI rwiFree(renJobHandle handle)
{
    if (loadthreadObject->freeItem(handle)) {
        return REN_OK;
    }
    else {
        return REN_OPERATION_FAILED;
    }
}

RENLIB int32_t RENAPI rwiQueueLength()
{
    return loadthreadObject->queueSize();
}

RENLIB int32_t RENAPI rwiFinishedJobs()
{
    return loadthreadObject->resultCount();
}

RENLIB const char* RENAPI rwiGetFailMessage(renJobHandle handle)
{
    return loadthreadObject->getFailMessage(handle);
}

RENLIB renResultCode RENAPI rwiAddArchive(const char* filename)
{
    if (loadthreadObject->addArchive(filename)) {
        return REN_OK;
    }
    else return REN_FILE_CANNOT_BE_OPENED;
}

RENLIB renResultCode RENAPI rwiAddFilepath(const char* filename)
{
    if (loadthreadObject->addFilepath(filename)) {
        return REN_OK;
    }
    return REN_FILE_CANNOT_BE_OPENED;
}

RENLIB renResultCode RENAPI rwiCacheCollfiles()
{
    if (loadthreadObject->cacheCollfiles()) {
        return REN_OK;
    }
    return REN_COLLFILES_ALREADY_CACHED;
}

RENLIB renResultCode RENAPI rwiStartLoadthread()
{
    if (loadthreadObject != nullptr) {
        return REN_LOADTHREAD_ALREADY_RUNNING;
    }

    loadthreadObject = new Loadthread();
    ren::Logging::log("Created Loadthread object.\n");
    return REN_OK;
}

RENLIB renResultCode RENAPI rwiStopLoadthread()
{
    if (loadthreadObject == nullptr) {
        return REN_LOADTHREAD_NOT_RUNNING;
    }

    delete loadthreadObject;
    loadthreadObject = nullptr;
    ren::Logging::log("Destroyed Loadthread object.\n");
    return REN_OK;
}

RENLIB void RENAPI rwiEnableLogging(const char* logfileName)
{
    ren::LogfileModule* module = new ren::LogfileModule("rwiLog.txt");
    ren::Logging::init(module);
}

RENLIB void RENAPI rwiDisableLogging()
{
    ren::Logging::quit();
}

RENLIB void RENAPI rwiForceLogFlush()
{
    ren::Logging::forceFlush(true);
}

RENLIB void RENAPI rwiNoforceLogFlush()
{
    ren::Logging::forceFlush(false);
}


