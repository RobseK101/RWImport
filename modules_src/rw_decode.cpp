#include <ren/rw_decode.h>
#include <ren/Exceptions.hpp>
#include <algorithm>
#include <cstring>
#include <ren/Stringtools.hpp>
#include <ren/TransformNode.h>
#include <ren/TNodeSerialization.h>

namespace ren
{
	MeshData rwGeometryToMesh(rw::RWContainer& _geometryNode, ren_process_flags _processFlags)
	{
		using namespace rw;
		Logging::log("%s() called. Node created by lib version 0x%X; build 0x%X.\n", __FUNCTION__, libraryIDUnpackVersion(_geometryNode.libID()), libraryIDUnpackBuild(_geometryNode.libID()));

		// First check overall integrity:
		if (_geometryNode.sectionID() != RW_GEOMETRY) {
			throwException<std::logic_error>("%s(): Wrong type of node (%s).",
				__FUNCTION__, sectionName(_geometryNode.sectionID()));
		}
		size_t childCount = _geometryNode.childrenCount();
		RWData* geometryStruct = nullptr;
		if (childCount && _geometryNode[0].sectionID() == RW_STRUCT) {
			geometryStruct = static_cast<RWData*>(&_geometryNode[0]);
		}
		else {
			throwException<std::runtime_error>("%s(): Invalid RW_GEOMETRY container (no struct on first position).", __FUNCTION__);
		}

		// Retrieve geometry data:
		GeometryStructResult geometryData = RWChunk::toGeometryStruct(geometryStruct->data(), geometryStruct->dataSize(), geometryStruct->libID());
		if (_processFlags & REN_GEOMETRY_SWAP_YZ) {
			geometryData.positions = swapYZ(geometryData.positions);
			geometryData.normals = swapYZ(geometryData.normals);
			geometryData.boundingSphere.center = { geometryData.boundingSphere.center.x, geometryData.boundingSphere.center.z, geometryData.boundingSphere.center.y };
		}

		// Build GLMesh:
		MeshData result;
		
		std::vector<MaterialDescriptor> physicalMaterials;
		// Retrieve material data, if they exist. 
		int matlistIndex = _geometryNode.findFirstOf(RW_MATERIALLIST, 1);
		std::vector<MaterialIndex> materialIndices;

		if (matlistIndex >= 0) {
			RWContainer& matlistContainer = static_cast<RWContainer&>(_geometryNode[matlistIndex]);
			RWData* matlistStruct = nullptr;
			if (matlistContainer.childrenCount() && matlistContainer[0].sectionID() == RW_STRUCT) {
				matlistStruct = static_cast<RWData*>(&matlistContainer[0]);
			}
			else {
				throwException<std::runtime_error>("%s(): Invalid RW_MATERIALLIST container (no struct on first position).", __FUNCTION__);
			}
			materialIndices = RWChunk::toMaterialListStruct(matlistStruct->data(), matlistStruct->dataSize());
			for (int i = matlistContainer.findFirstOf(RW_MATERIAL, 1); i >= 0; i = matlistContainer.findFirstOf(RW_MATERIAL, i + 1)) {
				RWContainer& materialContainer = static_cast<RWContainer&>(matlistContainer[i]);
				physicalMaterials.push_back(rwMaterialToMaterial(materialContainer));
			}
		}

		// translate the material indices:
		size_t currentPhysicalMaterial = 0;
		for (size_t matIndex = 0; matIndex < materialIndices.size(); matIndex++) {
			if (materialIndices[matIndex] == -1) {
				if (currentPhysicalMaterial < physicalMaterials.size()) {
					result.materials.push_back(physicalMaterials[currentPhysicalMaterial]);
					currentPhysicalMaterial++;
				}
				else {
					throwException<std::out_of_range>("%s(): The number of declared original materials exceeds the number of material definitions.", 
						__FUNCTION__);
				}
			}
			else {
				if (materialIndices[matIndex] < result.materials.size()) {
					result.materials.push_back(result.materials[materialIndices[matIndex]]);
				}
				else {
					throwException<std::out_of_range>("%s(): Material duplicate declaration refers past itself!", __FUNCTION__);
				}
			}
		}

		// Retrieve bin mesh data, if they exist
		BinMeshPLGResult binMeshResult;
		int extensionIndex = _geometryNode.findFirstOf(RW_EXTENSION, matlistIndex + 1);
		if (extensionIndex >= 0) {
			RWContainer& extensionContainer = static_cast<RWContainer&>(_geometryNode[extensionIndex]);
			int binMeshIndex = extensionContainer.findFirstOf(RW_BINMESH_PLG, 0);
			if (binMeshIndex >= 0) {
				RWData* binMeshData = static_cast<RWData*>(&extensionContainer[binMeshIndex]);
				binMeshResult = RWChunk::toBinMeshPLG(binMeshData->data(), binMeshData->dataSize());
				Logging::log("Assign Bin Mesh Result complete!\n");
			}
		}

		SurfaceContainer surface = (binMeshResult.materialIndices.size() && binMeshResult.meshes.size()) ?
			extractSurface(binMeshResult, _processFlags & REN_GEOMETRY_FLIP_TRIANGLES) : extractSurface(geometryData, materialIndices.size(), _processFlags & REN_GEOMETRY_FLIP_TRIANGLES);
		// -> so in short, if binMeshResult is valid, use that, else go the complicated route through the geometry

		// get vertex normals and prelit colors
		std::vector<glm::vec3> vertexNormals;
		if (_processFlags & REN_GEOMETRY_FLIP_NORMALS) {
			vertexNormals = (geometryData.normals.size()) ? 
				flipVectorArray(geometryData.normals) : calculateVertexNormals(geometryData.positions, surface.triangles, false);
		}
		else {
			vertexNormals = (geometryData.normals.size()) ?
				geometryData.normals : calculateVertexNormals(geometryData.positions, surface.triangles, false);
		}
		std::vector<glm::vec4> prelitColors = convertPrelitColors4(geometryData.prelitColors, geometryData.positions.size()); // changed 2025-10-08 for completeness


		// build everything
		result.positions = std::move(geometryData.positions);
		result.UVs = std::move(geometryData.uvs);
		result.normals = std::move(vertexNormals);
		result.colors = std::move(prelitColors);
		result.triangles = std::move(surface.triangles);

		// set submeshes
		result.submeshes = std::move(surface.submeshes);

		// set materials
		result.materials;	// !!!

		return result;
	}

