#pragma once
#include <vector>
#include <map>
#include <DirectXMath.h>
#include "FPrimitiveBoxInstanced.h"

class FPrimitiveBoxInstanced;



class FMeshInstanceManager
{
public:
	static unsigned int myNrOfInstancesPerPool;
	static FMeshInstanceManager* GetInstance();
	unsigned int GetMeshInstanceId(const std::string& aMeshName);
	FRenderableObjectInstanceData& GetInstanceData(const std::string& aMeshName, unsigned int anInstanceId);
	FPrimitiveBoxInstanced* GetInstance(const std::string& aMeshName, unsigned int anInstanceId);
private:
	struct MeshPool
	{
		explicit MeshPool(const std::string& aMeshName);
		bool HasFreeInstance();
		unsigned int GetNextInstanceId() { unsigned int ret = myInstanceCounter; ++myInstanceCounter; return ret; }
		FPrimitiveBoxInstanced* myPool;
		unsigned int myInstanceCounter; // @todo: implement something to reuse freed indices
	};
	FMeshInstanceManager();
	~FMeshInstanceManager();
	
	std::map<std::string, std::vector<MeshPool> > myMeshInstances;
};

