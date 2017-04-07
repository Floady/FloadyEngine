#include "FFontRenderer.h"
#include "d3dx12.h"
#include "D3dCompiler.h"
#include "FD3d12Renderer.h"
#include "FCamera.h"
#include <vector>

#include <ft2build.h>
#include <ftglyph.h>

#include "FMatrix.h"
#include "FVector3.h"

using namespace DirectX;

static const UINT TexturePixelSize = 4;	// The number of bytes used to represent a pixel in the texture.


static FT_Library  m_library;
static FT_Face     m_face;
										// Generate a simple black and white checkerboard texture.
std::vector<UINT8> GenerateTextureData3(const char* aText, int TextureWidth, int TextureHeight, int wordLength, UINT largestBearing)
{
	const UINT rowPitch = TextureWidth * TexturePixelSize;
	const UINT cellPitch = rowPitch >> 3;		// The width of a cell in the checkboard texture.
	const UINT cellHeight = TextureWidth >> 3;	// The height of a cell in the checkerboard texture.
	const UINT textureSize = rowPitch * TextureHeight;
	
	std::vector<UINT8> data(textureSize);
	UINT8* pData = &data[0];

	// fill buffer
	int xoffset = 0;
	for (int h = 0; h < wordLength; h++)
	{
		int counter = 0;
		FT_Error error = FT_Load_Char(m_face, aText[h], 0);

		error = FT_Render_Glyph(m_face->glyph, FT_RENDER_MODE_NORMAL);
		FT_Bitmap bitmap = m_face->glyph->bitmap;

		//int top = (texHeight - bitmap.rows);
		int glyphHeight = (m_face->glyph->metrics.height >> 6);
		int top = largestBearing - (m_face->glyph->metrics.horiBearingY >> 6);

		xoffset += (m_face->glyph)->bitmap_left * 4;
		int offset = xoffset + top*TextureWidth * 4;

		for (unsigned int i = 0; i < bitmap.rows; i++)
		{
			for (int j = 0; j < bitmap.pitch; j++)
			{
				data[offset + counter++] = bitmap.buffer[i*bitmap.pitch + j];
				data[offset + counter++] = bitmap.buffer[i*bitmap.pitch + j];
				data[offset + counter++] = bitmap.buffer[i*bitmap.pitch + j];
				data[offset + counter++] = 255;
			}

			offset += (TextureWidth - bitmap.pitch) * 4;
		}

		// advance.x = whitespace before glyph + charWidth + whitespace after glyph, we already moved the whitespace before and the charWidth, so we move: advance.x - whiteSpace before (bitmap_left) = whitespace after glyph
		xoffset += ((m_face->glyph)->advance.x >> 6) * 4 - ((m_face->glyph)->bitmap_left * 4);
	}

	return data;
}

FFontRenderer::FFontRenderer(UINT width, UINT height, FVector3 aPos, const char* aText)
	: m_viewport(),
	m_scissorRect()
{
	myPos.x = aPos.x;
	myPos.y = aPos.y;
	myPos.z = aPos.z;

	myText = aText;
	m_viewport.Width = static_cast<float>(width);
	m_viewport.Height = static_cast<float>(height);
	m_viewport.MaxDepth = 1.0f;

	m_scissorRect.right = static_cast<LONG>(width);
	m_scissorRect.bottom = static_cast<LONG>(height);
	m_aspectRatio = (float)width / (float)height;

	FT_Error error = FT_Init_FreeType(&m_library);
	error = FT_New_Face(m_library, "C:/Windows/Fonts/Arial.ttf", 0, &m_face);
	
	int aSize = 150;

	int wordStart = 0;
	int texWidth = 0;
	int texHeight = 0;
	wordLength = static_cast<UINT>(strlen(aText));
	largestBearing = 0;
	error = FT_Set_Char_Size(m_face, 0, aSize * 32, 100, 100);
	FT_Bool hasKerning = FT_HAS_KERNING(m_face);

	FT_UInt prev;

	// calculate buffer dimensions
	for (unsigned int i = 0; i < wordLength; i++)
	{
		error = FT_Load_Char(m_face, aText[i], 0);

		if (i > 0)
		{
			prev = FT_Get_Char_Index(m_face, aText[i - 1]);
			FT_UInt next = FT_Get_Char_Index(m_face, aText[i]);
			FT_Vector delta;
			FT_Get_Kerning(m_face, prev, next, FT_KERNING_DEFAULT, &delta);
			int breakhere = 0;
		}

		texWidth += (m_face->glyph->advance.x >> 6);
		texWidth += (m_face->glyph->metrics.horiBearingX >> 6);
		int height = (m_face->glyph->metrics.height >> 6);
		height += height - (m_face->glyph->metrics.horiBearingY >> 6);

		largestBearing = max(largestBearing, m_face->glyph->metrics.horiBearingY >> 6);
		texHeight = max(texHeight, height);
	}

	TextureWidth = texWidth;
	TextureHeight = texHeight+1; // why +1?
}


