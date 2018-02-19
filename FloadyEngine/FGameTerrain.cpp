#include "FRenderableObject.h"
#include "FGameTerrain.h"
#include "FBulletPhysics.h"
#include "BulletDynamics\Dynamics\btRigidBody.h"
#include "btBulletDynamicsCommon.h"
#include "LinearMath/btVector3.h"
#include "FGame.h"
#include "FNavMeshManagerRecast.h"
#include "FDebugDrawer.h"
#include "FCamera.h"
#include "FLightManager.h"
#include "FUtilities.h"
#include "FRenderMeshComponent.h"
#include "FProfiler.h"

using namespace DirectX;

REGISTER_GAMEENTITY2(FGameTerrain);

FGameTerrain::FGameTerrain() : FGameEntity()
{
	m_vertexBuffer = nullptr;
	m_indexBuffer = nullptr;
	myPhysicsObject = nullptr;
}

void FGameTerrain::Init(const FJsonObject & anObj)
{
	FGameEntity::Init(anObj);

	FRenderMeshComponent* comp = static_cast<FRenderMeshComponent*>(CreateComponent("FRenderMeshComponent"));
	comp->Init(anObj);

	myTerrainSizeX = anObj.GetItem("terrainSizeX").GetAs<int>();
	myTerrainSizeZ = anObj.GetItem("terrainSizeZ").GetAs<int>();
	myTileSize = anObj.GetItem("tileSize").GetAs<double>();

	myGraphicsObject = comp->GetGraphicsObject();

	// generate a grid for the terrain terrain
	UINT8* pVertexDataBegin;
	UINT8* pIndexDataBegin;

	vertices.clear();
	const float stepX = 1.0f / myTerrainSizeX;
	const float stepZ = 1.0f / myTerrainSizeZ;
	const int uvScale = 1;
	float gradientY = 0.1f;
	vertices.reserve(myTerrainSizeX * myTerrainSizeZ);

	for (size_t i = 0; i < myTerrainSizeX; i++)
	{
		for (size_t j = 0; j < myTerrainSizeZ; j++)
		{
			vertices.push_back(FPrimitiveGeometry::Vertex(i * myTileSize, 0.0f, j * myTileSize, 0, 1, 0, i * stepX, j * stepZ));
		}
	}
	
	indices.clear();
	indices.reserve(myTerrainSizeX * myTerrainSizeZ * 6);
	for (int j = 0; j < myTerrainSizeX - 1; j++)
	{
			for (int i = 0; i < myTerrainSizeZ - 1; i++)
		{
			indices.push_back(j + (i*myTerrainSizeZ));
			indices.push_back(j + 1 + (i*myTerrainSizeZ));
			indices.push_back(j + ((i+1)*(myTerrainSizeZ)) + 1);

			indices.push_back(j + (i*myTerrainSizeZ));
			indices.push_back(j + (i*myTerrainSizeZ) + ((1)*myTerrainSizeZ) + 1);
			indices.push_back(j + (i*myTerrainSizeZ) + ((1)*myTerrainSizeZ));
		}
	}

	// test extruding some faces
	//*
	const int randSeed = 20;
	srand(randSeed);
	std::vector<int> extrudeFaces;
	std::vector<float> extrudeFaceHeights;
	const int baseHeight = 1;
	const int maxHeight = 20;
	const int percentageOfExtrudes = 10;
	for (int i = 1; i < myTerrainSizeX-1; i++)
		for (int j = 1; j < myTerrainSizeZ-1; j++)
		{
			int idx = i + j * myTerrainSizeX;
			if (rand() % 100 < percentageOfExtrudes)
			{
				extrudeFaces.push_back(idx);
				extrudeFaceHeights.push_back(baseHeight + (rand() % (maxHeight - baseHeight)));
			}
		}

	for (size_t i = 0; i < extrudeFaces.size(); i++)
	{
		ExtrudeFace(extrudeFaces[i], FVector3(0, extrudeFaceHeights[i], 0), vertices, indices);
	}
	//*/

	HRESULT hr = FD3d12Renderer::GetInstance()->GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(FPrimitiveGeometry::Vertex) * vertices.size()),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vertexBuffer));

	// Map the buffer
	CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
	hr = m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));

	// Create the vertex buffer.
	{
		const UINT vertexBufferSize = sizeof(FPrimitiveGeometry::Vertex) * vertices.size();
		memcpy(pVertexDataBegin, &vertices[0], vertexBufferSize);

		// index buffer
		hr = FD3d12Renderer::GetInstance()->GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(sizeof(int) * indices.size()),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_indexBuffer));

		// Map the buffer
		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		hr = m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
		const UINT indexBufferSize = sizeof(int) * indices.size();
		memcpy(pIndexDataBegin, &indices[0], indexBufferSize);
	}
	int nrOfIndices = indices.size();

	myGraphicsObject->myIndicesCount = nrOfIndices;
	myGraphicsObject->m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	myGraphicsObject->m_vertexBufferView.StrideInBytes = sizeof(FPrimitiveGeometry::Vertex);
	myGraphicsObject->m_vertexBufferView.SizeInBytes = sizeof(FPrimitiveGeometry::Vertex) * vertices.size();

	myGraphicsObject->m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	myGraphicsObject->m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;  // get from primitive manager
	myGraphicsObject->m_indexBufferView.SizeInBytes = sizeof(int) * nrOfIndices;
	
	myGraphicsObject->SetShader("terrainShader.hlsl");

	FLOG("Terrain made");

	btTriangleMesh* triangleMeshTerrain = new btTriangleMesh();
	for (int i = 0; i < indices.size(); i+=3)
	{
		btVector3 posA = btVector3(vertices[indices[i]].position.x, vertices[indices[i]].position.y, vertices[indices[i]].position.z);
		btVector3 posB = btVector3(vertices[indices[i+1]].position.x, vertices[indices[i+1]].position.y, vertices[indices[i+1]].position.z);
		btVector3 posC = btVector3(vertices[indices[i+2]].position.x, vertices[indices[i+2]].position.y, vertices[indices[i+2]].position.z);
		posA += btVector3(myPos.x, myPos.y, myPos.z);
		posB += btVector3(myPos.x, myPos.y, myPos.z);
		posC += btVector3(myPos.x, myPos.y, myPos.z);
		triangleMeshTerrain->addTriangle(posA, posB, posC);
	}

	// add terrain collision mesh
	btCollisionShape* myColShape = new btBvhTriangleMeshShape(triangleMeshTerrain, true);
	btDefaultMotionState* motionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)));
	btRigidBody::btRigidBodyConstructionInfo rigidBodyConstructionInfo(0.0f, motionState, myColShape, btVector3(0, 0, 0));
	myPhysicsObject = new btRigidBody(rigidBodyConstructionInfo);
	FGame::GetInstance()->GetPhysics()->AddTerrain(myPhysicsObject, myColShape, this);

	// send mesh to Recast	
	FNavMeshManagerRecast::FInputMesh mesh;
	for (FPrimitiveGeometry::Vertex& vtx : vertices)
	{
		mesh.myVertices.push_back(vtx.position.x);
		mesh.myVertices.push_back(vtx.position.y);
		mesh.myVertices.push_back(vtx.position.z);
	}

	for (int idx : indices)
	{
		mesh.myTriangles.push_back(idx);
	}

	FNavMeshManagerRecast::GetInstance()->SetInputMesh(mesh);
	FNavMeshManagerRecast::GetInstance()->GenerateNavMesh();

	myGraphicsObject->myAABB.Grow(FVector3(myTerrainSizeX * myTileSize, 40, myTerrainSizeZ * myTileSize));
	//myGraphicsObject->SetCastsShadows(false);
}


