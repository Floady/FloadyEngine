#pragma once

#include <map>
#include <string>
#include <vector>
#include "FPrimitiveGeometry.h"
#include "F3DModel.h"

class FJobSystem;

class FMeshManager
{
public:
	FMeshManager();
	void InitLoadedMeshesD3D();
	void LoadMeshObj(const std::string& aPath);
	~FMeshManager();
	static FMeshManager* GetInstance();

	struct FMeshObject 
	{
		FMeshObject();
		~FMeshObject();
		ID3D12Resource* myVertexBuffer;
		ID3D12Resource* myIndexBuffer;
		UINT8* myVertexDataBegin;
		UINT8* myIndexDataBegin;
		int myIndicesCount;
		int myVerticesSize;
		D3D12_VERTEX_BUFFER_VIEW myVertexBufferView;
		D3D12_INDEX_BUFFER_VIEW myIndexBufferView;
		std::vector<FPrimitiveGeometry::Vertex2> myVertices;
		std::vector<int> myIndices;
	};

	struct FMeshLoadObject
	{
		enum LoadState
		{
			None,
			Loading,
			Finished,
			Done,
		};
		FMeshLoadObject() { myLoadState = None; myObject = nullptr; }
		void Load();

		std::string myFileName;
		LoadState myLoadState;
		FDelegate2<void(const FMeshManager::FMeshLoadObject&)> myCallBack;
		F3DModel myModel; // CPU representation
		FMeshObject* myObject; // GPU representation
	};

	FMeshObject* GetMesh(const std::string& aPath, FDelegate2<void(const FMeshManager::FMeshLoadObject&)> aCB = FDelegate2<void(const FMeshManager::FMeshLoadObject&)>::FDelegate2());

private:
	std::map<std::string, FMeshObject*> myMeshes;
	std::map<std::string, FMeshLoadObject> myPendingLoadMeshes;
	static FMeshManager* ourInstance;
	FJobSystem* myJobSystem;
};

