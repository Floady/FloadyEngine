#include "FMeshInstanceManager.h"
#include "FPrimitiveBoxInstanced.h"
#include "FD3d12Renderer.h"
#include "FGame.h"
#include "FUtilities.h"
#include "FPrimitiveBoxMultiTex.h"

static FMeshInstanceManager* ourInstance = nullptr;
unsigned int FMeshInstanceManager::myNrOfInstancesPerPool = 16;

FMeshInstanceManager * FMeshInstanceManager::GetInstance()
{
	if (!ourInstance)
		ourInstance = new FMeshInstanceManager();
	
	return ourInstance;
}

unsigned int FMeshInstanceManager::GetMeshInstanceId(const std::string& aMeshName)
{
	if (myMeshInstances.find(aMeshName) == myMeshInstances.end())
	{
		myMeshInstances[aMeshName].push_back(MeshPool(aMeshName));
	}

	std::vector<MeshPool>& pool = myMeshInstances[aMeshName];
	for (int i = 0; i < pool.size(); i++)
	{
		if (pool[i].HasFreeInstance())
		{
			return i * myNrOfInstancesPerPool + pool[i].GetNextInstanceId();
		}
	}

	myMeshInstances[aMeshName].push_back(MeshPool(aMeshName));
	return (myMeshInstances[aMeshName].size()-1) * myNrOfInstancesPerPool + myMeshInstances[aMeshName][myMeshInstances[aMeshName].size()-1].GetNextInstanceId();
}

FRenderableObjectInstanceData& FMeshInstanceManager::GetInstanceData(const std::string& aMeshName, unsigned int anInstanceId)
{
	unsigned int poolIdx = floor(anInstanceId / myNrOfInstancesPerPool);
	return myMeshInstances[aMeshName][poolIdx].myPool->GetInstanceData(anInstanceId % myNrOfInstancesPerPool);
}

FPrimitiveBoxInstanced* FMeshInstanceManager::GetInstance(const std::string& aMeshName, unsigned int anInstanceId)
{
	if (myMeshInstances.find(aMeshName) == myMeshInstances.end())
	{
		FLOG("Not found!");
		return nullptr;
	}

	unsigned int poolIdx = floor(anInstanceId / myNrOfInstancesPerPool);
	return myMeshInstances[aMeshName][poolIdx].myPool;
}

FMeshInstanceManager::FMeshInstanceManager()
{
}


FMeshInstanceManager::~FMeshInstanceManager()
{
}

FMeshInstanceManager::MeshPool::MeshPool(const std::string & aMeshName)
{
	myInstanceCounter = 0;
	if(aMeshName == "sphere")
		myPool = new FPrimitiveBoxInstanced(FGame::GetInstance()->GetRenderer(), FVector3(10,0,10), FVector3(1,1,1), FPrimitiveBoxInstanced::PrimitiveType::Sphere, myNrOfInstancesPerPool);
	else if (aMeshName == "box")
		myPool = new FPrimitiveBoxInstanced(FGame::GetInstance()->GetRenderer(), FVector3(10, 0, 10), FVector3(1, 1, 1), FPrimitiveBoxInstanced::PrimitiveType::Box, myNrOfInstancesPerPool);
	else
	{
		// assume its a model
		myPool = new FPrimitiveBoxMultiTex(FD3d12Renderer::GetInstance(), FVector3(10, 0, 10), FVector3(1, 1, 1), FPrimitiveBoxInstanced::PrimitiveType::Sphere, myNrOfInstancesPerPool);
		FObjLoader::FObjMesh& m = dynamic_cast<FPrimitiveBoxMultiTex*>(myPool)->myObjMesh;
		FObjLoader objLoader;
		std::string path = "models/";
		path.append(aMeshName);

		// todo change this to a mesh load object so its clear when tis alive and dead
		FMeshManager::FMeshObject* mesh = FMeshManager::GetInstance()->GetMesh(path, FDelegate2<void(const FMeshManager::FMeshObject&)>::from<FPrimitiveBoxMultiTex, &FPrimitiveBoxMultiTex::ObjectLoadingDone>(dynamic_cast<FPrimitiveBoxMultiTex*>(myPool)));
		m = mesh->myMeshData;

		dynamic_cast<FPrimitiveBoxMultiTex*>(myPool)->myMesh = mesh;

		myPool->myIndicesCount = mesh->myIndicesCount;
		myPool->m_vertexBufferView.BufferLocation = mesh->myVertexBuffer->GetGPUVirtualAddress();
		myPool->m_vertexBufferView.StrideInBytes = sizeof(FPrimitiveGeometry::Vertex2);
		myPool->m_vertexBufferView.SizeInBytes = sizeof(FPrimitiveGeometry::Vertex2) * mesh->myVerticesSize;
		
		myPool->m_indexBufferView.BufferLocation = mesh->myIndexBuffer->GetGPUVirtualAddress();
		myPool->m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;  // get from primitive manager
		myPool->m_indexBufferView.SizeInBytes = sizeof(int) * mesh->myIndicesCount;

		FLOG("PrimitiveBoxInstanced: rendering mesh %d %p %p", mesh->myVertices.size(), &mesh->myVertexBufferView, &mesh->myIndexBufferView);

		// setup AABB - TODO bug vertices = 0 here, so there wont be an AABB
		for (FPrimitiveGeometry::Vertex2& vert : mesh->myVertices)
		{
			myPool->myAABB.myMax.x = max(myPool->myAABB.myMax.x, vert.position.x * myPool->GetScale().x);
			myPool->myAABB.myMax.y = max(myPool->myAABB.myMax.y, vert.position.y * myPool->GetScale().y);
			myPool->myAABB.myMax.z = max(myPool->myAABB.myMax.z, vert.position.z * myPool->GetScale().z);
										 
			myPool->myAABB.myMin.x = min(myPool->myAABB.myMin.x, vert.position.x * myPool->GetScale().x);
			myPool->myAABB.myMin.y = min(myPool->myAABB.myMin.y, vert.position.y * myPool->GetScale().y);
			myPool->myAABB.myMin.z = min(myPool->myAABB.myMin.z, vert.position.z * myPool->GetScale().z);
		}
	}

	FGame::GetInstance()->GetRenderer()->GetSceneGraph().AddObject(myPool, false);
}

bool FMeshInstanceManager::MeshPool::HasFreeInstance()
{
	return myInstanceCounter < myNrOfInstancesPerPool;
}
