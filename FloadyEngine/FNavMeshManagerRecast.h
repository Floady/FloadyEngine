#pragma once

#include "Recast.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "Recast.h"
#include "FRenderableObject.h"
#include <vector>
#include "FVector3.h"

class FContext : public rcContext
{
	virtual void doLog(const rcLogCategory /*category*/, const char* /*msg*/, const int /*len*/) override;
};

struct FInputMesh
{
	std::vector<float> myVertices;
	std::vector<int> myTriangles;
	FVector3 myMin;
	FVector3 myMax;
};

class FNavMesh
{

public:

	FNavMesh();
	~FNavMesh();

	bool Generate(const FInputMesh& aMesh);
	FVector3 RayCast(const FVector3& aStart, const FVector3& anEnd);
	FVector3 GetClosestPointOnNavMesh(const FVector3& aPoint);
	FVector3 GetClosestPointOnNavMesh(const FVector3& aPoint, const FVector3& aMaxExtends);
	std::vector<FVector3> FindPath(const FVector3& aStart, const FVector3& anEnd);

	rcContext* myContext;
	unsigned char* myTriareas;
	rcHeightfield* mySolid;
	rcCompactHeightfield* myCompactHeightField;
	rcContourSet* myContourSet;
	rcPolyMesh* myPolyMesh;
	rcConfig myConfig;
	rcPolyMeshDetail* myDetailMesh;
	
	std::vector<FAABB> myAABBList; // do we need this per navmesh?

	bool m_keepInterResults;
	bool m_filterLowHangingObstacles;
	bool m_filterLedgeSpans;
	bool m_filterWalkableLowHeightSpans;
	
	FInputMesh myInputMesh;
	
	class dtNavMesh* myNavMesh;
	class dtNavMeshQuery* myNavQuery;
	class dtCrowd* myCrowd; // whats this for?

	unsigned char* myNavData = 0;
	
	class rcContext* m_ctx;

	bool myInitialized;
};

class FNavMeshManagerRecast
{
public:
	FNavMeshManagerRecast();
	~FNavMeshManagerRecast();
	void SetDebugDrawEnabled(bool aShouldDebugDraw);
	void RemoveAllBlockingAABB() { myAABBList.clear(); }
	void Update();
	void DebugDraw();
	bool GenerateNavMesh();
	void SetActiveNavMesh(FNavMesh* aNavMesh) { myActiveNavMesh = aNavMesh; }
	FNavMesh* GetActiveNavMesh() { return myActiveNavMesh; }
	void SetInputMesh(FInputMesh& aMesh) { myInputMesh = aMesh; }
	FVector3 RayCast(const FVector3& aStart, const FVector3& anEnd);
	FVector3 GetClosestPointOnNavMesh(const FVector3& aPoint);
	FVector3 GetClosestPointOnNavMesh(const FVector3& aPoint, const FVector3& aMaxExtends);
	std::vector<FVector3> FindPath(const FVector3& aStart, const FVector3& anEnd);
	void AddBlockingAABB(FVector3 aMin, FVector3 aMax);
	static FNavMeshManagerRecast* GetInstance();
	const std::vector<FAABB>& GetAABBList() const { return myAABBList; }

	const FInputMesh& GetInputMesh() { return myInputMesh; }
public:
	rcConfig m_cfg;
	std::vector<FAABB> myAABBList;

	FInputMesh myInputMesh;

	enum DebugDrawFlags : int
	{
		DRAW_POLYS = 0,
		DRAW_VERTICES,
		DRAW_PATHS,
		DRAW_EDGES
	};

	int myDebugFlags;

	FNavMesh* myActiveNavMesh;
private:
	static FNavMeshManagerRecast* ourInstance;
};

