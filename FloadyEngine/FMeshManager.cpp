#include "FMeshManager.h"
#include "FRenderableObject.h"
#include "FD3d12Renderer.h"
#include "FPrimitiveGeometry.h"
#include "FObjLoader.h"
#include "FProfiler.h"
#include <unordered_map>
#include "FJobSystem.h"
#include "FUtilities.h"

void hash_combine23(size_t &seed, size_t hash)
{
	hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
	seed ^= hash;
}

namespace std {
	template<> struct hash<FPrimitiveGeometry::Vertex2> {
		size_t operator()(FPrimitiveGeometry::Vertex2 const& vertex) const {

			size_t seed = 0;
			seed += vertex.matId;
			hash<float> hasher;
			hash_combine23(seed, hasher(vertex.position.x));
			hash_combine23(seed, hasher(vertex.position.y));
			hash_combine23(seed, hasher(vertex.position.z));
			hash_combine23(seed, hasher(vertex.normal.x));
			hash_combine23(seed, hasher(vertex.normal.y));
			hash_combine23(seed, hasher(vertex.normal.z));
			hash_combine23(seed, hasher(vertex.uv.x));
			hash_combine23(seed, hasher(vertex.uv.y));
			return seed;
		}
	};

}

FMeshManager* FMeshManager::ourInstance = nullptr;

FMeshManager::FMeshManager()
{
	myJobSystem = new FJobSystem(1);
	myJobSystem->UnPause();
}

void FMeshManager::InitLoadedMeshesD3D()
{
	int nrDone = 0;
	for (auto& item : myPendingLoadMeshes)
	{
		if (item.second.myLoadState == FMeshLoadObject::LoadState::None)
		{
			const std::string& aPath = item.first;
			item.second.myFileName = aPath;

			if(myJobSystem->QueueJob(FDelegate2<void()>(item.second, &FMeshLoadObject::Load)))
			{
				item.second.myLoadState = FMeshLoadObject::LoadState::Loading;
			}

			//LoadMeshObj(aPath);
		}
		else if (item.second.myLoadState == FMeshLoadObject::LoadState::Finished)
		{
			const std::string& aPath = item.first;
			FMeshLoadObject& loadMeshObj = myPendingLoadMeshes[aPath];
			FMeshObject* obj = loadMeshObj.myObject;

			int nrOfIndices = item.second.myIndices.size();
			HRESULT hr = FD3d12Renderer::GetInstance()->GetDevice()->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(sizeof(FPrimitiveGeometry::Vertex2) * obj->myVertices.size()),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&obj->myVertexBuffer));

			if (FAILED(hr))
			{
				FLOG("KAPUT %x", hr);
			}

			// Map the buffer
			CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
			hr = obj->myVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&obj->myVertexDataBegin));

			// Create the vertex buffer.
			{
				const UINT vertexBufferSize = sizeof(FPrimitiveGeometry::Vertex2) * obj->myVertices.size();
				memcpy(obj->myVertexDataBegin, &obj->myVertices[0], vertexBufferSize);

				// index buffer
				hr = FD3d12Renderer::GetInstance()->GetDevice()->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
					D3D12_HEAP_FLAG_NONE,
					&CD3DX12_RESOURCE_DESC::Buffer(sizeof(int) * nrOfIndices),
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(&obj->myIndexBuffer));
				
				if (FAILED(hr))
				{
					FLOG("KAPUT %x", hr);
				}

				// Map the buffer
				CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
				hr = obj->myIndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&obj->myIndexDataBegin));

				if (FAILED(hr))
				{
					FLOG("KAPUT %x", hr);
				}

				const UINT indexBufferSize = sizeof(int) * nrOfIndices;
				memcpy(obj->myIndexDataBegin, &item.second.myIndices[0], indexBufferSize);
			}
		
			FMeshObject* obj2 = myMeshes[aPath];
			obj2->myVertexBufferView.BufferLocation = obj->myVertexBuffer->GetGPUVirtualAddress();
			obj2->myVertexBufferView.StrideInBytes = sizeof(FPrimitiveGeometry::Vertex2);
			obj2->myVertexBufferView.SizeInBytes = sizeof(FPrimitiveGeometry::Vertex2) * obj->myVertices.size();
			
			obj2->myIndexBufferView.BufferLocation = obj->myIndexBuffer->GetGPUVirtualAddress();
			obj2->myIndexBufferView.Format = DXGI_FORMAT_R32_UINT;  // get from primitive manager
			obj2->myIndexBufferView.SizeInBytes = sizeof(int) * nrOfIndices;

			obj2->myIndicesCount = nrOfIndices;
			
			if(loadMeshObj.myCallBack)
				loadMeshObj.myCallBack(*loadMeshObj.myObject);

			FLOG("Mesh Done %s", aPath.c_str());
			item.second.myLoadState = FMeshLoadObject::LoadState::Done;
		 }
		
		if (item.second.myLoadState == FMeshLoadObject::LoadState::Done)
		{
			nrDone++;
		}
	}

	if(nrDone && nrDone == myPendingLoadMeshes.size())
		myPendingLoadMeshes.clear();
}

