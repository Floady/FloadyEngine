#include "FPrimitiveGeometry.h"
#include "d3dx12.h"
#include "FVector3.h"

ID3D12Resource* FPrimitiveGeometry::Box::m_vertexBuffer = nullptr;
ID3D12Resource* FPrimitiveGeometry::Box::m_indexBuffer = nullptr;
ID3D12Resource* FPrimitiveGeometry::Box2::m_vertexBuffer = nullptr;
ID3D12Resource* FPrimitiveGeometry::Box2::m_indexBuffer = nullptr;
std::vector<int> FPrimitiveGeometry::Box::indices = std::vector<int>();
std::vector<FPrimitiveGeometry::Vertex> FPrimitiveGeometry::Box::vertices = std::vector<FPrimitiveGeometry::Vertex>();
std::vector<int> FPrimitiveGeometry::Box2::indices = std::vector<int>();
std::vector<FPrimitiveGeometry::Vertex> FPrimitiveGeometry::Box2::vertices = std::vector<FPrimitiveGeometry::Vertex>();

const int FPrimitiveGeometry::Box::GetIndicesBufferSize()
{
	return  sizeof(int) * indices.size();
}

const int FPrimitiveGeometry::Box::GetVertexBufferSize()
{
	return  sizeof(Vertex) * vertices.size();
}

const int FPrimitiveGeometry::Box2::GetIndicesBufferSize()
{
	return  sizeof(int) * FPrimitiveGeometry::Box2::indices.size();
}

const int FPrimitiveGeometry::Box2::GetVertexBufferSize()
{
	return  sizeof(Vertex) * FPrimitiveGeometry::Box2::vertices.size();
}


