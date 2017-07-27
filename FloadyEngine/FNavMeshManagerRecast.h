#pragma once

#include "Recast.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "Recast.h"
#include <vector>
#include "FVector3.h"

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
	void Update();
	void DebugDraw();
	bool GenerateNavMesh();
	void SetInputMesh(FInputMesh& aMesh) { myInputMesh = aMesh; }
	FVector3 RayCast(FVector3 aStart, FVector3 anEnd);
	FVector3 GetClosestPointOnNavMesh(FVector3 aPoint);
	std::vector<FVector3> FindPath(FVector3 aStart, FVector3 anEnd);

	static FNavMeshManagerRecast* GetInstance();
public:
	rcContext* m_ctx;
	unsigned char* m_triareas;
	rcHeightfield* m_solid;
	rcCompactHeightfield* m_chf;
	rcContourSet* m_cset;
	rcPolyMesh* m_pmesh;
	rcConfig m_cfg;
	rcPolyMeshDetail* m_dmesh;
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

