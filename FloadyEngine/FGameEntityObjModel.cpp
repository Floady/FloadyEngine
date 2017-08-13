#include "FGameEntityObjModel.h"
#include "FRenderableObject.h"
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
#include "FObjLoader.h"

using namespace DirectX;

REGISTER_GAMEENTITY2(FGameEntityObjModel);


FGameEntityObjModel::FGameEntityObjModel()
{
	m_vertexBuffer = nullptr;
	m_indexBuffer = nullptr;
}

FGameEntityObjModel::~FGameEntityObjModel()
{
}

void FGameEntityObjModel::Init(const FJsonObject & anObj)
{
	FGameEntity::Init(anObj);

	FObjLoader objLoader;
	FObjLoader::FObjMesh m;

	string model = anObj.GetItem("model").GetAs<string>();
	string path = "models/";
	path.append(model);
	objLoader.LoadObj(path.c_str(), m, "models/", true);

	std::vector<FPrimitiveGeometry::Vertex> vertices;
	std::vector<int> indices;
	for (const auto& shape : m.myShapes)
	{
	//	int count = shape.mesh.indices.size() / 2;  // test to check if cost is bound by vertex data - yes it is :p
	//	int counter = 0;
		for (const auto& index : shape.mesh.indices)
		{
	//		if (counter++ > count)
	//			break;

			FPrimitiveGeometry::Vertex vertex;

			vertex.position.x = m.myAttributes.vertices[3 * index.vertex_index + 0];
			vertex.position.y = m.myAttributes.vertices[3 * index.vertex_index + 1];
			vertex.position.z = m.myAttributes.vertices[3 * index.vertex_index + 2];

			if (m.myAttributes.normals.size() > 0)
			{
				vertex.normal.x = m.myAttributes.normals[3 * index.normal_index + 0];
				vertex.normal.y = m.myAttributes.normals[3 * index.normal_index + 1];
				vertex.normal.z = m.myAttributes.normals[3 * index.normal_index + 2];
			}

			if(m.myAttributes.texcoords.size() > 0)
			{
				vertex.uv.x = m.myAttributes.texcoords[2 * index.texcoord_index + 0];
				vertex.uv.y = m.myAttributes.texcoords[2 * index.texcoord_index + 1];
			}

			vertices.push_back(vertex);
			indices.push_back(indices.size());
		}
	}

	// set GPU data
	UINT8* pVertexDataBegin;
	UINT8* pIndexDataBegin;

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


	// setup AABB
	for (FPrimitiveGeometry::Vertex& vert : vertices)
	{
		myGraphicsObject->myAABB.myMax.x = max(myGraphicsObject->myAABB.myMax.x, vert.position.x * myGraphicsObject->GetScale().x);
		myGraphicsObject->myAABB.myMax.y = max(myGraphicsObject->myAABB.myMax.y, vert.position.y * myGraphicsObject->GetScale().y);
		myGraphicsObject->myAABB.myMax.z = max(myGraphicsObject->myAABB.myMax.z, vert.position.z * myGraphicsObject->GetScale().z);

		myGraphicsObject->myAABB.myMin.x = min(myGraphicsObject->myAABB.myMin.x, vert.position.x * myGraphicsObject->GetScale().x);
		myGraphicsObject->myAABB.myMin.y = min(myGraphicsObject->myAABB.myMin.y, vert.position.y * myGraphicsObject->GetScale().y);
		myGraphicsObject->myAABB.myMin.z = min(myGraphicsObject->myAABB.myMin.z, vert.position.z * myGraphicsObject->GetScale().z);
	}

	string tex = anObj.GetItem("tex").GetAs<string>();
	myGraphicsObject->SetTexture(tex.c_str());
}

void FGameEntityObjModel::Update(double aDeltaTime)
{
	FGameEntity::Update(aDeltaTime);
}