void FMeshManager::LoadMeshObj(const std::string & aPath)
{
	FMeshLoadObject& loadMeshObj = myPendingLoadMeshes[aPath];
	FMeshObject* obj = loadMeshObj.myObject;
	obj->myVertices.clear();

	FObjLoader::FObjMesh& m = obj->myMeshData;
	FObjLoader objLoader;

	std::string model = aPath;
	std::string path = "";
	path.append(model);
	objLoader.LoadObj(path.c_str(), m, "models/", true);

	std::unordered_map<FPrimitiveGeometry::Vertex2, uint32_t> uniqueVertices = {};

	std::vector<int>& indices = loadMeshObj.myIndices;
	for (const auto& shape : m.myShapes)
	{
		int idx2 = 0;
		int matId = 999;

		if (shape.mesh.material_ids.size() > 0)
			matId = shape.mesh.material_ids[0];

		for (const auto& index : shape.mesh.indices)
		{
			if ((idx2 % 3) == 0)
			{
				matId = shape.mesh.material_ids[idx2 / 3];
			}
			idx2++;

			FPrimitiveGeometry::Vertex2 vertex;

			vertex.position.x = m.myAttributes.vertices[3 * index.vertex_index + 0];
			vertex.position.y = m.myAttributes.vertices[3 * index.vertex_index + 1];
			vertex.position.z = m.myAttributes.vertices[3 * index.vertex_index + 2];

			if (m.myAttributes.normals.size() > 0)
			{
				vertex.normal.x = m.myAttributes.normals[3 * index.normal_index + 0];
				vertex.normal.y = m.myAttributes.normals[3 * index.normal_index + 1];
				vertex.normal.z = m.myAttributes.normals[3 * index.normal_index + 2];
			}

			if (m.myAttributes.texcoords.size() > 0)
			{
				vertex.uv.x = m.myAttributes.texcoords[2 * index.texcoord_index + 0];
				vertex.uv.y = 1.0f - m.myAttributes.texcoords[2 * index.texcoord_index + 1];
			}

			vertex.matId = matId == -1 ? 0 : matId;
			if (matId >= 0 && matId < m.myMaterials.size())
				vertex.normalmatId = m.myMaterials[matId].bump_texname.empty() ? 99 : matId;

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(obj->myVertices.size());
				obj->myVertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}

	loadMeshObj.myLoadState = FMeshLoadObject::LoadState::Finished;
}


FMeshManager::~FMeshManager()
{
}

FMeshManager* FMeshManager::GetInstance()
{
	if (!ourInstance)
		ourInstance = new FMeshManager();

	return ourInstance;
}

FMeshManager::FMeshObject* FMeshManager::GetMesh(const std::string & aPath, FDelegate2<void(const FMeshManager::FMeshObject&)> aCB)
{
	//FPROFILE_FUNCTION("Load mesh");

	if (myMeshes.find(aPath) != myMeshes.end())
		return myMeshes.find(aPath)->second;

	myPendingLoadMeshes[aPath] = FMeshLoadObject();
	myPendingLoadMeshes[aPath].myObject = new FMeshManager::FMeshObject();
	myPendingLoadMeshes[aPath].myCallBack = aCB;

	// set object to cube until its loaded
	FMeshManager::FMeshObject* obj = myPendingLoadMeshes[aPath].myObject;
	myMeshes[aPath] = obj;

	obj->myIndicesCount = FPrimitiveGeometry::Box2::GetIndices().size();
	obj->myVertexBuffer = FPrimitiveGeometry::Box2::GetVerticesBuffer();
	obj->myIndexBuffer = FPrimitiveGeometry::Box2::GetIndicesBuffer();
	obj->myVertexBufferView.BufferLocation = FPrimitiveGeometry::Box2::GetVerticesBuffer()->GetGPUVirtualAddress();
	obj->myVertexBufferView.StrideInBytes = FPrimitiveGeometry::Box2::GetVerticesBufferStride();
	obj->myVertexBufferView.SizeInBytes = FPrimitiveGeometry::Box2::GetVertexBufferSize();

	obj->myIndexBufferView.BufferLocation = FPrimitiveGeometry::Box2::GetIndicesBuffer()->GetGPUVirtualAddress();
	obj->myIndexBufferView.Format = DXGI_FORMAT_R32_UINT;  // get from primitive manager
	obj->myIndexBufferView.SizeInBytes = FPrimitiveGeometry::Box2::GetIndicesBufferSize();

	return obj;
}

FMeshManager::FMeshObject::FMeshObject()
{
	myVertexBuffer = nullptr;
	myIndexBuffer = nullptr;
	myVertexDataBegin = nullptr;
	myIndexDataBegin = nullptr;
	int myIndicesCount = 0;
}

void FMeshManager::FMeshLoadObject::Load()
{
	FMeshManager::GetInstance()->LoadMeshObj(myFileName);
}
