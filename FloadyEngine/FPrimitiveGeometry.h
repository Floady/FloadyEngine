#pragma once
#include <DirectXMath.h>
#include "FD3d12Renderer.h"

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

		bool operator==(const Vertex& other) const {
			return position.x == other.position.x
				&& position.y == other.position.y
				&& position.z == other.position.z
				&& normal.x == other.normal.x
				&& normal.y == other.normal.y
				&& normal.z == other.normal.z
				&& uv.x == other.uv.x
				&& uv.y == other.uv.y;
		}

		DirectX::XMFLOAT4 position;
		DirectX::XMFLOAT4 normal;
		DirectX::XMFLOAT4 uv;
	};

	struct Vertex2
	{
		Vertex2()
		{
			position.x = 0; position.y = 0; position.z = 0;
			normal.x = 0; normal.y = 0; normal.z = -1; uv.x = 0; uv.y = 0;
			position.w = 1.0f;
			normal.w = 1.0f;
			uv.z = 0.0f;
			uv.w = 0.0f;
			matId = 0;
			normalmatId = 0;
		}

		Vertex2(const Vertex& anOther)
		{
			position.x = anOther.position.x; position.y = anOther.position.y; position.z = anOther.position.z;
			normal.x = anOther.normal.x; normal.y = anOther.normal.y; normal.z = -1; uv.x = anOther.uv.x; uv.y = anOther.uv.y;
			position.w = 1.0f;
			normal.w = 1.0f;
			uv.z = 0.0f;
			uv.w = 0.0f;
			matId = 0;
			normalmatId = 0;
		}

		Vertex2(float x, float y, float z, float nx, float ny, float nz, float u, float v)
		{
			position.x = x; position.y = y; position.z = z;
			normal.x = nx; normal.y = ny; normal.z = nz; uv.x = u; uv.y = v;
			position.w = 1.0f;
			normal.w = 1.0f;
			uv.z = 0.0f;
			uv.w = 0.0f;
			matId = 0;
			normalmatId = 0;
		}

		bool operator==(const Vertex2& other) const {
			return position.x == other.position.x
				&& position.y == other.position.y
				&& position.z == other.position.z
				&& normal.x == other.normal.x
				&& normal.y == other.normal.y
				&& normal.z == other.normal.z
				&& uv.x == other.uv.x
				&& uv.y == other.uv.y
				&& matId == other.matId
				&& normalmatId == other.normalmatId;
		}

		DirectX::XMFLOAT4 position;
		DirectX::XMFLOAT4 normal;
		DirectX::XMFLOAT4 uv;
		unsigned int matId;
		unsigned int normalmatId;
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
		static std::vector<Vertex>& GetVertices() {	return vertices; }
		static ID3D12Resource*& GetIndicesBuffer() {	return m_indexBuffer;	}
		static ID3D12Resource*& GetVerticesBuffer() { return m_vertexBuffer; }
		static int const GetVerticesBufferStride() { return sizeof(Vertex); }
		static std::vector<int>& GetIndices() { return indices; }
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
		static std::vector<Vertex>& GetVertices() { return vertices; }
		static ID3D12Resource*& GetIndicesBuffer() { return m_indexBuffer; }
		static ID3D12Resource*& GetVerticesBuffer() { return m_vertexBuffer; }
		static int const GetVerticesBufferStride() { return sizeof(Vertex); }
		static std::vector<int>& GetIndices() { return indices; }
		static const int GetIndicesBufferSize();
		static const int GetVertexBufferSize();
	};

};

