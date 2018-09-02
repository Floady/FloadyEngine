#pragma once

#include "FAsset.h"
#include <string>
#include <vector>
#include "FVector2.h"
#include "FVector3.h"

class F3DModel :
	public FAsset
{
public:

	struct FMaterial
	{
		std::string myDiffuseTexture;
		std::string mySpecularTexture;
		std::string myNormalTexture;
	};

	struct FVertex
	{
		FVertex()
		{
			position.x = 0; position.y = 0; position.z = 0;
			normal.x = 0; normal.y = 0; normal.z = -1; uv.x = 0; uv.y = 0;
			position.w = 1.0f;
			normal.w = 1.0f;
			myDiffuseMatId = 0;
			myNormalMatId = 0;
			mySpecularMatId = 0;
		}

		FVertex(const FVertex& anOther)
		{
			position.x = anOther.position.x; position.y = anOther.position.y; position.z = anOther.position.z;
			normal.x = anOther.normal.x; normal.y = anOther.normal.y; normal.z = -1; uv.x = anOther.uv.x; uv.y = anOther.uv.y;
			position.w = 1.0f;
			normal.w = 1.0f;
			myDiffuseMatId = 0;
			myNormalMatId = 0;
			mySpecularMatId = 0;
		}

		FVertex(float x, float y, float z, float nx, float ny, float nz, float u, float v)
		{
			position.x = x; position.y = y; position.z = z;
			normal.x = nx; normal.y = ny; normal.z = nz; uv.x = u; uv.y = v;
			position.w = 1.0f;
			normal.w = 1.0f;
			myDiffuseMatId = 0;
			myNormalMatId = 0;
			mySpecularMatId = 0;
		}

		bool operator==(const FVertex& other) const {
			return position.x == other.position.x
				&& position.y == other.position.y
				&& position.z == other.position.z
				&& normal.x == other.normal.x
				&& normal.y == other.normal.y
				&& normal.z == other.normal.z
				&& uv.x == other.uv.x
				&& uv.y == other.uv.y
				&& myDiffuseMatId == other.myDiffuseMatId
				&& myNormalMatId == other.myNormalMatId
				&& mySpecularMatId == other.mySpecularMatId;
		}

		FVector3 position;
		FVector3 normal;
		FVector2 uv;
		unsigned int myDiffuseMatId;
		unsigned int myNormalMatId;
		unsigned int mySpecularMatId;
	};

	FAssetType GetType() override { return FAssetType::Model; }

	bool Load(const char* aPath) override;

	F3DModel();
	~F3DModel();

	std::vector<FVertex> myVertices;
	std::vector<int> myIndices;
	std::vector<FMaterial> myMaterials;
};

