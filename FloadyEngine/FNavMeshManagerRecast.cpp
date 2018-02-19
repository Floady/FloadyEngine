#include "FNavMeshManagerRecast.h"
#include <windows.h>
#include <math.h>
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourNavMeshQuery.h"
#include <vector>
#include "FD3d12Renderer.h"
#include "FDebugDrawer.h"
#include "FUtilities.h"

FNavMeshManagerRecast* FNavMeshManagerRecast::ourInstance = nullptr;

FNavMeshManagerRecast::FNavMeshManagerRecast()
{
	m_keepInterResults = true;
	m_filterLowHangingObstacles = false;
	m_filterLedgeSpans = false;
	m_filterWalkableLowHeightSpans = false;
	myDebugFlags = 0;
	m_navQuery = nullptr;
}


FNavMeshManagerRecast::~FNavMeshManagerRecast()
{
}

void FNavMeshManagerRecast::SetDebugDrawEnabled(bool aShouldDebugDraw)
{
	if(aShouldDebugDraw)
		myDebugFlags = (1 << DebugDrawFlags::DRAW_POLYS) | (1 << DebugDrawFlags::DRAW_VERTICES) | (1 << DebugDrawFlags::DRAW_PATHS) | (1 << DebugDrawFlags::DRAW_EDGES);
	else
		myDebugFlags = 0;
}

void FNavMeshManagerRecast::Update()
{
}