	MaterialDescriptor rwMaterialToMaterial(rw::RWContainer& _materialNode)
	{
		using namespace rw;
		Logging::log("%s() called.\n", __FUNCTION__);

		if (_materialNode.sectionID() != RW_MATERIAL) {
			throwException<std::logic_error>("%s(): Wrong type of node (%s).",
				__FUNCTION__, sectionName(_materialNode.sectionID()));
		}

		size_t childCount = _materialNode.childrenCount();
		RWData* materialStruct = nullptr;
		if (childCount && _materialNode[0].sectionID() == RW_STRUCT) {
			materialStruct = static_cast<RWData*>(&_materialNode[0]);
		}
		else {
			throwException<std::runtime_error>("%s(): Invalid RW_MATERIAL container (no struct on first position).", __FUNCTION__);
		}

		MaterialDescriptor result;

		CMaterialStruct rwMatStruct = RWChunk::toMaterialStruct(materialStruct->data(), materialStruct->dataSize());
		result.isTextured = 0;
		result.color = rwMaterialColorToVec4(rwMatStruct.color);
		result.ambient = rwMatStruct.ambient;
		result.specular = rwMatStruct.specular;
		result.diffuse = rwMatStruct.diffuse;

		int textureChildIndex = _materialNode.findFirstOf(RW_TEXTURE, 1);
		if (textureChildIndex >= 0) {
			RWContainer& textureNode = static_cast<RWContainer&>(_materialNode[textureChildIndex]);
			int stringChildIndex = textureNode.findFirstOf(RW_STRING);
			if (stringChildIndex >= 0) {
				RWData* textureString = static_cast<RWData*>(&textureNode[stringChildIndex]);
				result.textureName = RWChunk::rwStrToStdStr(textureString->data(), textureString->dataSize());
				result.isTextured = 1;
				Logging::log("%s(): Found texture name \"%s\" at index %d.\n", __FUNCTION__, result.textureName.c_str(), textureChildIndex);
			}
		}

		return result;
	}

