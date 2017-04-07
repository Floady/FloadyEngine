#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <dxgi1_4.h>
#include "d3dx12.h"
#include "FVector3.h"
#include "FPrimitiveGeometry.h"
#include "FRenderableObject.h"

class FCamera;
class FD3d12Renderer;

class FDynamicText : public FRenderableObject
{
public:

	FDynamicText(FD3d12Renderer* aManager, FVector3 aPos, const char* aText, float aWidth, float aHeight, bool aUseKerning, bool anIs2D);
	~FDynamicText();
	void Init() override;
	void Render() override;
	void RenderShadows() override {}
	void PopulateCommandList();
	void PopulateCommandListAsync() override;
	void PopulateCommandListAsyncShadows() override {}
	void SetText(const char* aNewText);
	void SetShader();

	// dont care for text - could use it to change font
	virtual void SetTexture(const char* aFilename) {};
	virtual void SetShader(const char* aFilename) {};
private:
	bool myUseKerning;

	ID3D12RootSignature* m_rootSignature;
	ID3D12PipelineState* m_pipelineState;
	ID3D12GraphicsCommandList* m_commandList;
	
	UINT8* myConstantBufferPtr;
	UINT8* pVertexDataBegin;

	ID3D12Resource* m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	ID3D12Resource* m_ModelProjMatrix;
	
	FVector3 myPos;
	char myText[128];
	FD3d12Renderer* myManagerClass;

	int myHeapOffsetCBV;
	int myHeapOffsetText;
	int myHeapOffsetAll;

	UINT myWordLength;
	
	bool skipNextRender;
	bool firstFrame;
	bool myIs2D;
	bool myIsDeferred;
	const char* myShaderName;

	float myWidth;
	float myHeight;
};

