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

class FNavMeshManagerRecast
{
public:
	struct FInputMesh
	{
		std::vector<float> myVertices;
		std::vector<int> myTriangles;
		FVector3 myMin;
		FVector3 myMax;
	};

	FNavMeshManagerRecast();
	~FNavMeshManagerRecast();
	void SetDebugDrawEnabled(bool aShouldDebugDraw);
	void RemoveAllBlockingAABB() { myAABBList.clear(); }
	void Update();
	void DebugDraw();
	bool GenerateNavMesh();
	void SetInputMesh(FInputMesh& aMesh) { myInputMesh = aMesh; }
	FVector3 RayCast(FVector3 aStart, FVector3 anEnd);
	FVector3 GetClosestPointOnNavMesh(FVector3 aPoint);
	FVector3 GetClosestPointOnNavMesh(FVector3 aPoint, FVector3 aMaxExtends);
	std::vector<FVector3> FindPath(FVector3 aStart, FVector3 anEnd);
	void AddBlockingAABB(FVector3 aMin, FVector3 aMax);
	static FNavMeshManagerRecast* GetInstance();
	const std::vector<FAABB>& GetAABBList() const { return myAABBList; }

public:
	rcContext* m_ctx;
	unsigned char* m_triareas;
	rcHeightfield* m_solid;
	rcCompactHeightfield* m_chf;
	rcContourSet* m_cset;
	rcPolyMesh* m_pmesh;
	rcConfig m_cfg;
	rcPolyMeshDetail* m_dmesh;
	std::vector<FAABB> myAABBList;

	bool m_keepInterResults;
	bool m_filterLowHangingObstacles;
	bool m_filterLedgeSpans;
	bool m_filterWalkableLowHeightSpans;
	FInputMesh myInputMesh;

	class dtNavMesh* m_navMesh;
	class dtNavMeshQuery* m_navQuery;
	class dtCrowd* m_crowd;
	
	enum DebugDrawFlags : int
	{
		DRAW_POLYS = 0,
		DRAW_VERTICES,
		DRAW_PATHS,
		DRAW_EDGES
	};

	int myDebugFlags;

private:
	static FNavMeshManagerRecast* ourInstance;
};