void FPrimitiveGeometry::InitD3DResources(ID3D12Device* aDevice, ID3D12GraphicsCommandList* aCommandList)
{
	// Box
	{
		UINT8* pVertexDataBegin;
		UINT8* pIndexDataBegin;


			int indices[] = {
				0,1,2,0,2,3,
				4,5,6,4,6,7,
				8,9,10,8,10,11,
				12,13,14,12,14,15,
				16,17,18,16,18,19,
				20,21,22,20,22,23
			};

			FPrimitiveGeometry::Box::GetIndices().resize(_countof(indices));
			memcpy(&FPrimitiveGeometry::Box::GetIndices()[0], &indices, sizeof(int) * _countof(indices));

			std::vector<Vertex>& vertices = FPrimitiveGeometry::Box::GetVertices();
			vertices.push_back(Vertex(-0.5f, -0.5f, -0.5f, 0, 0, -1, 0.0f, 1.0f));
			vertices.push_back(Vertex(-0.5f, +0.5f, -0.5f, 0, 0, -1, 0.0f, 0.0f));
			vertices.push_back(Vertex(+0.5f, +0.5f, -0.5f, 0, 0, -1, 1.0f, 0.0f));
			vertices.push_back(Vertex(+0.5f, -0.5f, -0.5f, 0, 0, -1, 1.0f, 1.0f));

			vertices.push_back(Vertex(-0.5f, -0.5f, +0.5f, 0, 0, +1, 1.0f, 1.0f));
			vertices.push_back(Vertex(+0.5f, -0.5f, +0.5f, 0, 0, +1, 0.0f, 1.0f));
			vertices.push_back(Vertex(+0.5f, +0.5f, +0.5f, 0, 0, +1, 0.0f, 0.0f));
			vertices.push_back(Vertex(-0.5f, +0.5f, +0.5f, 0, 0, +1, 1.0f, 0.0f));

			vertices.push_back(Vertex(-0.5f, +0.5f, -0.5f, 0, +1, 0, 0.0f, 1.0f));
			vertices.push_back(Vertex(-0.5f, +0.5f, +0.5f, 0, +1, 0, 0.0f, 0.0f));
			vertices.push_back(Vertex(+0.5f, +0.5f, +0.5f, 0, +1, 0, 1.0f, 0.0f));
			vertices.push_back(Vertex(+0.5f, +0.5f, -0.5f, 0, +1, 0, 1.0f, 1.0f));

			vertices.push_back(Vertex(-0.5f, -0.5f, -0.5f, 0, -1, 0, 1.0f, 1.0));
			vertices.push_back(Vertex(+0.5f, -0.5f, -0.5f, 0, -1, 0, 0.0f, 1.0));
			vertices.push_back(Vertex(+0.5f, -0.5f, +0.5f, 0, -1, 0, 0.0f, 0.0));
			vertices.push_back(Vertex(-0.5f, -0.5f, +0.5f, 0, -1, 0, 1.0f, 0.0));

			vertices.push_back(Vertex(-0.5f, -0.5f, +0.5f, -1, 0, 0, 0.0f, 1.0f));
			vertices.push_back(Vertex(-0.5f, +0.5f, +0.5f, -1, 0, 0, 0.0f, 0.0f));
			vertices.push_back(Vertex(-0.5f, +0.5f, -0.5f, -1, 0, 0, 1.0f, 0.0f));
			vertices.push_back(Vertex(-0.5f, -0.5f, -0.5f, -1, 0, 0, 1.0f, 1.0f));

			vertices.push_back(Vertex(+0.5f, -0.5f, -0.5f, +1, 0, 0, 0.0f, 1.0f));
			vertices.push_back(Vertex(+0.5f, +0.5f, -0.5f, +1, 0, 0, 0.0f, 0.0f));
			vertices.push_back(Vertex(+0.5f, +0.5f, +0.5f, +1, 0, 0, 1.0f, 0.0f));
			vertices.push_back(Vertex(+0.5f, -0.5f, +0.5f, +1, 0, 0, 1.0f, 1.0f));

			HRESULT hr = aDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(sizeof(Vertex) * 24),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&FPrimitiveGeometry::Box::GetVerticesBuffer()));

			// Map the buffer
			CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
			hr = FPrimitiveGeometry::Box::GetVerticesBuffer()->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));

			// Create the vertex buffer.
			{
				const UINT vertexBufferSize = FPrimitiveGeometry::Box::GetVertexBufferSize();
				memcpy(pVertexDataBegin, &vertices[0], vertexBufferSize);

				// index buffer
				hr = aDevice->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
					D3D12_HEAP_FLAG_NONE,
					&CD3DX12_RESOURCE_DESC::Buffer(sizeof(int) * 36),
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(&FPrimitiveGeometry::Box::GetIndicesBuffer()));

				// Map the buffer
				CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
				hr = FPrimitiveGeometry::Box::GetIndicesBuffer()->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
				const UINT indexBufferSize = FPrimitiveGeometry::Box::GetIndicesBufferSize();
				memcpy(pIndexDataBegin, &indices, indexBufferSize);
			}
		}

		// Sphere
		{
			UINT8* pVertexDataBegin;
			UINT8* pIndexDataBegin;

			std::vector<Vertex>& ret = FPrimitiveGeometry::Box2::GetVertices();
			std::vector<int>& indices = FPrimitiveGeometry::Box2::GetIndices();
			{
				float radius = 0.5f;
				int sliceCount = 20;
				int stackCount = 20;

				ret.push_back(Vertex(0, radius, 0, 0, 1, 0, 0, 0));
				float phiStep = PI / stackCount;
				float thetaStep = 2.0f* PI / sliceCount;

				for (int i = 0; i <= stackCount; i++) {
					float phi = i*phiStep;
					for (int j = 0; j <= sliceCount; j++) {
						float theta = j*thetaStep;
						FVector3 p = FVector3(
							(radius*sin(phi)*cos(theta)),
							(radius*cos(phi)),
							(radius* sin(phi)*sin(theta))
						);

						FVector3 t = FVector3(-radius*sin(phi)*sin(theta), 0, radius*sin(phi)*cos(theta));
						t.Normalize();
						FVector3 n = p;
						n.Normalize();
						FVector3 uv = FVector3(theta / (PI * 2), phi / PI, 0.0f);
						ret.push_back(Vertex(p.x, p.y, p.z, n.x, n.y, n.z, uv.x, uv.y));
					}
				}

				ret.push_back(Vertex(0, -radius, 0, 0, -1, 0, 0, 1));

				for (int i = 1; i <= sliceCount; i++) {
					indices.push_back(0);
					indices.push_back(i + 1);
					indices.push_back(i);
				}
				int baseIndex = 1;
				int ringVertexCount = sliceCount + 1;
				for (int i = 0; i < stackCount; i++) {
					for (int j = 0; j < sliceCount; j++) {
						indices.push_back(baseIndex + i*ringVertexCount + j);
						indices.push_back(baseIndex + i*ringVertexCount + j + 1);
						indices.push_back(baseIndex + (i + 1)*ringVertexCount + j);

						indices.push_back(baseIndex + (i + 1)*ringVertexCount + j);
						indices.push_back(baseIndex + i*ringVertexCount + j + 1);
						indices.push_back(baseIndex + (i + 1)*ringVertexCount + j + 1);
					}
				}
				int southPoleIndex = ret.size() - 1;
				baseIndex = southPoleIndex - ringVertexCount;
				for (int i = 0; i < sliceCount; i++) {
					indices.push_back(southPoleIndex);
					indices.push_back(baseIndex + i);
					indices.push_back(baseIndex + i + 1);
				}
			}

			HRESULT hr = aDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(sizeof(Vertex) * ret.size()), //todo these sizes should match the buff size aligned on memory if needed but is from old font
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&FPrimitiveGeometry::Box2::GetVerticesBuffer()));

			// Map the buffer
			CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
			hr = FPrimitiveGeometry::Box2::GetVerticesBuffer()->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));

			// Create the vertex buffer.
			{
				const UINT vertexBufferSize = FPrimitiveGeometry::Box2::GetVertexBufferSize();
				memcpy(pVertexDataBegin, &ret[0], vertexBufferSize);

				// index buffer
				hr = aDevice->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
					D3D12_HEAP_FLAG_NONE,
					&CD3DX12_RESOURCE_DESC::Buffer(sizeof(int) * indices.size()),
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(&FPrimitiveGeometry::Box2::GetIndicesBuffer()));

				// Map the buffer
				CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
				hr = FPrimitiveGeometry::Box2::GetIndicesBuffer()->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
				const UINT indexBufferSize = FPrimitiveGeometry::Box2::GetIndicesBufferSize();
				memcpy(pIndexDataBegin, &indices[0], indexBufferSize); // this was crashing with 20x20 1.0f radius
			}
		}
}
