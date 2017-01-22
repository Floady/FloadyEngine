#include "FPrimitiveGeometry.h"
#include "d3dx12.h"


int FPrimitiveGeometry::Box::indices[] = {
	0,1,2,0,2,3,
	4,5,6,4,6,7,
	8,9,10,8,10,11,
	12,13,14,12,14,15,
	16,17,18,16,18,19,
	20,21,22,20,22,23
};

FPrimitiveGeometry::Vertex FPrimitiveGeometry::Box::triangleVertices[] =
{
	{ { -1.0f, -1.0f, -1.0f, 1.0f },{ 0, 0, -1, 0 },{ 0.0f, 1.0f, 0.0f, 0.0f } },
	{ { -1.0f, +1.0f, -1.0f, 1.0f },{ 0, 0, -1, 0 },{ 0.0f, 0.0f, 0.0f, 0.0f } },
	{ { +1.0f, +1.0f, -1.0f, 1.0f },{ 0, 0, -1, 0 },{ 1.0f, 0.0f, 0.0f, 0.0f } },
	{ { +1.0f, -1.0f, -1.0f, 1.0f },{ 0, 0, -1, 0 },{ 1.0f, 1.0f, 0.0f, 0.0f } },

	{ { -1.0f, -1.0f, +1.0f, 1.0f },{ 0, 0, 1, 0 },{ 1.0f, 1.0f, 0.0f, 0.0f } },
	{ { +1.0f, -1.0f, +1.0f, 1.0f },{ 0, 0, 1, 0 },{ 0.0f, 1.0f, 0.0f, 0.0f } },
	{ { +1.0f, +1.0f, +1.0f, 1.0f },{ 0, 0, 1, 0 },{ 0.0f, 0.0f, 0.0f, 0.0f } },
	{ { -1.0f, +1.0f, +1.0f, 1.0f },{ 0, 0, 1, 0 },{ 1.0f, 0.0f, 0.0f, 0.0f } },

	{ { -1.0f, +1.0f, -1.0f, 1.0f },{ 0, 1, 0, 0 },{ 0.0f, 1.0f, 0.0f, 0.0f } },
	{ { -1.0f, +1.0f, +1.0f, 1.0f },{ 0, 1, 0, 0 },{ 0.0f, 0.0f, 0.0f, 0.0f } },
	{ { +1.0f, +1.0f, +1.0f, 1.0f },{ 0, 1, 0, 0 },{ 1.0f, 0.0f, 0.0f, 0.0f } },
	{ { +1.0f, +1.0f, -1.0f, 1.0f },{ 0, 1, 0, 0 },{ 1.0f, 1.0f, 0.0f, 0.0f } },

	{ { -1.0f, -1.0f, -1.0f, 1.0f },{ 0, -1, 0,  0 },{ 1.0f, 1.0f, 0.0f, 0.0f } },
	{ { +1.0f, -1.0f, -1.0f, 1.0f },{ 0, -1, 0,  0 },{ 0.0f, 1.0f, 0.0f, 0.0f } },
	{ { +1.0f, -1.0f, +1.0f, 1.0f },{ 0, -1, 0,  0 },{ 0.0f, 0.0f, 0.0f, 0.0f } },
	{ { -1.0f, -1.0f, +1.0f, 1.0f },{ 0, -1, 0,  0 },{ 1.0f, 0.0f, 0.0f, 0.0f } },

	{ { -1.0f, -1.0f, +1.0f, 1.0f },{ -1, 0, 0, 0 },{ 0.0f, 1.0f, 0.0f, 0.0f } },
	{ { -1.0f, +1.0f, +1.0f, 1.0f },{ -1, 0, 0, 0 },{ 0.0f, 0.0f, 0.0f, 0.0f } },
	{ { -1.0f, +1.0f, -1.0f, 1.0f },{ -1, 0, 0, 0 },{ 1.0f, 0.0f, 0.0f, 0.0f } },
	{ { -1.0f, -1.0f, -1.0f, 1.0f },{ -1, 0, 0, 0 },{ 1.0f, 1.0f, 0.0f, 0.0f } },

	{ { +1.0f, -1.0f, -1.0f, 1.0f },{ 1, 0, 0,0 },{ 0.0f, 1.0f, 0.0f, 0.0f } },
	{ { +1.0f, +1.0f, -1.0f, 1.0f },{ 1, 0, 0,0 },{ 0.0f, 0.0f, 0.0f, 0.0f } },
	{ { +1.0f, +1.0f, +1.0f, 1.0f },{ 1, 0, 0,0 },{ 1.0f, 0.0f, 0.0f, 0.0f } },
	{ { +1.0f, -1.0f, +1.0f, 1.0f },{ 1, 0, 0,0 },{ 1.0f, 1.0f, 0.0f, 0.0f } }
};

ID3D12Resource* FPrimitiveGeometry::Box::m_vertexBuffer = nullptr;
ID3D12Resource* FPrimitiveGeometry::Box::m_indexBuffer = nullptr;

const int FPrimitiveGeometry::Box::GetIndicesBufferSize()
{
	return  sizeof(int) * _countof(indices);
}

const int FPrimitiveGeometry::Box::GetVertexBufferSize()
{
	return  sizeof(Vertex) * _countof(triangleVertices);
}

void FPrimitiveGeometry::InitD3DResources(ID3D12Device* aDevice, ID3D12GraphicsCommandList* aCommandList)
{
	UINT8* pVertexDataBegin;
	UINT8* pIndexDataBegin;

	HRESULT hr = aDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(Vertex) * 128 * 6),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&FPrimitiveGeometry::Box::m_vertexBuffer));

	// Map the buffer
	CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
	hr = FPrimitiveGeometry::Box::m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));

	// Create the vertex buffer.
	{
		const UINT vertexBufferSize = FPrimitiveGeometry::Box::GetVertexBufferSize();
		memcpy(pVertexDataBegin, FPrimitiveGeometry::Box::GetVertices(), vertexBufferSize);

		// index buffer
		hr = aDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(sizeof(int) * 128 * 6),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&FPrimitiveGeometry::Box::m_indexBuffer));

		// Map the buffer
		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		hr = FPrimitiveGeometry::Box::m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
		const UINT indexBufferSize = FPrimitiveGeometry::Box::GetIndicesBufferSize();
		memcpy(pIndexDataBegin, FPrimitiveGeometry::Box::GetIndices(), indexBufferSize);
	}
}