	MeshmodelData* rwClumpToMeshmodel(rw::RWContainer& _clumpNode, ren_process_flags _processFlags)
	{
		using namespace rw;
		Logging::log("%s() called.\n", __FUNCTION__);

		if (_clumpNode.sectionID() != RW_CLUMP) {
			throwException<std::logic_error>("%s(): Wrong type of node (%s).",
				__FUNCTION__, sectionName(_clumpNode.sectionID()));
		}

		// decode clump struct
		size_t clumpChildCount = _clumpNode.childrenCount();
		CClumpStruct clumpStructData;
		if (clumpChildCount && _clumpNode[0].sectionID() == RW_STRUCT) {
			RWData* clumpStruct = static_cast<RWData*>(&_clumpNode[0]);
			clumpStructData = RWChunk::toClumpStruct(clumpStruct->data(), clumpStruct->dataSize());
		}
		else {
			throwException<std::runtime_error>("%s(): Invalid RW_CLUMP container (no struct on first position).", __FUNCTION__);
		}

		// get the frame list:
		std::vector<CFrameListStructEntry> frames;
		std::vector<std::string> frameNames;
		int frameListIndex = _clumpNode.findFirstOf(RW_FRAMELIST, 1);
		if (frameListIndex >= 0) {
			RWContainer& frameList = static_cast<RWContainer&>(_clumpNode[frameListIndex]);
			size_t frameListChildCount = frameList.childrenCount();
			if (frameListChildCount && frameList[0].sectionID() == RW_STRUCT) {
				RWData* frameListStruct = static_cast<RWData*>(&frameList[0]);
				frames = RWChunk::toFrameVector(frameListStruct->data(), frameListStruct->dataSize());
			}
			else {
				throwException<std::runtime_error>("%s(): Invalid RW_FRAMELIST container (no struct on first position).", __FUNCTION__);
			}
			frameNames = getFrameNameList(frameList);
			// create frame names that might be missing
			if (frameNames.size() < frames.size()) {
				size_t oldsize = frameNames.size();
				frameNames.resize(frames.size());
				for (int frameIndex = oldsize; frameIndex < frameNames.size(); frameIndex++) {
					frameNames[frameIndex] = ren::st::compose<std::string>("frame%03d", frameIndex);
				}
			}
		}

		// convert into transforms
		std::vector<Transform> transforms(frames.size());
		for (size_t transformIndex = 0; transformIndex < transforms.size(); transformIndex++) {
			Transform& currentTransform = transforms[transformIndex];
			CFrameListStructEntry& currentFrame = frames[transformIndex];

			currentTransform.name = frameNames[transformIndex];
			currentTransform.position 
				= (_processFlags & REN_GEOMETRY_SWAP_YZ) 
				? Vector3(currentFrame.position.x, currentFrame.position.z, currentFrame.position.y) 
				: currentFrame.position;
			currentTransform.rotation 
				= (_processFlags & REN_GEOMETRY_SWAP_YZ)
				? swapYZ(currentFrame.rotationMatrix)
				: currentFrame.rotationMatrix;
			currentTransform.parentIndex = currentFrame.parentIndex;
		}

		// get atomics
		std::vector<CAtomicStruct> atomics;
		for (int atomicIndex = _clumpNode.findFirstOf(RW_ATOMIC, 1); atomicIndex != -1; atomicIndex = _clumpNode.findFirstOf(RW_ATOMIC, atomicIndex + 1)) {
			RWContainer& atomic = static_cast<RWContainer&>(_clumpNode[atomicIndex]);
			size_t atomicChildCount = atomic.childrenCount();
			if (atomicChildCount && atomic[0].sectionID() == RW_STRUCT) {
				RWData* atomicStruct = static_cast<RWData*>(&atomic[0]);
				CAtomicStruct atomicData = RWChunk::toAtomicStruct(atomicStruct->data(), atomicStruct->dataSize());
				atomics.push_back(atomicData);
			}
		}
		if (atomics.size() != clumpStructData.atomicCount) {
			throwException<std::runtime_error>("%s(): Atomic count mismatch! (%d indicated, %d found)", __FUNCTION__,
				(int)clumpStructData.atomicCount, (int)atomics.size());
		}
		// this section may later implement scanning for geometry! (older RW versions)

		MeshmodelData* result = new MeshmodelData();

		// append geometry, this will not work in older versions of RW
		int geometryListIndex = _clumpNode.findFirstOf(RW_GEOMETRYLIST, 1);
		if (geometryListIndex >= 0) {
			RWContainer& geometryList = static_cast<RWContainer&>(_clumpNode[geometryListIndex]);
			// the struct isn't really that interesting this time
			for (int geometryIndex = geometryList.findFirstOf(RW_GEOMETRY, 0); geometryIndex != -1; geometryIndex = geometryList.findFirstOf(RW_GEOMETRY, geometryIndex + 1)) {
				RWContainer& geometry = static_cast<RWContainer&>(geometryList[geometryIndex]);
				result->meshes.push_back(rwGeometryToMesh(geometry, _processFlags));
			}
		}
		
		// Merge Transforms with Atomics model numbers:
		for (size_t atomicIndex = 0; atomicIndex < atomics.size(); atomicIndex++) {
			CAtomicStruct& currentAtomic = atomics[atomicIndex];
			if (currentAtomic.frameIndex < transforms.size() && currentAtomic.geometryIndex < result->meshes.size()) {
				transforms[currentAtomic.frameIndex].modelIndex = currentAtomic.geometryIndex;
			}
			else {
				throwException<std::runtime_error>(
					"%s(): Frame index (%d) or geometry index (%d) found in atomic (%d) is out of bounds (%d,%d).",
					__FUNCTION__, currentAtomic.frameIndex, currentAtomic.geometryIndex, (int)atomicIndex, 
					(int)transforms.size(), (int)result->meshes.size());
			}
		}

		if (_processFlags & REN_GEOMETRY_PRUNE_TRANSFORMS) {
			result->transforms = pruneTransforms(transforms);
		}
		else {
			result->transforms = std::move(transforms);
		}

		result->transforms_blittable.resize(result->transforms.size());
		for (size_t transformIndex = 0; transformIndex < result->transforms.size(); transformIndex++) {
			result->transforms_blittable[transformIndex] = result->transforms[transformIndex];
		}

		return result;
	}

	TextureSetData* rwTexdicToTextureSet(rw::RWContainer& _texdicNode)
	{
		using namespace rw;
		if (_texdicNode.sectionID() != RW_TEXTUREDICTIONARY) {
			throwException<std::logic_error>("%s(): Wrong type of note (%s).",
				__FUNCTION__, sectionName(_texdicNode.sectionID()));
		}

		size_t texdicChildCount = _texdicNode.childrenCount();
		CTextureDictionaryStruct texdicStructData;
		if (texdicChildCount && _texdicNode[0].sectionID() == RW_STRUCT) {
			RWData* texdicStruct = static_cast<RWData*>(&_texdicNode[0]);
			texdicStructData = RWChunk::toTextureDictionaryStruct(texdicStruct->data(), texdicStruct->dataSize());
		}
		else {
			throwException<std::runtime_error>("%s(): Node contains no struct.",
				__FUNCTION__);
		}

		TextureSetData* result = new TextureSetData();
		for (int childIndex = _texdicNode.findFirstOf(RW_RASTER, 1); childIndex != -1; childIndex = _texdicNode.findFirstOf(RW_RASTER, childIndex + 1)) {
			RWContainer& raster = static_cast<RWContainer&>(_texdicNode[childIndex]);
			if (raster.childrenCount() && raster[0].sectionID() == RW_STRUCT) {
				try {
					RWData* rasterStruct = static_cast<RWData*>(&raster[0]);
					RasterStructResult rasterData = RWChunk::toRasterStruct(rasterStruct->data(), rasterStruct->dataSize());
					TextureData texture = convertTextureData(rasterData, rasterStruct->versionNumber());
					result->addTexture(std::move(texture), rasterData.header.textureFormat.name);
				}
				catch (const std::exception& e) {
					Logging::log("%s(): Caught exception: \"%s\"\n", __FUNCTION__, e.what());
				}
				
				/*try {
					RWData* rasterStruct = static_cast<RWData*>(&raster[0]);
					RasterStructResult rasterData = RWChunk::toRasterStruct(rasterStruct->data(), rasterStruct->dataSize());
					ImageData imageData = convertImageData(rasterData);
					TextureData texture = { TYPE_RGBA, imageData.width, imageData.height };
					texture.data = std::move(imageData.pixels);
					result->addTexture(std::move(texture), rasterData.header.textureFormat.name);
				}
				catch (const std::exception& e) {
					Logging::log("%s(): Caught exception: \"%s\"\n", __FUNCTION__, e.what());
				}*/
			}
		}

		return result;
	}