void FNavMeshManagerRecast::DebugDraw()
{
	// myDebugFlags = (1 << DebugDrawFlags::DRAW_POLYS) | (1 << DebugDrawFlags::DRAW_PATHS);

	FDebugDrawer* debugDrawer = FD3d12Renderer::GetInstance()->GetDebugDrawer();

	if (!debugDrawer || !m_pmesh || !myDebugFlags)
		return;
	
	const int nvp = m_pmesh->nvp;
	const float cs = m_pmesh->cs;
	const float ch = m_pmesh->ch;
	const float* orig = m_pmesh->bmin;

	if (myDebugFlags & (1 << DebugDrawFlags::DRAW_POLYS))
	{
		/*
		for (int i = 0; i < m_pmesh->npolys; ++i)
		{
			const unsigned short* p = &m_pmesh->polys[i*nvp * 2];
			const unsigned char area = m_pmesh->areas[i];

			FVector3 color;

			// randomize color triangle to see the mesh
			float percent = i / (float)(m_pmesh->npolys);
			float g =  0.1f + ((i % 8) / 8.0f);
			float r =  1.0f - (0.1f + (((i + 4) % 8) / 8.0f));

			if (area == RC_WALKABLE_AREA)
				color = FVector3(r, g, 0.3);
			else if (area == RC_NULL_AREA)
				color = FVector3(0.8, 0.3, 0.3);
			else
				color = FVector3(0, 0, 1);

			//color = FVector3(r, g, 0.3);

			unsigned short vi[3];
			for (int j = 2; j < nvp; ++j)
			{
				if (p[j] == RC_MESH_NULL_IDX) break;
				vi[0] = p[0];
				vi[1] = p[j - 1];
				vi[2] = p[j];

				FVector3 vtx[3];
				int vtxIndex = 0;
				for (int k = 0; k < 3; ++k)
				{
					const unsigned short* v = &m_pmesh->verts[vi[k] * 3];
					const float x = orig[0] + v[0] * cs;
					const float y = orig[1] + (v[1] + 1)*ch;
					const float z = orig[2] + v[2] * cs;
					vtx[vtxIndex].x = x;
					vtx[vtxIndex].y = y;
					vtx[vtxIndex].z = z;
					vtxIndex++;
				}

				debugDrawer->DrawTriangle(vtx[0], vtx[1], vtx[2], color);
			}
		}
		*/

		const dtNavMesh* navmesh = static_cast<const dtNavMesh*>(m_navMesh);
		for (int i = 0; i < navmesh->getMaxTiles(); ++i)
		{
			const dtMeshTile* tile = navmesh->getTile(i);
			if (!tile->header) continue;
			for (int i = 0; i < tile->header->polyCount; ++i)
			{
				const dtPoly* p = &tile->polys[i];
				if (p->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)	// Skip off-mesh links.
					continue;

				const dtPolyDetail* pd = &tile->detailMeshes[i];

				FVector3 colorNav(0.5f,0.8f,0.5f);
				dtPolyRef base = navmesh->getPolyRefBase(tile);
				const dtNavMeshQuery* query = nullptr;
				unsigned char flags = 0;
				if (query && query->isInClosedList(base | (dtPolyRef)i))
					colorNav = FVector3(1, 0.8f, 0);
				else
				{
					if (p->getArea() == 1) // these flags are game specific - todo improve with enums and area costs
						colorNav = FVector3(0.8f, 0.2f, 0.2f);
					/*
					if (flags & DU_DRAWNAVMESH_COLOR_TILES) //DU_DRAWNAVMESH_COLOR_TILES
						col = tileColor;
					else
						col = duTransCol(dd->areaToCol(p->getArea()), 64);
					*/
				}

				for (int j = 0; j < pd->triCount; ++j)
				{
					FVector3 vtx[3];
					int vtxIndex = 0;
					const unsigned char* t = &tile->detailTris[(pd->triBase + j) * 4];
					for (int k = 0; k < 3; ++k)
					{
						if (t[k] < p->vertCount)
						{
							vtx[vtxIndex].x = tile->verts[p->verts[t[k]] * 3];
							vtx[vtxIndex].y = tile->verts[p->verts[t[k]] * 3 + 1];
							vtx[vtxIndex].z = tile->verts[p->verts[t[k]] * 3 + 2];
						}
						else
						{
							vtx[vtxIndex].x = tile->detailVerts[(pd->vertBase + t[k] - p->vertCount) * 3];
							vtx[vtxIndex].y = tile->detailVerts[(pd->vertBase + t[k] - p->vertCount) * 3 + 1];
							vtx[vtxIndex].z = tile->detailVerts[(pd->vertBase + t[k] - p->vertCount) * 3 + 2];
						}

						vtxIndex++;
					}

					debugDrawer->DrawTriangle(vtx[0], vtx[1], vtx[2], colorNav);
				}
			}
		}
	}

	// Draw neighbours edges
	if (myDebugFlags & (1 << DebugDrawFlags::DRAW_EDGES))
	{
		FVector3 colorNeighbor(1, 1, 1);
		for (int i = 0; i < m_pmesh->npolys; ++i)
		{
			const unsigned short* p = &m_pmesh->polys[i*nvp * 2];
			for (int j = 0; j < nvp; ++j)
			{
				if (p[j] == RC_MESH_NULL_IDX) break;
				if (p[nvp + j] & 0x8000) continue;
				const int nj = (j + 1 >= nvp || p[j + 1] == RC_MESH_NULL_IDX) ? 0 : j + 1;
				const int vi[2] = { p[j], p[nj] };

				FVector3 vtx[2];
				int vtxIndex = 0;
				for (int k = 0; k < 2; ++k)
				{
					const unsigned short* v = &m_pmesh->verts[vi[k] * 3];
					const float x = orig[0] + v[0] * cs;
					const float y = orig[1] + (v[1] + 1)*ch + 0.1f;
					const float z = orig[2] + v[2] * cs;
					vtx[vtxIndex].x = x;
					vtx[vtxIndex].y = y;
					vtx[vtxIndex].z = z;
					vtxIndex++;
				}
				debugDrawer->drawLine(vtx[0], vtx[1], colorNeighbor);
			}
		}
	}

	if (myDebugFlags & (1 << DebugDrawFlags::DRAW_EDGES))
	{
		// Draw boundary edges
		FVector3 colorBoundary(0, 1, 0);

		for (int i = 0; i < m_pmesh->npolys; ++i)
		{
			const unsigned short* p = &m_pmesh->polys[i*nvp * 2];
			for (int j = 0; j < nvp; ++j)
			{
				if (p[j] == RC_MESH_NULL_IDX) break;
				if ((p[nvp + j] & 0x8000) == 0) continue;
				const int nj = (j + 1 >= nvp || p[j + 1] == RC_MESH_NULL_IDX) ? 0 : j + 1;
				const int vi[2] = { p[j], p[nj] };

				FVector3 color = colorBoundary;
				if ((p[nvp + j] & 0xf) != 0xf)
					color = FVector3(1, 1, 1);

				FVector3 vtx[2];
				int vtxIndex = 0;

				for (int k = 0; k < 2; ++k)
				{
					const unsigned short* v = &m_pmesh->verts[vi[k] * 3];
					const float x = orig[0] + v[0] * cs;
					const float y = orig[1] + (v[1] + 1)*ch + 0.1f;
					const float z = orig[2] + v[2] * cs;
					vtx[vtxIndex].x = x;
					vtx[vtxIndex].y = y;
					vtx[vtxIndex].z = z;
					vtxIndex++;
				}

				debugDrawer->drawLine(vtx[0], vtx[1], color);
			}
		}
	}

	if (myDebugFlags & (1 << DebugDrawFlags::DRAW_VERTICES))
	{
		FVector3 colorVtx(0.5f, 0.1f, 0.1f);
		for (int i = 0; i < m_pmesh->nverts; ++i)
		{
			const unsigned short* v = &m_pmesh->verts[i * 3];
			const float x = orig[0] + v[0] * cs;
			const float y = orig[1] + (v[1] + 1)*ch + 0.1f;
			const float z = orig[2] + v[2] * cs;
			FVector3 point(x, y, z);
			debugDrawer->DrawPoint(point, 0.3f, colorVtx);
		}
	}

	if (myDebugFlags & (1 << DebugDrawFlags::DRAW_PATHS))
	{
		// test getting a path
		/*
		dtPolyRef startRef, endRef;
		float spos[3] = { 5, 0, 15 };
		float epos[3] = { 50, 0, 50 };
		float nspos[3];
		float nepos[3];
		const float polyPickExt[3] = { 2,4,2 };
		dtQueryFilter filter;

		static const int MAX_POLYS = 256;
		dtPolyRef polys[MAX_POLYS];
		int npolys;
		m_navQuery->findNearestPoly(spos, polyPickExt, &filter, &startRef, nspos);
		m_navQuery->findNearestPoly(epos, polyPickExt, &filter, &endRef, nepos);
		m_navQuery->findPath(startRef, endRef, spos, epos, &filter, polys, &npolys, MAX_POLYS);

		std::vector<FVector3> path;
		for (int i = 0; i < npolys; i++)
		{
			dtStatus status = 0;
			float reqPos[3];
			status = m_navQuery->closestPointOnPoly(polys[i], epos, reqPos, 0);
			path.push_back(FVector3(reqPos[0], reqPos[1], reqPos[2]));
		}

		FVector3 startPos(spos[0], spos[1], spos[2]);
		FVector3 endPos(epos[0], epos[1], epos[2]);
		debugDrawer->DrawPoint(startPos + FVector3(0, 0.4f, 0), 0.2f, FVector3(0, 1, 0));
		debugDrawer->DrawPoint(endPos + FVector3(0, 0.4f, 0), 0.2f, FVector3(1, 0, 0));

		for (int i = 0; i < path.size(); i++)
		{
			debugDrawer->DrawPoint(path[i] + FVector3(0, 0.4f, 0), 0.2f, FVector3(1, 1, 1));
			if (i > 0)
				debugDrawer->drawLine(path[i] + FVector3(0, 0.4f, 0), path[i - 1] + FVector3(0, 0.4f, 0), FVector3(1, 1, 1));
		}
		*/
	}

}

