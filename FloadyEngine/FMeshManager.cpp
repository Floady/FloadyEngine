#include "FMeshManager.h"
#include "FD3d12Renderer.h"
#include "FRenderableObject.h"
#include "FPrimitiveGeometry.h"
#include "FProfiler.h"
#include <unordered_map>
#include "FJobSystem.h"
#include "FUtilities.h"
#include "F3DModel.h"

#pragma optimize("", off)

FMeshManager* FMeshManager::ourInstance = nullptr;

FMeshManager::FMeshManager()
{
	myJobSystem = FJobSystem::GetInstance();
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

			if(myJobSystem->QueueJob(FDelegate2<void()>(item.second, &FMeshLoadObject::Load), true))
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

			//* // TEST copy CPU vertex data from loadObj to GPU struct
			
			obj->myVertices.clear();
			for (size_t i = 0; i < loadMeshObj.myModel.myVertices.size(); i++)
			{
				obj->myVertices.push_back(FPrimitiveGeometry::Vertex2(loadMeshObj.myModel.myVertices[i]));
			}

			obj->myIndices.clear();
			for (size_t i = 0; i < loadMeshObj.myModel.myIndices.size(); i++)
			{
				obj->myIndices.push_back(loadMeshObj.myModel.myIndices[i]);
			}

			//*/


			int nrOfIndices = obj->myIndices.size();
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
				memcpy(obj->myIndexDataBegin, &obj->myIndices[0], indexBufferSize);
			}
		
			FMeshObject* obj2 = myMeshes[aPath];
			obj2->myVertexBufferView.BufferLocation = obj->myVertexBuffer->GetGPUVirtualAddress();
			obj2->myVertexBufferView.StrideInBytes = sizeof(FPrimitiveGeometry::Vertex2);
			obj2->myVertexBufferView.SizeInBytes = sizeof(FPrimitiveGeometry::Vertex2) * obj->myVertices.size();
			
			obj2->myIndexBufferView.BufferLocation = obj->myIndexBuffer->GetGPUVirtualAddress();
			obj2->myIndexBufferView.Format = DXGI_FORMAT_R32_UINT;  // get from primitive manager
			obj2->myIndexBufferView.SizeInBytes = sizeof(int) * nrOfIndices;

			obj2->myIndicesCount = nrOfIndices;
			obj2->myVerticesSize = obj->myVertices.size();
			
			if(loadMeshObj.myCallBack)
				loadMeshObj.myCallBack(loadMeshObj);

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

	std::string model = aPath;
	std::string path = "";
	path.append(model);

	loadMeshObj.myModel.Load(path.c_str());
	loadMeshObj.myLoadState = FMeshLoadObject::LoadState::Finished;
}


FMeshManager::~FMeshManager()
{
	for (auto& obj : myMeshes)
	{
		delete obj.second;
	}
}

FMeshManager* FMeshManager::GetInstance()
{
	if (!ourInstance)
		ourInstance = new FMeshManager();

	return ourInstance;
}

FMeshManager::FMeshObject* FMeshManager::GetMesh(const std::string & aPath, FDelegate2<void(const FMeshManager::FMeshLoadObject&)> aCB)
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

	obj->myIndicesCount = FPrimitiveGeometry::DefaultModel::GetIndices().size();
	obj->myVertexBuffer = FPrimitiveGeometry::DefaultModel::GetVerticesBuffer();
	obj->myIndexBuffer = FPrimitiveGeometry::DefaultModel::GetIndicesBuffer();
	obj->myVerticesSize = FPrimitiveGeometry::DefaultModel::GetVertices().size();
	obj->myVertexBufferView.BufferLocation = FPrimitiveGeometry::DefaultModel::GetVerticesBuffer()->GetGPUVirtualAddress();
	obj->myVertexBufferView.StrideInBytes = FPrimitiveGeometry::DefaultModel::GetVerticesBufferStride();
	obj->myVertexBufferView.SizeInBytes = FPrimitiveGeometry::DefaultModel::GetVertexBufferSize();

	obj->myIndexBufferView.BufferLocation = FPrimitiveGeometry::DefaultModel::GetIndicesBuffer()->GetGPUVirtualAddress();
	obj->myIndexBufferView.Format = DXGI_FORMAT_R32_UINT;  // get from primitive manager
	obj->myIndexBufferView.SizeInBytes = FPrimitiveGeometry::DefaultModel::GetIndicesBufferSize();

	return obj;
}

FMeshManager::FMeshObject::FMeshObject()
{
	myVertexBuffer = nullptr;
	myIndexBuffer = nullptr;
	myVertexDataBegin = nullptr;
	myIndexDataBegin = nullptr;
	myIndicesCount = 0;
	myVerticesSize = 0;
}

FMeshManager::FMeshObject::~FMeshObject()
{
	if (myVertexBuffer)
		myVertexBuffer->Release();

	if (myIndexBuffer)
		myIndexBuffer->Release();

	myIndexBuffer = nullptr;
	myVertexDataBegin = nullptr;
	myIndexDataBegin = nullptr;
	myIndicesCount = 0;
	myVerticesSize = 0;
}

void FMeshManager::FMeshLoadObject::Load()
{
	FMeshManager::GetInstance()->LoadMeshObj(myFileName);
}