	glm::vec4 rwVertexColorToVec4(rw::RwRGBA _color)
	{
		constexpr float oneBy128 = 1.0f / 128.0f;
		constexpr float oneBy255 = 1.0f / 255.0f;

		return glm::vec4(
			((_color) & 0xFF) * oneBy128,			// red
			((_color >> 8) & 0xFF) * oneBy128,		// green
			((_color >> 16) & 0xFF) * oneBy128,		// blue
			((_color >> 24) & 0xFF) * oneBy255		// alpha
		);
	}

	glm::vec4 rwMaterialColorToVec4(rw::RwRGBA _color)
	{
		constexpr float oneBy255 = 1.0f / 255.0f;

		return glm::vec4(
			((_color) & 0xFF) * oneBy255,			// red
			((_color >> 8) & 0xFF) * oneBy255,		// green
			((_color >> 16) & 0xFF) * oneBy255,		// blue
			((_color >> 24) & 0xFF) * oneBy255		// alpha
		);
	}

	rw::BinMeshPLGResult toBinMeshPLGResult(rw::RWData& _binMeshPLGNode)
	{
		using namespace rw;
		Logging::log("%s() called.\n", __FUNCTION__);

		if (_binMeshPLGNode.sectionID() != RW_BINMESH_PLG) {
			throwException<std::logic_error>("%s(): Wrong type of node (%s).",
				__FUNCTION__, sectionName(_binMeshPLGNode.sectionID()));
		}
		return RWChunk::toBinMeshPLG(_binMeshPLGNode.data(), _binMeshPLGNode.dataSize());
	}

	std::vector<ShortTriangle> extractTriangles(const rw::BinMeshPLGResult& _binMeshData)
	{
		Logging::log("%s() called.\n", __FUNCTION__);

		std::vector<ShortTriangle> result;
		if (_binMeshData.isTriangleStrip) {
			for (size_t submeshIndex = 0; submeshIndex < _binMeshData.meshes.size(); submeshIndex++) {
				for (size_t index = 2; index < _binMeshData.meshes[submeshIndex].size(); index++) {
					if (index % 2 == 0) {
						result.push_back({
							(uint16_t)_binMeshData.meshes[submeshIndex][index - 2],
							(uint16_t)_binMeshData.meshes[submeshIndex][index - 1],
							(uint16_t)_binMeshData.meshes[submeshIndex][index]
							});
					}
					else {
						result.push_back({
							(uint16_t)_binMeshData.meshes[submeshIndex][index - 1],
							(uint16_t)_binMeshData.meshes[submeshIndex][index - 2],
							(uint16_t)_binMeshData.meshes[submeshIndex][index]
							});
					}
				}
			}
		}
		else {
			for (size_t submeshIndex = 0; submeshIndex < _binMeshData.meshes.size(); submeshIndex++) {
				for (size_t index = 0; index < _binMeshData.meshes[submeshIndex].size(); index += 3) {
					result.push_back({
						(uint16_t)_binMeshData.meshes[submeshIndex][index],
						(uint16_t)_binMeshData.meshes[submeshIndex][index + 1],
						(uint16_t)_binMeshData.meshes[submeshIndex][index + 2]
						});
				}
			}
		}
		return result;
	}

	std::vector<ShortTriangle> extractTrianglesFlipped(const rw::BinMeshPLGResult& _binMeshData)
	{
		Logging::log("%s() called.\n", __FUNCTION__);

		std::vector<ShortTriangle> result;
		if (_binMeshData.isTriangleStrip) {
			for (size_t submeshIndex = 0; submeshIndex < _binMeshData.meshes.size(); submeshIndex++) {
				for (size_t index = 2; index < _binMeshData.meshes[submeshIndex].size(); index++) {
					if (index % 2 == 0) {
						result.push_back({
							(uint16_t)_binMeshData.meshes[submeshIndex][index - 1],
							(uint16_t)_binMeshData.meshes[submeshIndex][index - 2],
							(uint16_t)_binMeshData.meshes[submeshIndex][index]
							});
					}
					else {
						result.push_back({
							(uint16_t)_binMeshData.meshes[submeshIndex][index - 2],
							(uint16_t)_binMeshData.meshes[submeshIndex][index - 1],
							(uint16_t)_binMeshData.meshes[submeshIndex][index]
							});
					}
				}
			}
		}
		else {
			for (size_t submeshIndex = 0; submeshIndex < _binMeshData.meshes.size(); submeshIndex++) {
				for (size_t index = 0; index < _binMeshData.meshes[submeshIndex].size(); index += 3) {
					result.push_back({
						(uint16_t)_binMeshData.meshes[submeshIndex][index],
						(uint16_t)_binMeshData.meshes[submeshIndex][index + 2],
						(uint16_t)_binMeshData.meshes[submeshIndex][index + 1]
						});
				}
			}
		}
		return result;
	}