bool FNavMeshManagerRecast::GenerateNavMesh()
{
	float m_cellSize = 1.3f;
	float m_cellHeight = 0.5f;
	float m_agentHeight = 2.0f;
	float m_agentRadius = 1.0f;
	float m_agentMaxClimb = 1.0f;
	float m_agentMaxSlope = 45.0f;
	float m_edgeMaxLen = 200.0f;
	float m_regionMinSize = 1.0f;
	float m_regionMergeSize = 0.1f;
	int m_edgeMaxError = 0;
	float m_vertsPerPoly = 6.0f;
	float m_detailSampleDist = 3.0f;
	float m_detailSampleMaxError = 1.0f;

	// Init build configuration
	memset(&m_cfg, 0, sizeof(m_cfg));
	m_cfg.cs = m_cellSize;
	m_cfg.ch = m_cellHeight;
	m_cfg.walkableSlopeAngle = m_agentMaxSlope;
	m_cfg.walkableHeight = (int)ceilf(m_agentHeight / m_cfg.ch);
	m_cfg.walkableClimb = (int)floorf(m_agentMaxClimb / m_cfg.ch);
	m_cfg.walkableRadius = (int)ceilf(m_agentRadius / m_cfg.cs);
	m_cfg.maxEdgeLen = (int)(m_edgeMaxLen / m_cellSize);
	m_cfg.maxSimplificationError = m_edgeMaxError;
	m_cfg.minRegionArea = (int)rcSqr(m_regionMinSize);		// Note: area = size*size
	m_cfg.mergeRegionArea = (int)rcSqr(m_regionMergeSize);	// Note: area = size*size
	m_cfg.maxVertsPerPoly = (int)m_vertsPerPoly;
	m_cfg.detailSampleDist = m_detailSampleDist < 0.9f ? 0 : m_cellSize * m_detailSampleDist;
	m_cfg.detailSampleMaxError = m_cellHeight * m_detailSampleMaxError;

	float bmin[] = { -400, -10, -400 };
	float bmax[] = { 400, 800, 400 };
	rcVcopy(m_cfg.bmin, bmin);
	rcVcopy(m_cfg.bmax, bmax);
	rcCalcGridSize(m_cfg.bmin, m_cfg.bmax, m_cfg.cs, &m_cfg.width, &m_cfg.height);

	// allocate poly soup
	m_solid = rcAllocHeightfield();

	m_ctx = new FContext();
	m_ctx->resetTimers();

	if (!m_solid)
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'solid'.");
		return false;
	}
	if (!rcCreateHeightfield(m_ctx, *m_solid, m_cfg.width, m_cfg.height, m_cfg.bmin, m_cfg.bmax, m_cfg.cs, m_cfg.ch))
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not create solid heightfield.");
		return false;
	}

	// Allocate array that can hold triangle area types.
	// If you have multiple meshes you need to process, allocate
	// and array which can hold the max number of triangles you need to process.
	int nverts = myInputMesh.myVertices.size();
	int ntris = myInputMesh.myTriangles.size() / 3;

	m_triareas = new unsigned char[ntris];
	if (!m_triareas)
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'm_triareas' (%d).", ntris);
		return false;
	}

	// Find triangles which are walkable based on their slope and rasterize them.
	// If your input data is multiple meshes, you can transform them here, calculate
	// the are type for each of the meshes and rasterize them.
	memset(m_triareas, 0, ntris * sizeof(unsigned char));
	rcMarkWalkableTriangles(m_ctx, m_cfg.walkableSlopeAngle, &myInputMesh.myVertices[0], nverts, &myInputMesh.myTriangles[0], ntris, m_triareas);
	if (!rcRasterizeTriangles(m_ctx, &myInputMesh.myVertices[0], nverts, &myInputMesh.myTriangles[0], m_triareas, ntris, *m_solid, m_cfg.walkableClimb))
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not rasterize triangles.");
		return false;
	}

	if (!m_keepInterResults)
	{
		delete[] m_triareas;
		m_triareas = 0;
	}

	// Once all geoemtry is rasterized, we do initial pass of filtering to
	// remove unwanted overhangs caused by the conservative rasterization
	// as well as filter spans where the character cannot possibly stand.
	if (m_filterLowHangingObstacles)
		rcFilterLowHangingWalkableObstacles(m_ctx, m_cfg.walkableClimb, *m_solid);
	if (m_filterLedgeSpans)
		rcFilterLedgeSpans(m_ctx, m_cfg.walkableHeight, m_cfg.walkableClimb, *m_solid);
	if (m_filterWalkableLowHeightSpans)
		rcFilterWalkableLowHeightSpans(m_ctx, m_cfg.walkableHeight, *m_solid);

	// Compact the heightfield so that it is faster to handle from now on.
	// This will result more cache coherent data as well as the neighbours
	// between walkable cells will be calculated.
	m_chf = rcAllocCompactHeightfield();
	if (!m_chf)
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'chf'.");
		return false;
	}
	if (!rcBuildCompactHeightfield(m_ctx, m_cfg.walkableHeight, m_cfg.walkableClimb, *m_solid, *m_chf))
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build compact data.");
		return false;
	}

	if (!m_keepInterResults)
	{
		rcFreeHeightField(m_solid);
		m_solid = 0;
	}

	// Erode the walkable area by agent radius.
	if (!rcErodeWalkableArea(m_ctx, m_cfg.walkableRadius, *m_chf))
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not erode.");
		return false;
	}

	// these areas are not automatically eroded with charRadius
	for (int i = 0; i < myAABBList.size(); ++i)
	{
		FVector3 points[4];
		points[0] = myAABBList[i].myMin;
		points[1] = FVector3(myAABBList[i].myMin.x, myAABBList[i].myMin.y, myAABBList[i].myMax.z);
		points[2] = myAABBList[i].myMax;
		points[3] = FVector3(myAABBList[i].myMax.x, myAABBList[i].myMin.y, myAABBList[i].myMin.z);

		float hmin = 0.0f;
		float hmax = 100.0f;

		float verts[4 * 3];
		int vertCount = 0;
		for (int  i = 0; i < 4; i++)
		{
			verts[vertCount++] = points[i].x;
			verts[vertCount++] = points[i].y;
			verts[vertCount++] = points[i].z;
		}

		rcMarkConvexPolyArea(m_ctx, verts, 4, hmin, hmax, (unsigned char)1, *m_chf); // area type is game defined - todo: wrap in enum flags and area costs
	}

	// Partition, check sample for different partition options (watershed is the best but slowest, monotone is the fastest
	{
		// Partition the walkable surface into simple regions without holes.
		// Monotone partitioning does not need distancefield.
		if (!rcBuildRegionsMonotone(m_ctx, *m_chf, 0, m_cfg.minRegionArea, m_cfg.mergeRegionArea))
		{
			m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build monotone regions.");
			return false;
		}
	}
	
	// Create contours.
	m_cset = rcAllocContourSet();
	if (!m_cset)
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'cset'.");
		return false;
	}
	if (!rcBuildContours(m_ctx, *m_chf, m_cfg.maxSimplificationError, m_cfg.maxEdgeLen, *m_cset))
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not create contours.");
		return false;
	}

	// Build polygon navmesh from the contours.
	m_pmesh = rcAllocPolyMesh();
	if (!m_pmesh)
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmesh'.");
		return false;
	}
	if (!rcBuildPolyMesh(m_ctx, *m_cset, m_cfg.maxVertsPerPoly, *m_pmesh))
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not triangulate contours.");
		return false;
	}

	m_dmesh = rcAllocPolyMeshDetail();
	if (!m_dmesh)
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmdtl'.");
		return false;
	}

	if (!rcBuildPolyMeshDetail(m_ctx, *m_pmesh, *m_chf, m_cfg.detailSampleDist, m_cfg.detailSampleMaxError, *m_dmesh))
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build detail mesh.");
		return false;
	}

	if (!m_keepInterResults)
	{
		rcFreeCompactHeightfield(m_chf);
		m_chf = 0;
		rcFreeContourSet(m_cset);
		m_cset = 0;
	}

	// Detour
	if (m_cfg.maxVertsPerPoly <= DT_VERTS_PER_POLYGON)
	{
		unsigned char* navData = 0;
		int navDataSize = 0;

		// Update poly flags from areas. (area type and flags)
		for (int i = 0; i < m_pmesh->npolys; ++i)
		{
			if (m_pmesh->areas[i] == RC_WALKABLE_AREA)
				m_pmesh->areas[i] = 0;

			if (m_pmesh->areas[i] == 0)
			{
				m_pmesh->flags[i] = 1;
			}
		}


		dtNavMeshCreateParams params;
		memset(&params, 0, sizeof(params));
		params.verts = m_pmesh->verts;
		params.vertCount = m_pmesh->nverts;
		params.polys = m_pmesh->polys;
		params.polyAreas = m_pmesh->areas;
		params.polyFlags = m_pmesh->flags;
		params.polyCount = m_pmesh->npolys;
		params.nvp = m_pmesh->nvp;
		params.detailMeshes = m_dmesh->meshes;
		params.detailVerts = m_dmesh->verts;
		params.detailVertsCount = m_dmesh->nverts;
		params.detailTris = m_dmesh->tris;
		params.detailTriCount = m_dmesh->ntris;
	/*	params.offMeshConVerts = m_geom->getOffMeshConnectionVerts();
		params.offMeshConRad = m_geom->getOffMeshConnectionRads();
		params.offMeshConDir = m_geom->getOffMeshConnectionDirs();
		params.offMeshConAreas = m_geom->getOffMeshConnectionAreas();
		params.offMeshConFlags = m_geom->getOffMeshConnectionFlags();
		params.offMeshConUserID = m_geom->getOffMeshConnectionId();
		params.offMeshConCount = m_geom->getOffMeshConnectionCount();*/
		params.walkableHeight = m_agentHeight;
		params.walkableRadius = m_agentRadius;
		params.walkableClimb = m_agentMaxClimb;
		rcVcopy(params.bmin, m_pmesh->bmin);
		rcVcopy(params.bmax, m_pmesh->bmax);
		params.cs = m_cfg.cs;
		params.ch = m_cfg.ch;
		params.buildBvTree = true;

		if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
		{
			m_ctx->log(RC_LOG_ERROR, "Could not build Detour navmesh.");
			return false;
		}

		m_navMesh = dtAllocNavMesh();
		if (!m_navMesh)
		{
			dtFree(navData);
			m_ctx->log(RC_LOG_ERROR, "Could not create Detour navmesh");
			return false;
		}

		dtStatus status;

		status = m_navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
		if (dtStatusFailed(status))
		{
			dtFree(navData);
			m_ctx->log(RC_LOG_ERROR, "Could not init Detour navmesh");
			return false;
		}

		m_navQuery = dtAllocNavMeshQuery();
		status = m_navQuery->init(m_navMesh, 2048);
		if (dtStatusFailed(status))
		{
			m_ctx->log(RC_LOG_ERROR, "Could not init Detour navmesh query");
			return false;
		}
	}

	m_ctx->stopTimer(RC_TIMER_TOTAL);

	// Show performance stats.
	//duLogBuildTimes(*m_ctx, m_ctx->getAccumulatedTime(RC_TIMER_TOTAL));
	m_ctx->log(RC_LOG_PROGRESS, ">> Polymesh: %d vertices  %d polygons", m_pmesh->nverts, m_pmesh->npolys);

	float m_totalBuildTimeMs = m_ctx->getAccumulatedTime(RC_TIMER_TOTAL) / 1000.0f;
	
	return true;
}
namespace
{