FFontRenderer::~FFontRenderer()
{
}

void FFontRenderer::Init(ID3D12CommandAllocator* aCmdAllocator, ID3D12Device* aDevice, D3D12_CPU_DESCRIPTOR_HANDLE& anRTVHandle, ID3D12CommandQueue* aCmdQueue, ID3D12DescriptorHeap* anSRVHeap, ID3D12RootSignature* aRootSig, FD3d12Renderer* aManager)
{
	m_device = aDevice;
	myManagerClass = aManager;
	HRESULT hr;

	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

		// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

		if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}


		CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

		CD3DX12_ROOT_PARAMETER1 rootParameters[1];
		rootParameters[0].InitAsDescriptorTable(2, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC  rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ID3DBlob* signature;
		ID3DBlob* error;
		hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error);
		hr = aDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));

	}

	ID3DBlob* vertexShader;
	ID3DBlob* pixelShader;

	UINT compileFlags = 0;
	D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr);
	D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr);

	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_rootSignature;
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = TRUE;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;

	hr = aDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));

	hr = aDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, aCmdAllocator, m_pipelineState, IID_PPV_ARGS(&m_commandList));
	m_commandList->SetName(L"FFontRenderer");



	// Create the vertex buffer.
	{
		// scale to viewport
		float texWidthRescaled = (float)TextureWidth / m_viewport.Width;
		float texHeightRescaled = (float)TextureHeight / m_viewport.Width;

		const float texMultiplierSize = 10.0f;
		texWidthRescaled *= texMultiplierSize;
		texHeightRescaled *= texMultiplierSize;

		//half it
		texWidthRescaled /= 2.0f;
		texHeightRescaled /= 2.0f;

		const float quadZ = 1.0f;
		Vertex triangleVertices[] =
		{
			{ { -texWidthRescaled, texHeightRescaled * m_aspectRatio, quadZ },{ 0.0f, 0.0f } },
			{ { texWidthRescaled, -texHeightRescaled * m_aspectRatio, quadZ },{ 1.0f, 1.0f } },
			{ { -texWidthRescaled, -texHeightRescaled * m_aspectRatio, quadZ },{ 0.0f, 1.0f } },

			{ { -texWidthRescaled, texHeightRescaled * m_aspectRatio, quadZ },{ 0.0f, 0.0f } },
			{ { texWidthRescaled, texHeightRescaled * m_aspectRatio, quadZ },{ 1.0f, 0.0f } },
			{ { texWidthRescaled, -texHeightRescaled * m_aspectRatio, quadZ },{ 1.0f, 1.0f } }
		};


		const UINT vertexBufferSize = sizeof(triangleVertices);

		// Note: using upload heaps to transfer static data like vert buffers is not 
		// recommended. Every time the GPU needs it, the upload heap will be marshalled 
		// over. Please read up on Default Heap usage. An upload heap is used here for 
		// code simplicity and because there are very few verts to actually transfer.
		hr = aDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer));

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		hr = m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
		memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
		m_vertexBuffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(Vertex);
		m_vertexBufferView.SizeInBytes = vertexBufferSize;

		//create 
		hr = aDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(256),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_ModelProjMatrix));

		hr = m_ModelProjMatrix->Map(0, &readRange, reinterpret_cast<void**>(&myConstantBufferPtr));
		
		//m_ModelProjMatrix->Unmap(0, nullptr); // we dont need this i think? that way we can update cb in render func without map
		
		myHeapOffsetCBV = myManagerClass->GetNextOffset();
		myHeapOffsetAll = myHeapOffsetCBV;
		// Describe and create a constant buffer view.
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc[1] = {};
		cbvDesc[0].BufferLocation = m_ModelProjMatrix->GetGPUVirtualAddress();
		cbvDesc[0].SizeInBytes = 256; // required to be 256 bytes aligned -> (sizeof(ConstantBuffer) + 255) & ~255
		unsigned int srvSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle0(anSRVHeap->GetCPUDescriptorHandleForHeapStart(), myHeapOffsetCBV, srvSize);
		aDevice->CreateConstantBufferView(cbvDesc, cbvHandle0);

		// TEXTURE

		// Describe and create a shader resource view (SRV) heap for the texture.
		//D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		//srvHeapDesc.NumDescriptors = 1;
		//srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		//srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		//m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap));
		//
		ID3D12Resource* textureUploadHeap;
		// Create the texture.
		{
			// Describe and create a Texture2D.
			D3D12_RESOURCE_DESC textureDesc = {};
			textureDesc.MipLevels = 1;
			textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			textureDesc.Width = TextureWidth;
			textureDesc.Height = TextureHeight;
			textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			textureDesc.DepthOrArraySize = 1;
			textureDesc.SampleDesc.Count = 1;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

			hr = aDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&textureDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&m_texture));

			// these indices are also wrong -> need to be global for upload heap? get from device i guess
			const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture, 0, 1);

			// Create the GPU upload buffer.
			aDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&textureUploadHeap));

			// Copy data to the intermediate upload heap and then schedule a copy 
			// from the upload heap to the Texture2D.
			std::vector<UINT8> texture = GenerateTextureData3(myText, TextureWidth, TextureHeight, wordLength, largestBearing);

			D3D12_SUBRESOURCE_DATA textureData = {};
			textureData.pData = &texture[0];
			textureData.RowPitch = TextureWidth * TexturePixelSize;
			textureData.SlicePitch = textureData.RowPitch * TextureHeight;

			UpdateSubresources(m_commandList, m_texture, textureUploadHeap, 0, 0, 1, &textureData);

			m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_texture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

			// Describe and create a SRV for the texture.
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = textureDesc.Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;


			// Get the size of the memory location for the render target view descriptors.
			unsigned int srvSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			myHeapOffsetText = myManagerClass->GetNextOffset();
			CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle0(anSRVHeap->GetCPUDescriptorHandleForHeapStart(), myHeapOffsetText, srvSize);
			aDevice->CreateShaderResourceView(m_texture, &srvDesc, srvHandle0);
		}

		m_commandList->Close();

		// do we need this?
		ID3D12CommandList* ppCommandLists[] = { m_commandList };
		aCmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}


}