	size_t tricountFuncStrip(size_t _indexCount)
	{
		return _indexCount - 2;
	}

	size_t tricountFuncList(size_t _indexCount)
	{
		return _indexCount / 3;
	}

	// this algorithm might be deficient in the sense that it just averages adjacent face
	// normals without any other considerations
	std::vector<glm::vec3> calculateVertexNormals(const std::vector<glm::vec3>& _positions, const std::vector<ShortTriangle>& _triangles, bool _flipped)
	{
		Logging::log("%s() called.\n", __FUNCTION__);

		// find all occurences of each positional vector; calculate face normals
		std::vector<std::vector<size_t>> occurenceList(_positions.size());
		std::vector<glm::vec3> faceNormals(_triangles.size());
		for (size_t triangleIndex = 0; triangleIndex < _triangles.size(); triangleIndex++) {
			// discard degenerate triangles
			if (_triangles[triangleIndex].isDegenerate()) continue;

			// just write out each occurence
			for (int i = 0; i < 3; i++) {
				unsigned int index = _triangles[triangleIndex].indices[i];
				if (index < occurenceList.size()) {
					occurenceList[index].push_back(triangleIndex);
				}
				else {
					throwException<std::logic_error>("%s(): Positions/Triangles vector mismatch! (Vertex index [%d] out of bounds [%d])", 
						__FUNCTION__, index, (int)occurenceList.size());
				}
			}

			// calculate face normal; check has already been done
			if (_flipped) {
				glm::vec3 thumbVector = _positions[_triangles[triangleIndex].indices[2]] - _positions[_triangles[triangleIndex].indices[0]];
				glm::vec3 indexVector = _positions[_triangles[triangleIndex].indices[1]] - _positions[_triangles[triangleIndex].indices[0]];
				glm::vec3 middleVector = glm::cross(thumbVector, indexVector);
				faceNormals[triangleIndex] = glm::normalize(middleVector);
			}
			else {
				glm::vec3 thumbVector = _positions[_triangles[triangleIndex].indices[1]] - _positions[_triangles[triangleIndex].indices[0]];
				glm::vec3 indexVector = _positions[_triangles[triangleIndex].indices[2]] - _positions[_triangles[triangleIndex].indices[0]];
				glm::vec3 middleVector = glm::cross(thumbVector, indexVector);
				faceNormals[triangleIndex] = glm::normalize(middleVector);
			}
		}

		// calculate the vertex normals
		std::vector<glm::vec3> vertexNormals(_positions.size());
		for (size_t vertexIndex = 0; vertexIndex < vertexNormals.size(); vertexIndex++) {
			glm::vec3 sumOfNormals = { 0.0f, 0.0f, 0.0f };
			for (size_t triRefIndex = 0; triRefIndex < occurenceList[vertexIndex].size(); triRefIndex++) {
				sumOfNormals += faceNormals[occurenceList[vertexIndex][triRefIndex]];
			}
			vertexNormals[vertexIndex] = glm::normalize(sumOfNormals);
		}

		return vertexNormals;
	}

	SurfaceContainer extractSurface(const rw::GeometryStructResult& _geometryStruct, size_t _materialCount, bool _flipTriangles)
	{
		// alternative approach would be using something like std::sort
		using namespace rw;
		Logging::log("%s() called.\n", __FUNCTION__);
		Logging::log("Extracting surface from Geometry Struct Result...\n");

		std::vector<std::vector<ShortTriangle>> submeshes(_materialCount);
		for (size_t triangleIndex = 0; triangleIndex < _geometryStruct.triangles.size(); triangleIndex++) {
			const RpTriangle& currentTriangle = _geometryStruct.triangles[triangleIndex];
			if (_flipTriangles) {
				if (currentTriangle.materialID < submeshes.size()) {
					submeshes[currentTriangle.materialID].push_back({
						currentTriangle.vertex1, currentTriangle.vertex3, currentTriangle.vertex2
						});
				}
				else {
					throwException<std::runtime_error>("%s(): Invalid material index encountered! (Mat count is %d, index was %d)",
						__FUNCTION__, (int)_materialCount, (int)currentTriangle.materialID);
				}
			}
			else {
				if (currentTriangle.materialID < submeshes.size()) {
					submeshes[currentTriangle.materialID].push_back({ currentTriangle.vertex1, currentTriangle.vertex2, currentTriangle.vertex3 });
				}
				else {
					throwException<std::runtime_error>("%s(): Invalid material index encountered! (Mat count is %d, index was %d)",
						__FUNCTION__, (int)_materialCount, (int)currentTriangle.materialID);
				}
			}
		}
		SurfaceContainer result = { _geometryStruct.triangles.size(), submeshes.size() };
		size_t accumulatedCount = 0;
		for (size_t submeshIndex = 0; submeshIndex < submeshes.size(); submeshIndex++) {
			size_t currentCount = submeshes[submeshIndex].size();
			std::memcpy(&result.triangles[accumulatedCount], submeshes[submeshIndex].data(), currentCount * sizeof(ShortTriangle));
			result.submeshes[submeshIndex] = { (uint32_t)accumulatedCount, (uint32_t)currentCount, (uint32_t)submeshIndex };
			accumulatedCount += currentCount;
		}
		return result;
	}

