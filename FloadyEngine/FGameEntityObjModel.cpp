#include "FGameEntityObjModel.h"
#include "FRenderableObject.h"
#include "btBulletDynamicsCommon.h"
#include "LinearMath/btVector3.h"
#include "FGame.h"
#include "FNavMeshManagerRecast.h"
#include "FDebugDrawer.h"
#include "FCamera.h"
#include "FLightManager.h"
#include "FUtilities.h"
#include "FObjLoader.h"
#include <unordered_map>
#include "FPrimitiveBox.h"
#include "FTextureManager.h"
#include "FPrimitiveBoxMultiTex.h"
#include "FBulletPhysics.h"
#include "FRenderMeshComponent.h"
#include "BulletDynamics\Dynamics\btRigidBody.h"

using namespace DirectX;

REGISTER_GAMEENTITY2(FGameEntityObjModel);


FGameEntityObjModel::FGameEntityObjModel()
{
	myGraphicsObject = nullptr;
	m_vertexBuffer = nullptr;
	m_indexBuffer = nullptr;
}

FGameEntityObjModel::~FGameEntityObjModel()
{
}


void hash_combine(size_t &seed, size_t hash)
{
	hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
	seed ^= hash;
}

namespace std {
	template<> struct hash<FPrimitiveGeometry::Vertex2> {
		size_t operator()(FPrimitiveGeometry::Vertex2 const& vertex) const {

			size_t seed = 0;
			seed += vertex.matId;
			hash<float> hasher;
			hash_combine(seed, hasher(vertex.position.x));
			hash_combine(seed, hasher(vertex.position.y));
			hash_combine(seed, hasher(vertex.position.z));
			hash_combine(seed, hasher(vertex.normal.x));
			hash_combine(seed, hasher(vertex.normal.y));
			hash_combine(seed, hasher(vertex.normal.z));
			hash_combine(seed, hasher(vertex.uv.x));
			hash_combine(seed, hasher(vertex.uv.y));
			return seed;
		}
	};

}
void FGameEntityObjModel::Init(const FJsonObject & anObj)
{
	FGameEntity::Init(anObj);
		
	myGraphicsObject = GetComponentInSlot<FRenderMeshComponent>(0)->GetGraphicsObject();

	//*
	FGame::GetInstance()->GetRenderer()->GetSceneGraph().RemoveObject(myGraphicsObject);
	FVector3 pos = myGraphicsObject->GetPos();
	FVector3 scale = myGraphicsObject->GetScale();
	delete myGraphicsObject;
	myGraphicsObject = new FPrimitiveBoxMultiTex(FD3d12Renderer::GetInstance(), pos, scale, FPrimitiveBoxInstanced::PrimitiveType::Box, 1);
	FGame::GetInstance()->GetRenderer()->GetSceneGraph().AddObject(myGraphicsObject, false);
	FObjLoader::FObjMesh& m = dynamic_cast<FPrimitiveBoxMultiTex*>(myGraphicsObject)->myObjMesh;
	/*/
	FObjLoader::FObjMesh m;
	//*/

	FObjLoader objLoader;
	
	string model = anObj.GetItem("model").GetAs<string>();
	string path = "models/";
	path.append(model);
	objLoader.LoadObj(path.c_str(), m, "models/", true);
	
	std::unordered_map<FPrimitiveGeometry::Vertex2, uint32_t> uniqueVertices = {};
	std::vector<FPrimitiveGeometry::Vertex2> vertices;
	std::vector<int> indices;
	int count = m.myShapes.size() / 2;  // test to check if cost is bound by vertex data - yes it is :p
	int counter = 0;
	int flipflop = 256;
	for (const auto& shape : m.myShapes)
	{
		counter++;

		if (counter != flipflop)
		{
		//	continue;
		}

		int idx2 = 0;
		int matId = 999;

		if (shape.mesh.material_ids.size() > 0)
			matId = shape.mesh.material_ids[0];


		for (const auto& index : shape.mesh.indices)
		{
			if ((idx2 % 3) == 0)
			{
				matId = shape.mesh.material_ids[idx2/3];
			}
			idx2++;

			FPrimitiveGeometry::Vertex2 vertex;

			vertex.position.x = m.myAttributes.vertices[3 * index.vertex_index + 0];
			vertex.position.y = m.myAttributes.vertices[3 * index.vertex_index + 1];
			vertex.position.z = m.myAttributes.vertices[3 * index.vertex_index + 2];

			if (m.myAttributes.normals.size() > 0)
			{
				vertex.normal.x = m.myAttributes.normals[3 * index.normal_index + 0];
				vertex.normal.y = m.myAttributes.normals[3 * index.normal_index + 1];
				vertex.normal.z = m.myAttributes.normals[3 * index.normal_index + 2];
			}

			if (m.myAttributes.texcoords.size() > 0)
			{
				vertex.uv.x = m.myAttributes.texcoords[2 * index.texcoord_index + 0];
				vertex.uv.y = 1.0f - m.myAttributes.texcoords[2 * index.texcoord_index + 1];
			}

			vertex.matId = matId;
			vertex.normalmatId = m.myMaterials[matId].bump_texname.empty() ? 99 : matId;

			//*
			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
			
			//vertices.push_back(vertex);
			/*/
			indices.push_back(indices.size());
			//*/
		}
	}

	// set GPU data
	UINT8* pVertexDataBegin;
	UINT8* pIndexDataBegin;

	HRESULT hr = FD3d12Renderer::GetInstance()->GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(FPrimitiveGeometry::Vertex2) * vertices.size()),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vertexBuffer));

	if (FAILED(hr))
	{
		FLOG("CreateCommittedResource failed %l", hr);
	}

	// Map the buffer
	CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
	hr = m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));

	// Create the vertex buffer.
	{
		const UINT vertexBufferSize = sizeof(FPrimitiveGeometry::Vertex2) * vertices.size();
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
	myGraphicsObject->m_vertexBufferView.StrideInBytes = sizeof(FPrimitiveGeometry::Vertex2);
	myGraphicsObject->m_vertexBufferView.SizeInBytes = sizeof(FPrimitiveGeometry::Vertex2) * vertices.size();

	myGraphicsObject->m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	myGraphicsObject->m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;  // get from primitive manager
	myGraphicsObject->m_indexBufferView.SizeInBytes = sizeof(int) * nrOfIndices;


	// setup AABB
	for (FPrimitiveGeometry::Vertex2& vert : vertices)
	{
		myGraphicsObject->myAABB.myMax.x = max(myGraphicsObject->myAABB.myMax.x, vert.position.x * myGraphicsObject->GetScale().x);
		myGraphicsObject->myAABB.myMax.y = max(myGraphicsObject->myAABB.myMax.y, vert.position.y * myGraphicsObject->GetScale().y);
		myGraphicsObject->myAABB.myMax.z = max(myGraphicsObject->myAABB.myMax.z, vert.position.z * myGraphicsObject->GetScale().z);

		myGraphicsObject->myAABB.myMin.x = min(myGraphicsObject->myAABB.myMin.x, vert.position.x * myGraphicsObject->GetScale().x);
		myGraphicsObject->myAABB.myMin.y = min(myGraphicsObject->myAABB.myMin.y, vert.position.y * myGraphicsObject->GetScale().y);
		myGraphicsObject->myAABB.myMin.z = min(myGraphicsObject->myAABB.myMin.z, vert.position.z * myGraphicsObject->GetScale().z);
	}


	// send mesh to Recast	
	{
		const FVector3& scale = myGraphicsObject->GetScale();
		FInputMesh mesh;
		for (auto& item : vertices)
		{
			const FPrimitiveGeometry::Vertex2& vtx = item;
			mesh.myVertices.push_back(vtx.position.x * scale.x);
			mesh.myVertices.push_back(vtx.position.y * scale.y);
			mesh.myVertices.push_back(vtx.position.z * scale.z);
		}

		for (int idx : indices)
		{
			mesh.myTriangles.push_back(idx);
		}

		FNavMeshManagerRecast::GetInstance()->SetInputMesh(mesh);
		FNavMeshManagerRecast::GetInstance()->GenerateNavMesh();
	}

	string tex = anObj.GetItem("tex").GetAs<string>();
	myGraphicsObject->SetTexture(tex.c_str());

	FLOG("Obj model #vtx: %i", vertices.size());
	FLOG("Obj model #idx: %i", indices.size());
	FLOG("Obj model #unique idx: %i", uniqueVertices.size());
}

void FGameEntityObjModel::Update(double aDeltaTime)
{
	FGameEntity::Update(aDeltaTime);
}