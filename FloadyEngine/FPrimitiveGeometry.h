#pragma once
#include <DirectXMath.h>
#include "FD3DClass.h"

using namespace DirectX;

class FPrimitiveGeometry
{
	struct Vertex
	{
		DirectX::XMFLOAT4 position;
		XMFLOAT4 normal;
		XMFLOAT4 uv;
	};

public:
	static void InitD3DResources(ID3D12Device* aDevice, ID3D12GraphicsCommandList* aCommandList);

	struct Box
	{
	private:
		static int indices[];
		static Vertex triangleVertices[];

	public:
		static ID3D12Resource* m_vertexBuffer;
		static ID3D12Resource* m_indexBuffer;
		static const Vertex* const GetVertices() {	return triangleVertices; }
		static const int* const GetIndices() {	return indices;	}
		static const int GetIndicesBufferSize();
		static const int GetVertexBufferSize();
	};

};