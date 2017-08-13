#pragma once
#include <string>
#include <d3d12.h>
#include <DirectXMath.h>
#include <dxgi1_4.h>
#include "d3dx12.h"
#include "FVector3.h"
#include "FPrimitiveGeometry.h"
#include "FRenderableObject.h"


class FCamera;
class FD3d12Renderer;

class FScreenQuad : public FRenderableObject
{
public:

	FScreenQuad(FD3d12Renderer* aManager, FVector3 aPos, const char* aTexture, float aWidth, float aHeight, bool aUseKerning, bool anIs2D);
	~FScreenQuad();
	void Init() override;
	void Render() override;
	void RenderShadows() override {}
	void PopulateCommandList();
	void PopulateCommandListAsync() override;
	void PopulateCommandListAsyncShadows() override {}
	void SetText(const char* aNewText);
	void SetShader();
	void SetUVOffset(const FVector3& aTL, const FVector3& aBR);

	// dont care for text - could use it to change font
	virtual void SetTexture(const char* aFilename) {}
	virtual const char* GetTexture() override { return nullptr; }
	virtual void SetShader(const char* aFilename) {}
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
	FVector3 myUVTL;
	FVector3 myUVBR;
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

	std::string myTexName;
};