	std::vector<glm::vec4> convertPrelitColors4(const std::vector<rw::RwRGBA>& _colors, size_t _targetSize, glm::vec4 _default)
	{
		Logging::log("%s() called.\n", __FUNCTION__);

		if (_targetSize != _colors.size() && _colors.size() > 0) {
			throwException<std::logic_error>("%s(): Colors array and target size mismatch! Zero color array first for default initialization.", __FUNCTION__);
		}
		std::vector<glm::vec4> result(_targetSize);
		if (_colors.size()) {
			for (size_t vertexIndex = 0; vertexIndex < _colors.size(); vertexIndex++) {
				result[vertexIndex] = rwVertexColorToVec4(_colors[vertexIndex]);
			}
		}
		else {
			for (size_t vertexIndex = 0; vertexIndex < _colors.size(); vertexIndex++) {
				result[vertexIndex] = _default;
			}
		}
		return result;
	}

	std::vector<glm::vec3> convertPrelitColors3(const std::vector<rw::RwRGBA>& _colors, size_t _targetSize, glm::vec3 _default)
	{
		Logging::log("%s() called.\n", __FUNCTION__);

		if (_targetSize != _colors.size() && _colors.size() > 0) {
			throwException<std::logic_error>("%s(): Colors array and target size mismatch! Zero color array first for default initialization.", __FUNCTION__);
		}
		std::vector<glm::vec3> result(_targetSize);
		if (_colors.size()) {
			for (size_t vertexIndex = 0; vertexIndex < _colors.size(); vertexIndex++) {
				result[vertexIndex] = rwVertexColorToVec4(_colors[vertexIndex]);
			}
		}
		else {
			for (size_t vertexIndex = 0; vertexIndex < result.size(); vertexIndex++) {
				result[vertexIndex] = _default;
			}
		}
		return result;
	}

	std::vector<SubmeshDescriptor>extractSubmeshInfo(const rw::BinMeshPLGResult& _binMeshData)
	{
		Logging::log("%s() called.\n", __FUNCTION__);

		size_t accumulatedTriangles = 0;
		std::vector<SubmeshDescriptor> result;

		size_t(*tricountFunction)(size_t) =
			(_binMeshData.isTriangleStrip) ? &tricountFuncStrip : &tricountFuncList;

		for (size_t submeshIndex = 0; submeshIndex < _binMeshData.meshes.size(); submeshIndex++) {
			size_t currentTriangleCount = tricountFunction(_binMeshData.meshes[submeshIndex].size());
			result.push_back({
					(uint32_t)accumulatedTriangles, (uint32_t)currentTriangleCount, _binMeshData.materialIndices[submeshIndex] });
			accumulatedTriangles += currentTriangleCount;
		}
		return result;
	}

	SurfaceContainer extractSurface(const rw::BinMeshPLGResult& _binMeshData, bool _flipTriangles)
	{
		Logging::log("%s() called.\n", __FUNCTION__);
		Logging::log("Extracting surface from Bin Mesh Result...\n");
		return (_flipTriangles) ? SurfaceContainer(extractTrianglesFlipped(_binMeshData), extractSubmeshInfo(_binMeshData)) 
			: SurfaceContainer(extractTriangles(_binMeshData), extractSubmeshInfo(_binMeshData));
	}

	ImageData convertImageData(const rw::RasterStructResult& _rasterStruct)
	{
		using namespace rw;
		Logging::log("%s() called.\n", __FUNCTION__);

		ImageData result = { _rasterStruct.header.rasterFormat.width, _rasterStruct.header.rasterFormat.height, sizeof(RWPaletteEntry), ImageData::ORDERING_REDFIRST };
		if (_rasterStruct.paletteData.size()) {
			if (_rasterStruct.header.rasterFormat.format.extendedFormat & RASTEREXT_PAL8) {
				expand8BitIndexed(
					result.pixels.data(), result.pixels.size(),
					_rasterStruct.imageData.data(), _rasterStruct.imageData.size(),
					_rasterStruct.paletteData.data(), _rasterStruct.paletteData.size(), sizeof(RWPaletteEntry));
			}
			else if (_rasterStruct.header.rasterFormat.format.extendedFormat & RASTEREXT_PAL4) {
				expand4BitIndexed(
					result.pixels.data(), result.pixels.size(),
					_rasterStruct.imageData.data(), _rasterStruct.imageData.size(),
					_rasterStruct.paletteData.data(), _rasterStruct.paletteData.size(), sizeof(RWPaletteEntry));
			}
			else {
				throwException<std::logic_error>("%s(): Invalid flag combination.", __FUNCTION__);
			}
			if (_rasterStruct.header.rasterFormat.format.baseFormat == RASTERBASE_888) {
				for (size_t rowIndex = 0; rowIndex < result.height; rowIndex++) {
					for (size_t elementIndex = 0; elementIndex < result.width; elementIndex++) {
						ColorRGBA currentColor = result.getPixelR(elementIndex, rowIndex);
						currentColor.dataAlpha(0xFF);
						result.putPixelR(elementIndex, rowIndex, currentColor);
					}
				}
			}
			return result;
		}
		else if (_rasterStruct.header.rasterFormat.compression) {
			throwException<std::logic_error>("%s(): Decoding compressed images is not yet supported \"%s\".", __FUNCTION__,
				_rasterStruct.header.textureFormat.name);
		}
		else {
			throwException<std::logic_error>("%s(): UNSUPPORTED \"%s\".", __FUNCTION__, 
				_rasterStruct.header.textureFormat.name);
		}
	}

