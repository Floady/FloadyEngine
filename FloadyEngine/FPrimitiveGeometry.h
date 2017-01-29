#pragma once
#include <DirectXMath.h>
#include "FD3d12Renderer.h"

using namespace DirectX;

class FPrimitiveGeometry
{
public:
	struct Vertex
	{
		Vertex()
		{
			position.x = 0; position.y = 0; position.z = 0;
			normal.x = 0; normal.y = 0; normal.z = -1; uv.x = 0; uv.y = 0;
			position.w = 1.0f;
			normal.w = 1.0f;
			uv.z = 0.0f;
			uv.w = 0.0f;
		}

		Vertex(float x, float y, float z, float nx, float ny, float nz, float u, float v)
		{
			position.x = x; position.y = y; position.z = z; 
			normal.x = nx; normal.y = ny; normal.z = nz; uv.x = u; uv.y = v;
			position.w = 1.0f;
			normal.w = 1.0f;
			uv.z = 0.0f;
			uv.w = 0.0f;
		}

		XMFLOAT4 position;
		XMFLOAT4 normal;
		XMFLOAT4 uv;
	};

public:
	static void InitD3DResources(ID3D12Device* aDevice, ID3D12GraphicsCommandList* aCommandList);

	struct Box
	{
	private:
		static std::vector<int> indices;
		static std::vector<Vertex> vertices;
		static ID3D12Resource* m_vertexBuffer;
		static ID3D12Resource* m_indexBuffer;

	public:
		static std::vector<Vertex>& const GetVertices() {	return vertices; }
		static ID3D12Resource*& const GetIndicesBuffer() {	return m_indexBuffer;	}
		static ID3D12Resource*& const GetVerticesBuffer() { return m_vertexBuffer; }
		static int const GetVerticesBufferStride() { return sizeof(Vertex); }
		static std::vector<int>& const GetIndices() { return indices; }
		static const int GetIndicesBufferSize();
		static const int GetVertexBufferSize();
	};

	struct Box2
	{
	private:
		static std::vector<int> indices;
		static std::vector<Vertex> vertices;
		static ID3D12Resource* m_vertexBuffer;
		static ID3D12Resource* m_indexBuffer;

	public:
		static std::vector<Vertex>& const GetVertices() { return vertices; }
		static ID3D12Resource*& const GetIndicesBuffer() { return m_indexBuffer; }
		static ID3D12Resource*& const GetVerticesBuffer() { return m_vertexBuffer; }
		static int const GetVerticesBufferStride() { return sizeof(Vertex); }
		static std::vector<int>& const GetIndices() { return indices; }
		static const int GetIndicesBufferSize();
		static const int GetVertexBufferSize();
	};

};