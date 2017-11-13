#include "FPrimitiveBoxMultiTex.h"
#include "FVector3.h"
#include "d3dx12.h"
#include "FD3d12Renderer.h"
#include "FCamera.h"
#include <vector>

#include "FDelegate.h"
#include "FFontManager.h"
#include "FJobSystem.h"
#include "FTextureManager.h"
#include "FLightManager.h"
#include "FPrimitiveGeometry.h"
#include "FProfiler.h"

#include "FObjLoader.h"

void FPrimitiveBoxMultiTex::Init()
{
	FPrimitiveBox::Init();

	// get root sig with 32 SRV's
	m_rootSignature = FD3d12Renderer::GetInstance()->GetRootSignature(64, 1);
	m_rootSignature->SetName(L"PrimitiveBoxMultiTex");

	// set to multitex shader
	SetShader("primitiveshader_deferredMultiTex.hlsl");
	SetShader();
	myManagerClass->GetShaderManager().RegisterForHotReload(shaderfilename, this, FDelegate2<void()>::from<FPrimitiveBox, &FPrimitiveBox::SetShader>(this));
	
	// map all textures to the slots
	const FObjLoader::FObjMesh& m = myObjMesh;
	int matcounter = 0;
	std::string name;
	for (size_t i = 0; i < m.myMaterials.size(); i++)
	{
		const tinyobj::material_t& mat = m.myMaterials[i];

		if (!mat.diffuse_texname.empty())
		{
			name = mat.diffuse_texname.substr(mat.diffuse_texname.find('\\') + 1, mat.diffuse_texname.length());
		}
			
		if (!FD3d12Renderer::GetInstance()->BindTexture(name))
		{
			FD3d12Renderer::GetInstance()->GetNextOffset();
		}
	}

	for (size_t i = 0; i < m.myMaterials.size(); i++)
	{
		const tinyobj::material_t& mat = m.myMaterials[i];
		if (!mat.bump_texname.empty())
		{
			name = mat.bump_texname.substr(mat.bump_texname.find('\\') + 1, mat.bump_texname.length());
		}

		if (!FD3d12Renderer::GetInstance()->BindTexture(name))
		{
			FD3d12Renderer::GetInstance()->GetNextOffset();
		}
	}


	for (size_t i = m.myMaterials.size() * 2; i < 63; i++)
	{
		FD3d12Renderer::GetInstance()->BindTexture("");
	}
}


FPrimitiveBoxMultiTex::FPrimitiveBoxMultiTex(FD3d12Renderer* aRenderer, FVector3 aPos, FVector3 aScale, FPrimitiveBox::PrimitiveType aType)
	: FPrimitiveBox(aRenderer, aPos, aScale, aType)
{
}


FPrimitiveBoxMultiTex::~FPrimitiveBoxMultiTex()
{
}