FGameTerrain::~FGameTerrain()
{
}

void FGameTerrain::Update(double aDeltaTime)
{
	//FPROFILE_FUNCTION("FGameTerrain Update");
	FGameEntity::Update(aDeltaTime);

	// update light AABB
	FCamera* cam = FD3d12Renderer::GetInstance()->GetCamera();
	FAABB& aabb = FLightManager::GetInstance()->GetVisibleAABB();

	for (int i = 0; i < vertices.size(); i++)
	{
		DirectX::XMFLOAT4& vtx = vertices[i].position;
		if (cam->IsInFrustum(vtx.x, vtx.y, vtx.z))
		{
			aabb.Grow(vtx.x, vtx.y, vtx.z);
		}
	}

	// bloat visible by tilesize
	aabb.Grow(FLightManager::GetInstance()->GetVisibleAABB().myMax + FVector3(myTileSize, myTileSize, myTileSize));
	aabb.Grow(FLightManager::GetInstance()->GetVisibleAABB().myMin - FVector3(myTileSize, myTileSize, myTileSize));
	
}

void FGameTerrain::ExtrudeFace(int anIdxTL, FVector3 anExtrudeVec, std::vector<FPrimitiveGeometry::Vertex>& aVertices, std::vector<int>& anIndices)
{
	// try extrude face
	// from top view the topleftcorner of the square we wanna pull out
	int idx = anIdxTL;
	int lastIdx = aVertices.size();

	FVector3 extrude = anExtrudeVec;// FVector3(0, 2, 0);
	FPrimitiveGeometry::Vertex tl = aVertices[idx];
	FPrimitiveGeometry::Vertex tr = aVertices[idx + 1];
	FPrimitiveGeometry::Vertex bl = aVertices[idx + myTerrainSizeX];
	FPrimitiveGeometry::Vertex br = aVertices[idx + myTerrainSizeX + 1];

	// add top aVertices
	tl.position.y += extrude.y;
	tr.position.y += extrude.y;
	bl.position.y += extrude.y;
	br.position.y += extrude.y;
	tl.uv.x = 0; tl.uv.y = 0;
	bl.uv.x = 0; bl.uv.y = 1;
	tr.uv.x = 1; tr.uv.y = 0;
	br.uv.x = 1; br.uv.y = 1;
	aVertices.push_back(tl);
	aVertices.push_back(tr);
	aVertices.push_back(br);
	aVertices.push_back(bl);
	anIndices.push_back(lastIdx);
	anIndices.push_back(lastIdx + 1);
	anIndices.push_back(lastIdx + 2);
	anIndices.push_back(lastIdx);
	anIndices.push_back(lastIdx + 2);
	anIndices.push_back(lastIdx + 3);
	lastIdx = aVertices.size();

	{
		// add left face (imagine from top view)
		FPrimitiveGeometry::Vertex newtl = aVertices[idx];
		FPrimitiveGeometry::Vertex newbl = aVertices[idx + myTerrainSizeX];
		FVector3 faceNormal = FVector3(0, 0, -1);

		tl.normal.x = faceNormal.x; tl.normal.y = faceNormal.y; tl.normal.z = faceNormal.z;
		bl.normal.x = faceNormal.x; bl.normal.y = faceNormal.y; bl.normal.z = faceNormal.z;
		newtl.normal.x = faceNormal.x; newtl.normal.y = faceNormal.y; newtl.normal.z = faceNormal.z;
		newbl.normal.x = faceNormal.x; newbl.normal.y = faceNormal.y; newbl.normal.z = faceNormal.z;
		tl.uv.x = 0; tl.uv.y = 0;
		bl.uv.x = 0; bl.uv.y = 1;
		newtl.uv.x = 1; newtl.uv.y = 0;
		newbl.uv.x = 1; newbl.uv.y = 1;
		aVertices.push_back(tl);
		aVertices.push_back(bl);
		aVertices.push_back(newbl);
		aVertices.push_back(newtl);
		anIndices.push_back(lastIdx);
		anIndices.push_back(lastIdx + 1);
		anIndices.push_back(lastIdx + 2);
		anIndices.push_back(lastIdx);
		anIndices.push_back(lastIdx + 2);
		anIndices.push_back(lastIdx + 3);
		lastIdx = aVertices.size();

		//*
		// add right face (imagine from top view)
		newtl = aVertices[idx];
		newbl = aVertices[idx + myTerrainSizeX];
		faceNormal = FVector3(0, 0, 1);
		tl.normal.x = faceNormal.x; tl.normal.y = faceNormal.y; tl.normal.z = faceNormal.z;
		bl.normal.x = faceNormal.x; bl.normal.y = faceNormal.y; bl.normal.z = faceNormal.z;
		newtl.normal.x = faceNormal.x; newtl.normal.y = faceNormal.y; newtl.normal.z = faceNormal.z;
		newbl.normal.x = faceNormal.x; newbl.normal.y = faceNormal.y; newbl.normal.z = faceNormal.z;
		tl.position.z += myTileSize;
		newtl.position.z += myTileSize;
		bl.position.z += myTileSize;
		newbl.position.z+= myTileSize;
		tl.uv.x = 0; tl.uv.y = 0;
		bl.uv.x = 0; bl.uv.y = 1;
		newtl.uv.x = 1; newtl.uv.y = 0;
		newbl.uv.x = 1; newbl.uv.y = 1;
		aVertices.push_back(tl);
		aVertices.push_back(bl);
		aVertices.push_back(newbl);
		aVertices.push_back(newtl);
		anIndices.push_back(lastIdx);
		anIndices.push_back(lastIdx + 2);
		anIndices.push_back(lastIdx + 1);
		anIndices.push_back(lastIdx);
		anIndices.push_back(lastIdx + 3);
		anIndices.push_back(lastIdx + 2);
		lastIdx = aVertices.size();
		//*/
	}

	{
		tl = aVertices[idx + myTerrainSizeX];
		tr = aVertices[idx];
		bl = aVertices[idx + myTerrainSizeX + 1];
		br = aVertices[idx + 1];
		tl.position.y += extrude.y;
		tr.position.y += extrude.y;
		bl.position.y += extrude.y;
		br.position.y += extrude.y;

		// add top face (imagine from top view)
		FPrimitiveGeometry::Vertex newtl = aVertices[idx + myTerrainSizeX];
		FPrimitiveGeometry::Vertex newbl = aVertices[idx + myTerrainSizeX + 1];
		FVector3 faceNormal = FVector3(1, 0, 0);

		tl.normal.x = faceNormal.x; tl.normal.y = faceNormal.y; tl.normal.z = faceNormal.z;
		bl.normal.x = faceNormal.x; bl.normal.y = faceNormal.y; bl.normal.z = faceNormal.z;
		newtl.normal.x = faceNormal.x; newtl.normal.y = faceNormal.y; newtl.normal.z = faceNormal.z;
		newbl.normal.x = faceNormal.x; newbl.normal.y = faceNormal.y; newbl.normal.z = faceNormal.z;
		tl.uv.x = 0; tl.uv.y = 0;
		bl.uv.x = 0; bl.uv.y = 1;
		newtl.uv.x = 1; newtl.uv.y = 0;
		newbl.uv.x = 1; newbl.uv.y = 1;
		aVertices.push_back(tl);
		aVertices.push_back(bl);
		aVertices.push_back(newbl);
		aVertices.push_back(newtl);
		anIndices.push_back(lastIdx);
		anIndices.push_back(lastIdx + 1);
		anIndices.push_back(lastIdx + 2);
		anIndices.push_back(lastIdx);
		anIndices.push_back(lastIdx + 2);
		anIndices.push_back(lastIdx + 3);
		lastIdx = aVertices.size();

		//*
		// add bot face (imagine from top view)
		newtl = aVertices[idx + myTerrainSizeX];
		newbl = aVertices[idx + myTerrainSizeX + 1];
		faceNormal = FVector3(-1, 0, 0);
		tl.normal.x = faceNormal.x; tl.normal.y = faceNormal.y; tl.normal.z = faceNormal.z;
		bl.normal.x = faceNormal.x; bl.normal.y = faceNormal.y; bl.normal.z = faceNormal.z;
		newtl.normal.x = faceNormal.x; newtl.normal.y = faceNormal.y; newtl.normal.z = faceNormal.z;
		newbl.normal.x = faceNormal.x; newbl.normal.y = faceNormal.y; newbl.normal.z = faceNormal.z;
		tl.position.x -= myTileSize;
		newtl.position.x -= myTileSize;
		bl.position.x -= myTileSize;
		newbl.position.x -= myTileSize;
		tl.uv.x = 0; tl.uv.y = 0;
		bl.uv.x = 0; bl.uv.y = 1;
		newtl.uv.x = 1; newtl.uv.y = 0;
		newbl.uv.x = 1; newbl.uv.y = 1;
		aVertices.push_back(tl);
		aVertices.push_back(bl);
		aVertices.push_back(newbl);
		aVertices.push_back(newtl);
		anIndices.push_back(lastIdx);
		anIndices.push_back(lastIdx + 2);
		anIndices.push_back(lastIdx + 1);
		anIndices.push_back(lastIdx);
		anIndices.push_back(lastIdx + 3);
		anIndices.push_back(lastIdx + 2);
		//*/
	}
}
