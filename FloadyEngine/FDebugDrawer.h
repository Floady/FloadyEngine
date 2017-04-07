#pragma once

#include <vector>
#include "btBulletDynamicsCommon.h"
#include "FRenderableObject.h"

class FD3d12Renderer;
struct ID3D12Resource;
struct ID3D12RootSignature;
struct ID3D12PipelineState;
struct ID3D12GraphicsCommandList;
struct D3D12_VERTEX_BUFFER_VIEW;
struct D3D12_INDEX_BUFFER_VIEW;

class FDebugDrawer : public btIDebugDraw, public FRenderableObject
{
public:
	FDebugDrawer(FD3d12Renderer* aManager);
	~FDebugDrawer();

	void	drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override;
	void	drawLine(const FVector3 & from, const FVector3 & to, const FVector3 & color);
	void	reportErrorWarning(const char* warningString) override;
	void	draw3dText(const btVector3& location, const char* textString) override;
	void	setDebugMode(int debugMode) override;
	int		getDebugMode() const override;
	void	drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override;

	void	DrawTriangle(const FVector3& aV1, const FVector3& aV2, const FVector3& aV3, const FVector3& aColor);

	// FRenderableObject
	void Init() override;
	void Render() override;
	void RenderShadows() override {};
	void PopulateCommandListAsync() override;
	void PopulateCommandListAsyncShadows() override {};

	void SetTexture(const char* aFilename) override {};
	void SetShader(const char* aFilename) override {};

private:
	void PopulateCommandListInternal(ID3D12GraphicsCommandList* aCmdList);

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
	ID3D12GraphicsCommandList* m_commandList;

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

