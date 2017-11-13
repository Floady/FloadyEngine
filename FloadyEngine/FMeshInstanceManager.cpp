#include "FMeshInstanceManager.h"
#include "FPrimitiveBoxInstanced.h"
#include "FD3d12Renderer.h"
#include "FGame.h"
#include "FUtilities.h"

static FMeshInstanceManager* ourInstance = nullptr;
unsigned int FMeshInstanceManager::myNrOfInstancesPerPool = 4;

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

FPrimitiveBoxInstanced::PerInstanceData& FMeshInstanceManager::GetInstanceData(const std::string& aMeshName, unsigned int anInstanceId)
{
	unsigned int poolIdx = floor(anInstanceId / myNrOfInstancesPerPool);
	return myMeshInstances[aMeshName][poolIdx].myPool->GetInstanceData(anInstanceId % myNrOfInstancesPerPool);
}

FPrimitiveBoxInstanced* FMeshInstanceManager::GetInstance(const std::string& aMeshName, unsigned int anInstanceId)
{
	if (myMeshInstances.find(aMeshName) == myMeshInstances.end())
	{
		FUtilities::FLog("Not found!");
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
	myPool = new FPrimitiveBoxInstanced(FGame::GetInstance()->GetRenderer(), FVector3(10,0,10), FVector3(1,1,1), FPrimitiveBoxInstanced::PrimitiveType::Sphere, myNrOfInstancesPerPool);
	FGame::GetInstance()->GetRenderer()->GetSceneGraph().AddObject(myPool, false);
}

bool FMeshInstanceManager::MeshPool::HasFreeInstance()
{
	return myInstanceCounter < myNrOfInstancesPerPool;
}