	struct Triangle
	{
		FVector3 V0;
		FVector3 V1;
		FVector3 V2;
	};

	struct Ray
	{
		FVector3 P0;
		FVector3 P1;
	};

	float dot(FVector3 aFirst, FVector3 aSecond)
	{
		return aFirst.Dot(aSecond);
	}

	FVector3 cross(FVector3 aFirst, FVector3 aSecond)
	{
		return aFirst.Cross(aSecond);
	}
}

int intersect3D_RayTriangle(Ray R, Triangle T, FVector3& I)
{

	FVector3    u, v, n;              // triangle vectors
	FVector3    dir, w0, w;           // ray vectors
	float     r, a, b;              // params to calc ray-plane intersect

									// get triangle edge vectors and plane normal
	u = T.V1 - T.V0;
	v = T.V2 - T.V0;
	n = cross(u, v);              // cross product
	if (n.Length() == 0)             // triangle is degenerate
		return -1;                  // do not deal with this case

	dir = R.P1 - R.P0;              // ray direction vector
	w0 = R.P0 - T.V0;
	a = -dot(n, w0);
	b = dot(n, dir);
	if (fabs(b) < 0.0000001f) {     // ray is  parallel to triangle plane
		if (a == 0)                 // ray lies in triangle plane
			return 2;
		else return 0;              // ray disjoint from plane
	}

	// get intersect point of ray with triangle plane
	r = a / b;
	if (r < 0.0)                    // ray goes away from triangle
		return 0;                   // => no intersect
									// for a segment, also test if (r > 1.0) => no intersect

	I = R.P0 + r * dir;            // intersect point of ray and plane

									// is I inside T?
	float    uu, uv, vv, wu, wv, D;
	uu = dot(u, u);
	uv = dot(u, v);
	vv = dot(v, v);
	w = I - T.V0;
	wu = dot(w, u);
	wv = dot(w, v);
	D = uv * uv - uu * vv;

	// get and test parametric coords
	float s, t;
	s = (uv * wv - vv * wu) / D;
	if (s < 0.0 || s > 1.0)         // I is outside T
		return 0;
	t = (uv * wu - uu * wv) / D;
	if (t < 0.0 || (s + t) > 1.0)  // I is outside T
		return 0;

	return 1;                       // I is in T
}

