#pragma once

#include "FGameEntity.h"
#include "FPrimitiveGeometry.h"
#include <vector>

struct ID3D12Resource;
class btRigidBody;

class FGameTerrain : public FGameEntity
{
public:
	REGISTER_GAMEENTITY(FGameTerrain);

	FGameTerrain();
	virtual void Init(const FJsonObject& anObj) override;
	~FGameTerrain();
	void Update(double aDeltaTime) override;
	int GetTerrainSizeX() { return myTerrainSizeX; }
	int GetTerrainSizeZ() { return myTerrainSizeZ; }
	void ExtrudeFace(int anIdxTL, FVector3 anExtrudeVec, std::vector<FPrimitiveGeometry::Vertex>& aVertices, std::vector<int>& anIndices);
private:
	FRenderableObject* myGraphicsObject;
	btRigidBody* myPhysicsObject;
	ID3D12Resource* m_vertexBuffer;
	ID3D12Resource* m_indexBuffer;
	int myTerrainSizeX;
	int myTerrainSizeZ;
	float myTileSize;
	std::vector<FPrimitiveGeometry::Vertex> vertices;
	std::vector<int> indices;
};

