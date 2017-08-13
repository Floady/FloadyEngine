#pragma once
#include "FGameEntity.h"
#include "FPrimitiveGeometry.h"
#include <vector>

struct ID3D12Resource;

class FGameEntityObjModel : public FGameEntity
{
public:
	REGISTER_GAMEENTITY(FGameEntityObjModel);

	FGameEntityObjModel();
	~FGameEntityObjModel();

	virtual void Init(const FJsonObject& anObj) override;
	void Update(double aDeltaTime) override;
private:
	ID3D12Resource* m_vertexBuffer;
	ID3D12Resource* m_indexBuffer;
	int myTerrainSizeX;
	int myTerrainSizeZ;
	float myTileSize;
	std::vector<FPrimitiveGeometry::Vertex> vertices;
	std::vector<int> indices;
};