FVector3 FNavMeshManagerRecast::RayCast(FVector3 aStart, FVector3 anEnd)
{
	// raycast against input mesh(es)
	int closestTri;
	float closestDist = -1.0f;
	FVector3 closestPos;
	Triangle t;
	int nrOfTri = myInputMesh.myTriangles.size() / 3;
	for (int i = 0; i < nrOfTri; i++)
	{
		int idx = i * 3;
		t.V0.x = myInputMesh.myVertices[3 * myInputMesh.myTriangles[idx]];
		t.V0.y = myInputMesh.myVertices[3 * myInputMesh.myTriangles[idx] + 1];
		t.V0.z = myInputMesh.myVertices[3 * myInputMesh.myTriangles[idx] + 2];
										 
		t.V1.x = myInputMesh.myVertices[3 * myInputMesh.myTriangles[idx+1]];
		t.V1.y = myInputMesh.myVertices[3 * myInputMesh.myTriangles[idx+1] + 1];
		t.V1.z = myInputMesh.myVertices[3 * myInputMesh.myTriangles[idx+1] + 2];
										
		t.V2.x = myInputMesh.myVertices[3 * myInputMesh.myTriangles[idx+2]];
		t.V2.y = myInputMesh.myVertices[3 * myInputMesh.myTriangles[idx+2] + 1];
		t.V2.z = myInputMesh.myVertices[3 * myInputMesh.myTriangles[idx+2] + 2];

		Ray ray;
		ray.P0 = aStart;
		ray.P1 = aStart + (anEnd-aStart).Normalized();
		FVector3 intersectPos;

		if (intersect3D_RayTriangle(ray, t, intersectPos) == 1)
		{
			float newDist = (ray.P0 - intersectPos).Length();
			if (newDist < closestDist || closestDist == -1.0f)
			{
				closestDist = newDist;
				closestPos = intersectPos;
				closestTri = i;
			}
		}
	}

	FD3d12Renderer::GetInstance()->GetDebugDrawer()->DrawTriangle(t.V0, t.V1, t.V2, FVector3(0.8f, 0.3f, 0.3f));
	return closestPos;
}