void FFontRenderer::Render(ID3D12Resource* aRenderTarget, ID3D12CommandAllocator* aCmdAllocator, ID3D12CommandQueue* aCmdQueue, D3D12_CPU_DESCRIPTOR_HANDLE& anRTVHandle, D3D12_CPU_DESCRIPTOR_HANDLE& aDSVHandle, ID3D12DescriptorHeap* anSRVHeap, FCamera* aCam)
{
	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	HRESULT hr;

	memcpy(myConstantBufferPtr, aCam->GetViewProjMatrixWithOffset(myPos.x, myPos.y, myPos.z).m, sizeof(XMFLOAT4X4));

	hr = m_commandList->Reset(aCmdAllocator, m_pipelineState);
	
	// Set necessary state.
	m_commandList->SetGraphicsRootSignature(m_rootSignature);
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetDepthBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// is this how we bind textures?
	ID3D12DescriptorHeap* ppHeaps[] = { anSRVHeap };
	m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// Get the size of the memory location for the render target view descriptors.
	unsigned int srvSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

/*
this is how it was initialized:
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
*/
	D3D12_GPU_DESCRIPTOR_HANDLE handle = anSRVHeap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += srvSize*myHeapOffsetAll;
	m_commandList->SetGraphicsRootDescriptorTable(0, handle);
	
	// set viewport/scissor
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// Indicate that the back buffer will be used as a render target.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(aRenderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	m_commandList->OMSetRenderTargets(1, &anRTVHandle, FALSE, &aDSVHandle);

	// Record commands.
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->DrawInstanced(6, 1, 0, 0);

	// Indicate that the back buffer will now be used to present.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(aRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetDepthBuffer(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	hr = m_commandList->Close();

	ID3D12CommandList* ppCommandLists[] = { m_commandList };
	aCmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}
