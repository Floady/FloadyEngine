#pragma once

#include "FRenderableObject.h"
#include <vector>
#include "btBulletDynamicsCommon.h"

class FD3d12Renderer;
struct ID3D12Resource;
struct ID3D12RootSignature;
struct ID3D12PipelineState;
struct ID3D12GraphicsCommandList;
struct D3D12_VERTEX_BUFFER_VIEW;
struct D3D12_INDEX_BUFFER_VIEW;

class FDebugDrawer :public FRenderableObject
{
public:
	FDebugDrawer(FD3d12Renderer* aManager);
	~FDebugDrawer();

	void DrawTriangle(const FVector3& aV1, const FVector3& aV2, const FVector3& aV3, const FVector3& aColor);
	void DrawPoint(const FVector3& aV, float aSize, const FVector3& aColor);
	void DrawSphere(const FVector3& aPoint, float aRadius, const FVector3& aColor);
	void drawLine(const FVector3 & from, const FVector3 & to, const FVector3 & color);
	void drawAABB(const FVector3& aMin, const FVector3& aMax, const FVector3 & color);
	void drawAABB(const FAABB& anAABB, const FVector3 & color);
	// FRenderableObject
	void Init() override;
	void Render() override;
	void RenderShadows() override {};
	void PopulateCommandListAsync() override;
	void PopulateCommandListAsyncShadows() override {};
	void PopulateCommandListInternalShadows(ID3D12GraphicsCommandList* aCmdList) override {};

	void SetTexture(const char* aFilename) override {}
	virtual const char* GetTexture() override { return nullptr; }
	void SetShader(const char* aFilename) override {}
	void PopulateCommandListInternal(ID3D12GraphicsCommandList* aCmdList) override;

private:

	struct Line
	{
		FVector3 myStart;
		FVector3 myEnd;
		FVector3 myColor;
	};

	struct Triangle
	{
		FVector3 myVtx1;
		FVector3 myVtx2;
		FVector3 myVtx3;
		FVector3 myColor;
	};

	FD3d12Renderer* myManagerClass;
	ID3D12RootSignature* m_rootSignature;
	
	std::vector<Line> myLines;
	ID3D12PipelineState* myPsoLines;
	D3D12_VERTEX_BUFFER_VIEW* myVertexBufferViewLines;
	ID3D12Resource* myVertexBufferLines;
	UINT8* myGPUVertexDataLines;
	
	std::vector<Triangle> myTriangles;
	ID3D12PipelineState* myPsoTriangles;
	D3D12_VERTEX_BUFFER_VIEW* myVertexBufferViewTriangles;
	ID3D12Resource* myVertexBufferTriangles;
	UINT8* myGPUVertexDataTriangles;

	UINT8* myConstantBufferPtr;
	std::string myTexName;

	ID3D12Resource* m_ModelProjMatrix;

	int myHeapOffsetCBV;
	int myHeapOffsetCBVShadow;
	int myHeapOffsetText;
	int myHeapOffsetAll;
	bool skipNextRender;
};