FVector3 FNavMeshManagerRecast::GetClosestPointOnNavMesh(FVector3 aPoint)
{
	if (!m_navQuery)
		return FVector3();

	float extends[3] = { 10, 10, 10 }; // how far to search for
	dtQueryFilter m_filter;
	float nearestPt[3];
	dtPolyRef nearestPoly;
	dtStatus status = m_navQuery->findNearestPoly(&aPoint.x, extends, &m_filter, &nearestPoly, nearestPt);

	if (nearestPoly == 0)
		return FVector3();

	return FVector3(nearestPt[0], nearestPt[1], nearestPt[2]);
}

FVector3 FNavMeshManagerRecast::GetClosestPointOnNavMesh(FVector3 aPoint, FVector3 aMaxExtends)
{
	if (!m_navQuery)
		return FVector3();

	float extends[3] = { aMaxExtends.x, aMaxExtends.y, aMaxExtends.z }; // how far to search for
	dtQueryFilter m_filter;
	float nearestPt[3];
	dtPolyRef nearestPoly;
	dtStatus status = m_navQuery->findNearestPoly(&aPoint.x, extends, &m_filter, &nearestPoly, nearestPt);

	if (nearestPoly == 0)
		return aPoint;// FVector3();

	return FVector3(nearestPt[0], nearestPt[1], nearestPt[2]);
}