	TextureData convertTextureData(const rw::RasterStructResult& _rasterStruct, uint32_t _rwLibVersion)
	{
		using namespace rw;
		Logging::log("%s() called.\n", __FUNCTION__);

		if (_rwLibVersion < 0x36000) { // i.e. Pre-SA
			if (_rasterStruct.header.rasterFormat.compression == 0) {
				ImageData resultImageData = { _rasterStruct.header.rasterFormat.width, _rasterStruct.header.rasterFormat.height,
					sizeof(RWPaletteEntry), ImageData::ORDERING_REDFIRST };
				MDTextureType textureType;
				if (_rasterStruct.header.rasterFormat.format.extendedFormat & RASTEREXT_PAL8) {
					expand8BitIndexed(resultImageData.pixels.data(), resultImageData.pixels.size(), _rasterStruct.imageData.data(), _rasterStruct.imageData.size(),
						_rasterStruct.paletteData.data(), _rasterStruct.paletteData.size(), sizeof(RWPaletteEntry));
					textureType = TYPE_RGBA;
				}
				else if (_rasterStruct.header.rasterFormat.format.extendedFormat & RASTEREXT_PAL4) {
					expand4BitIndexed(resultImageData.pixels.data(), resultImageData.pixels.size(), _rasterStruct.imageData.data(), _rasterStruct.imageData.size(),
						_rasterStruct.paletteData.data(), _rasterStruct.paletteData.size(), sizeof(RWPaletteEntry));
					textureType = TYPE_RGBA;
				}
				else if (_rasterStruct.header.rasterFormat.format.baseFormat == RASTERBASE_8888) {
					resultImageData.orderingColor = ImageData::ORDERING_BLUEFIRST;
					std::memcpy(resultImageData.pixels.data(), _rasterStruct.imageData.data(), _rasterStruct.imageData.size());
					textureType = TYPE_BGRA;
				}
				else {
					throwException<std::runtime_error>("%s(): Unsupported Raster Format.", __FUNCTION__);
				}
				if (_rasterStruct.header.rasterFormat.format.baseFormat == RASTERBASE_888) {
					for (size_t rowIndex = 0; rowIndex < resultImageData.height; rowIndex++) {
						for (size_t elementIndex = 0; elementIndex < resultImageData.width; elementIndex++) {
							ColorRGBA currentColor = resultImageData.getPixelR(elementIndex, rowIndex);
							currentColor.dataAlpha(0xFF);
							resultImageData.putPixelR(elementIndex, rowIndex, currentColor);
						}
					}
				}
				TextureData resultTextureData = { textureType, resultImageData.width, resultImageData.height };
				resultTextureData.data = std::move(resultImageData.pixels);
				return resultTextureData;
			}
			else {
				throwException<std::runtime_error>("%s(): Compressed Raster Formats are unsupported.", __FUNCTION__);
			}
		}
		else {
			throwException<std::exception>("San Andreas Raster formats are not implemented yet.");
		}
	}

	// saved for backup:
	/*ImageData result = { _rasterStruct.header.rasterFormat.width, _rasterStruct.header.rasterFormat.height, sizeof(RWPaletteEntry), ImageData::ORDERING_REDFIRST };
		if (_rasterStruct.paletteData.size()) {
			if (_rasterStruct.header.rasterFormat.format.extendedFormat & RASTEREXT_PAL8) {
				expand8BitIndexed(
					result.pixels.data(), result.pixels.size(),
					_rasterStruct.imageData.data(), _rasterStruct.imageData.size(),
					_rasterStruct.paletteData.data(), _rasterStruct.paletteData.size(), sizeof(RWPaletteEntry));
			}
			else if (_rasterStruct.header.rasterFormat.format.extendedFormat & RASTEREXT_PAL4) {
				expand4BitIndexed(
					result.pixels.data(), result.pixels.size(),
					_rasterStruct.imageData.data(), _rasterStruct.imageData.size(),
					_rasterStruct.paletteData.data(), _rasterStruct.paletteData.size(), sizeof(RWPaletteEntry));
			}
			else {
				throwException<std::logic_error>("%s(): Invalid flag combination.", __FUNCTION__);
			}
			if (_rasterStruct.header.rasterFormat.format.baseFormat == RASTERBASE_888) {
				for (size_t rowIndex = 0; rowIndex < result.height; rowIndex++) {
					for (size_t elementIndex = 0; elementIndex < result.width; elementIndex++) {
						ColorRGBA currentColor = result.getPixelR(elementIndex, rowIndex);
						currentColor.dataAlpha(0xFF);
						result.putPixelR(elementIndex, rowIndex, currentColor);
					}
				}
			}
			return result;
		}
		else if (_rasterStruct.header.rasterFormat.compression) {
			throwException<std::logic_error>("%s(): Decoding compressed images is not yet supported.", __FUNCTION__);
		}
		else {
			throwException<std::logic_error>("%s(): UNSUPPORTED.", __FUNCTION__);
		}
	}*/

