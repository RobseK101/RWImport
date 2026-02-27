#pragma once
#include <rw/BSFTree.h>
#include <glm/glm.hpp>
#include <ren/ImageFormats.hpp>
#include <ren/Logging.h>
#include <ren/Geometry.hpp>
#include <ren/ModelData.hpp>
#include <string>
#include <ren/ModelDataDefinitions.h>

/// @file
/// @brief Utilities to convert RenderWare streams found in the GTA games.

namespace ren
{
	// How to create a MeshModel? 
	// - Get the materials
	// - Get the transforms
	// - Get the meshes/geometries

	constexpr char rwDefaultShader[] = "default";

	/// @name Top level functions for data retrieval
	/// @{
	MeshData rwGeometryToMesh(rw::RWContainer& _geometryNode, ren_process_flags _processFlags = REN_NOFLAG);
	MaterialDescriptor rwMaterialToMaterial(rw::RWContainer& _materialNode);
	MeshmodelData* rwClumpToMeshmodel(rw::RWContainer& _clumpNode, ren_process_flags _processFlags = REN_NOFLAG);
	TextureSetData* rwTexdicToTextureSet(rw::RWContainer& _texdicNode);
	/// @}

	/// @name Helper functions
	/// @{
	glm::vec4 rwVertexColorToVec4(rw::RwRGBA _color);
	glm::vec4 rwMaterialColorToVec4(rw::RwRGBA _color);
	rw::BinMeshPLGResult toBinMeshPLGResult(rw::RWData& _binMeshPLGNode);
	std::vector<ShortTriangle> extractTriangles(const rw::BinMeshPLGResult& _binMeshData);
	std::vector<ShortTriangle> extractTrianglesFlipped(const rw::BinMeshPLGResult& _binMeshData);
	std::vector<SubmeshDescriptor>extractSubmeshInfo(const rw::BinMeshPLGResult& _binMeshData);
	SurfaceContainer extractSurface(const rw::BinMeshPLGResult& _binMeshData, bool _flipTriangles = false);
	size_t tricountFuncStrip(size_t _indexCount);
	size_t tricountFuncList(size_t _indexCount);
	std::vector<glm::vec3> calculateVertexNormals(const std::vector<glm::vec3>& _positions, const std::vector<ShortTriangle>& _triangles, bool _flipped = false);
	SurfaceContainer extractSurface(const rw::GeometryStructResult& _geometryStruct, size_t _materialCount, bool _flipTriangles = false);
	std::vector<glm::vec4> convertPrelitColors4(const std::vector<rw::RwRGBA>& _colors, size_t _targetSize, glm::vec4 _default = { 1.0f, 1.0f, 1.0f, 1.0f });
	std::vector<glm::vec3> convertPrelitColors3(const std::vector<rw::RwRGBA>& _colors, size_t _targetSize, glm::vec3 _default = { 1.0f, 1.0f, 1.0f });
	ImageData convertImageData(const rw::RasterStructResult& _rasterStruct);
	TextureData convertTextureData(const rw::RasterStructResult& _rasterStruct, uint32_t _rwLibVersion);
	std::vector<std::string> getFrameNameList(const rw::RWContainer& _frameList);
	std::vector<glm::vec3> flipVectorArray(const std::vector<glm::vec3>& _array);
	std::vector<glm::vec3> swapYZ(const std::vector<glm::vec3>& _array);
	glm::mat3 swapYZ(const glm::mat3& _rotationMatrix);
	glm::vec3 stripEmission(glm::vec4& _prelitVertexColor);
	std::vector<glm::vec3> stripEmission(std::vector<glm::vec4>& _prelitVertexColors);
	std::vector<Transform> pruneTransforms(const std::vector<Transform>& _input);
	/// @}
}