std::vector<FVector3> FNavMeshManagerRecast::FindPath(FVector3 aStart, FVector3 anEnd)
{
	dtPolyRef startRef, endRef;
	float spos[3] = { aStart.x, aStart.y, aStart.z };
	float epos[3] = { anEnd.x, anEnd.y, anEnd.z };
	float nspos[3];
	float nepos[3];
	const float polyPickExt[3] = { 2,4,2 };
	dtQueryFilter filter;

	static const int MAX_POLYS = 256;
	dtPolyRef polys[MAX_POLYS];
	dtPolyRef polysStraight[MAX_POLYS];
	float polysStraight2[MAX_POLYS];
	int npolys;
	int npolysStraight;
	m_navQuery->findNearestPoly(spos, polyPickExt, &filter, &startRef, nspos);
	m_navQuery->findNearestPoly(epos, polyPickExt, &filter, &endRef, nepos);
	m_navQuery->findPath(startRef, endRef, spos, epos, &filter, polys, &npolys, MAX_POLYS);
	m_navQuery->findStraightPath(spos, epos, polys, npolys, polysStraight2, nullptr, polysStraight, &npolysStraight, MAX_POLYS);
	
	std::vector<FVector3> path;
	path.push_back(aStart);

	for (int i = 0; i < npolys; i++)
	{
		dtStatus status = 0;
		float reqPos[3];
		status = m_navQuery->closestPointOnPoly(polys[i], epos, reqPos, 0);
		path.push_back(FVector3(reqPos[0], reqPos[1], reqPos[2]));
	}

	// use straight path for now (there is also follow and sliced which i think are better?)
	if (npolys)
	{
		// In case of partial path, make sure the end point is clamped to the last polygon.
		float epos[3];
		if (polys[npolys - 1] != endRef)
			m_navQuery->closestPointOnPoly(polys[npolys - 1], epos, epos, 0);

		int m_straightPathOptions = 0; //enum: dtStraightPathOptions
		float m_straightPath[MAX_POLYS * 3];
		int m_nstraightPath;
		unsigned char m_straightPathFlags[MAX_POLYS];
		dtPolyRef m_straightPathPolys[MAX_POLYS];
		m_navQuery->findStraightPath(spos, epos, polys, npolys,
			m_straightPath, m_straightPathFlags,
			m_straightPathPolys, &m_nstraightPath, MAX_POLYS, m_straightPathOptions);

		path.clear();
		path.push_back(aStart);
		for (int i = 0; i < m_nstraightPath; ++i)
		{
			path.push_back(FVector3(m_straightPath[i * 3], m_straightPath[i * 3 + 1], m_straightPath[i * 3 + 2]));
		}
	}
	
	return path;
}

void FNavMeshManagerRecast::AddBlockingAABB(FVector3 aMin, FVector3 aMax)
{
	FAABB aabb;
	FVector3 aDimensions = aMax - aMin;

	aMin = aMin - (aDimensions.Normalized() * 0.5f); // fake erosion with charRadius (currently hardcode 50cm)
	aMax = aMax + (aDimensions.Normalized() * 0.5f);

	aabb.myMin = aMin;
	aabb.myMax = aMax;
	myAABBList.push_back(aabb);
}

FNavMeshManagerRecast * FNavMeshManagerRecast::GetInstance()
{
	if (!ourInstance)
		ourInstance = new FNavMeshManagerRecast();

	return ourInstance;
}

void FContext::doLog(const rcLogCategory, const char * aText, const int aLen)
{
	FLOG("%s", aText);
}