	std::vector<std::string> getFrameNameList(const rw::RWContainer& _frameList)
	{
		using namespace rw;

		std::vector<std::string> result;
		for (
			int extensionIndex = _frameList.findFirstOf(RW_EXTENSION, 0); 
			extensionIndex != -1; 
			extensionIndex = _frameList.findFirstOf(RW_EXTENSION, extensionIndex + 1)
			) {
			const RWContainer& extension = static_cast<const RWContainer&>(_frameList[extensionIndex]);
			int frameNameIndex = extension.findFirstOf(RG_FRAME, 0);
			if (frameNameIndex != -1) {
				const RWData& frameName = static_cast<const RWData&>(extension[frameNameIndex]);
				result.push_back(RWChunk::frameToStdString(frameName.data(), frameName.dataSize()));
			}
		}
		return result;
	}

	std::vector<glm::vec3> flipVectorArray(const std::vector<glm::vec3>& _array)
	{
		std::vector<glm::vec3> result(_array.size());
		for (size_t i = 0; i < result.size(); i++) {
			result[i] = -_array[i];
		}
		return result;
	}

	std::vector<glm::vec3> swapYZ(const std::vector<glm::vec3>& _array)
	{
		std::vector<glm::vec3> result(_array.size());
		for (size_t i = 0; i < result.size(); i++) {
			result[i] = { _array[i].x, _array[i].z, _array[i].y };
		}
		return result;
	}

	glm::mat3 swapYZ(const glm::mat3& _rotationMatrix)
	{
		// Note: Since this is glm, this is column-major (Code lines are matrix columns)!
		// However, since this matrix is symmetrical, it doesn't really matter.
		// See comment after function.
		constexpr glm::mat3 permutationMatrix = 
		{1.0f, 0.0f, 0.0f, 
		 0.0f, 0.0f, 1.0f,
		 0.0f, 1.0f, 0.0f};

		return permutationMatrix * _rotationMatrix * permutationMatrix;
	}

	/*
	------------------------------------------------------------
	 Swapping Y and Z axes for vectors and rotation matrices (ChatGPT)
	------------------------------------------------------------

	1. Position vector:
	   p' = S * p

	   where
		   S = [ 1  0  0 ]
			   [ 0  0  1 ]
			   [ 0  1  0 ]

	   This simply swaps y and z:
		   p' = ( x, z, y )

	------------------------------------------------------------
	2. Rotation matrix:
	   R represents orientation in 3D space.
	   Depending on your goal, you can use one of the following:

	   (A) Reinterpret rotation under swapped coordinate convention:
		   R' = S * R * S
		   -> use when converting between Y-up and Z-up systems
			  (i.e. changing coordinate basis)

	   (B) Apply swap AFTER rotation:
		   R' = S * R
		   -> swaps output axes (object space)

	   (C) Apply swap BEFORE rotation:
		   R' = R * S
		   -> swaps input axes (coordinate space)

	   Note:
		 S is orthogonal, so S^-1 = S^T = S.

	------------------------------------------------------------
	 Summary:
	   Goal                                  | Formula
	   ------------------------------------- | ----------------
	   Change coordinate convention          | R' = S * R * S
	   Swap output axes (after rotation)     | R' = S * R
	   Swap input axes  (before rotation)    | R' = R * S
	------------------------------------------------------------
	*/

	glm::vec3 stripEmission(glm::vec4& _prelitVertexColor)
	{
		glm::vec3 emission = {
			glm::clamp(_prelitVertexColor.x, 1.0f, 2.0f) - 1.0f,
			glm::clamp(_prelitVertexColor.y, 1.0f, 2.0f) - 1.0f,
			glm::clamp(_prelitVertexColor.z, 1.0f, 2.0f) - 1.0f};

		_prelitVertexColor.x = glm::clamp(_prelitVertexColor.x, 0.0f, 1.0f);
		_prelitVertexColor.y = glm::clamp(_prelitVertexColor.y, 0.0f, 1.0f);
		_prelitVertexColor.z = glm::clamp(_prelitVertexColor.z, 0.0f, 1.0f);

		return emission;
	}

	std::vector<glm::vec3> stripEmission(std::vector<glm::vec4>& _prelitVertexColors)
	{
		std::vector<glm::vec3> emissionColors(_prelitVertexColors.size());

		for (size_t i = 0; i < emissionColors.size(); i++) {
			emissionColors[i] = stripEmission(_prelitVertexColors[i]);
		}

		return emissionColors;
	}

		/*
	std::vector<Transform> pruneTransforms(const std::vector<Transform>& _input)
	{
		Logging::log("Pruning transforms...\n");
		TransformNode* rootNode = TransformNode::rebuildHierarchy(_input);
		Logging::log("Root node child count before pruning: %d\n", (int)rootNode->childCount());
		rootNode->prune();
		Logging::log("Root node child count after pruning: %d\n", (int)rootNode->childCount());
		std::vector<Transform> result = TransformNode::serializeToFlatList(rootNode);
		rootNode->destroy();
		return result;
	}
		*/

	std::vector<Transform> pruneTransforms(const std::vector<Transform>& _input) 
	{
		Logging::log("Pruning transforms...\n");
		TransformNode* rootNode = rebuildTNodeHierarchy(_input);
		Logging::log("Root node child count before pruning: %d\n", (int)rootNode->childCount());
		rootNode->prune();
		Logging::log("Root node child count after pruning: %d\n", (int)rootNode->childCount());
		std::vector<Transform> result = serializeTNodeToTransformList(rootNode);
		rootNode->destroy();
		return result;
	}
}