#include "FTextureManager.h"
#include <png.h>
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <vector>
#include <stdio.h>
#include "FUtilities.h"
#include "FreeImage.h"
#include "FProfiler.h"

static std::string ourFallbackTextureName = "reserved_fallback.reserved";
//#pragma optimize("", off)

namespace
{
	std::string ConvertFromUtf16ToUtf8(const std::wstring& wstr)
	{
		std::string convertedString;
		int requiredSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, 0, 0, 0, 0);
		if (requiredSize > 0)
		{
			std::vector<char> buffer(requiredSize);
			WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &buffer[0], requiredSize, 0, 0);
			convertedString.assign(buffer.begin(), buffer.end() - 1);
		}
		return convertedString;
	}

	std::wstring ConvertFromUtf8ToUtf16(const std::string& str)
	{
		std::wstring convertedString;
		int requiredSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, 0, 0);
		if (requiredSize > 0)
		{
			std::vector<wchar_t> buffer(requiredSize);
			MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &buffer[0], requiredSize);
			convertedString.assign(buffer.begin(), buffer.end() - 1);
		}

		return convertedString;
	}

}

// Generate a simple black and white checkerboard texture.

UINT8* FTextureManager::GenerateCheckerBoard()
{
	UINT TextureWidth = 256;
	UINT TextureHeight = 256;
	UINT TexturePixelSize = 4;

	const UINT rowPitch = TextureWidth * TexturePixelSize;
	const UINT cellPitch = rowPitch >> 3;		// The width of a cell in the checkboard texture.
	const UINT cellHeight = TextureWidth >> 3;	// The height of a cell in the checkerboard texture.
	const UINT textureSize = rowPitch * TextureHeight;

	//data.resize(textureSize);
	UINT8* pData = new UINT8[textureSize];

	for (UINT n = 0; n < textureSize; n += TexturePixelSize)
	{
		UINT x = n % rowPitch;
		UINT y = n / rowPitch;
		UINT i = x / cellPitch;
		UINT j = y / cellHeight;

		if (i % 2 == j % 2)
		{
			pData[n] = 0xff;		// R
			pData[n + 1] = 0x00;	// G
			pData[n + 2] = 0x00;	// B
			pData[n + 3] = 0xff;	// A
		}
		else
		{
			pData[n] = 0xff;		// R
			pData[n + 1] = 0xff;	// G
			pData[n + 2] = 0xff;	// B
			pData[n + 3] = 0xff;	// A
		}
	}

	return pData;
}

FTextureManager* FTextureManager::ourInstance = nullptr;

int width, height;
int fi_width, fi_height;
png_byte color_type;
png_byte bit_depth;
png_bytep* row_pointers;
UINT8* transformedBytes = nullptr;
UINT8* transformedBytes2 = nullptr;
void read_png_file(const char *filename) {
	FILE *fp;
	
	fopen_s(&fp, filename, "rb");

	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png) abort();

	png_infop info = png_create_info_struct(png);
	if (!info) abort();

	if (setjmp(png_jmpbuf(png))) abort();

	png_init_io(png, fp);

	png_read_info(png, info);

	width = png_get_image_width(png, info);
	height = png_get_image_height(png, info);
	color_type = png_get_color_type(png, info);
	bit_depth = png_get_bit_depth(png, info);

	// Read any color_type into 8bit depth, RGBA format.
	// See http://www.libpng.org/pub/png/libpng-manual.txt

	if (bit_depth == 16)
		png_set_strip_16(png);

	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png);

	// PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png);

	if (png_get_valid(png, info, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png);

	// These color_type don't have an alpha channel then fill it with 0xff.
	if (color_type == PNG_COLOR_TYPE_RGB ||
		color_type == PNG_COLOR_TYPE_GRAY ||
		color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

	if (color_type == PNG_COLOR_TYPE_GRAY ||
		color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png);

	png_read_update_info(png, info);

	row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
	size_t rowcounter = png_get_rowbytes(png, info);
	for (int y = 0; y < height; y++) {
		//row_pointers[y] = (png_byte*)malloc(rowcounter);
		row_pointers[y] = (png_byte*)png_malloc(png, png_get_rowbytes(png, info));
	}

	png_read_image(png, row_pointers);
	const int texpixelsize = 4;
	int texsize = 4096 * 4096 * texpixelsize;// height * width * texpixelsize; //Todo: crash if buffer is smaller
	transformedBytes = (UINT8*)malloc(texsize);

	for (size_t i = 0; i < height; i++)
	{
		png_byte* rowdata = row_pointers[i];
		for (size_t j = 0; j < width*texpixelsize; j += texpixelsize)
		{
			transformedBytes[i * height*texpixelsize + j]		= rowdata[j];
			transformedBytes[i * height*texpixelsize + j + 1]	= rowdata[j + 1];
			transformedBytes[i * height*texpixelsize + j + 2]	= rowdata[j + 2];
			transformedBytes[i * height*texpixelsize + j + 3]	= rowdata[j + 3];
		}
	}

	fclose(fp);
}


bool LoadTexture(const char* filename)
{
	//image format
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	//pointer to the image, once loaded
	FIBITMAP *dib(0);
	//pointer to the image data
	BYTE* bits = 0;

	//image width and height
	unsigned int width(0), height(0);

	//check the file signature and deduce its format
	fif = FreeImage_GetFileType(filename, 0);
	//if still unknown, try to guess the file format from the file extension
	if (fif == FIF_UNKNOWN)
		fif = FreeImage_GetFIFFromFilename(filename);
	//if still unkown, return failure
	if (fif == FIF_UNKNOWN)
		return false;

	//check that the plugin has reading capabilities and load the file
	if (FreeImage_FIFSupportsReading(fif))
		dib = FreeImage_Load(fif, filename);
	//if the image failed to load, return failure
	if (!dib)
		return false;

	//retrieve the image data
	bits = FreeImage_GetBits(dib);
	//get the image width and height
	width = FreeImage_GetWidth(dib);
	height = FreeImage_GetHeight(dib);
	//if this somehow one of these failed (they shouldn't), return failure
	if ((bits == 0) || (width == 0) || (height == 0))
		return false;

	// make tex?
	const int texpixelsize = 4;
	int texsize = width * height * 4;
	transformedBytes2 = (UINT8*)malloc(texsize);
	fi_width = width;
	fi_height = height;

	for (size_t j = 0; j < height; j++)
	{
		for (size_t i = 0; i < width; i++)
		{
			tagRGBQUAD val;
			FreeImage_GetPixelColor(dib, i, (height-j), &val);
			transformedBytes2[(j * width + i)*texpixelsize] = val.rgbRed;
			transformedBytes2[(j * width + i)*texpixelsize + 1] = val.rgbGreen;
			transformedBytes2[(j * width + i)*texpixelsize + 2] = val.rgbBlue;
			transformedBytes2[(j * width + i)*texpixelsize + 3] = 255;
		}
	}

	//Free FreeImage's copy of the data
	FreeImage_Unload(dib);

	//return success
	return true;
}

void FTextureManager::ReloadTextures()
{
	HANDLE hFind;
	WIN32_FIND_DATA data;

	//myTextures.clear();
	//myTextureMutex.WaitFor();
	//myTextureMutex.Lock();

	LPCWSTR folderFilters[] = { L"Textures//*.png" ,L"Textures//*.jpg" , L"Textures//sponza//*.png", L"Textures//sponza2//*.tga" };
	LPCWSTR folderPrefix[] = { L"Textures//" ,  L"Textures//" , L"Textures//sponza//" , L"Textures//sponza2//" };

	for (size_t i = 0; i < _countof(folderFilters); i++)
	{
		hFind = FindFirstFileW(folderFilters[i], &data);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
//				FLOG("%ws", data.cFileName);

				std::wstring mywstring(data.cFileName);
				std::wstring concatted_stdstr = folderPrefix[i] + mywstring;

				std::string cStrFilename = ConvertFromUtf16ToUtf8(mywstring);
				std::string cStrFilenameConcat = ConvertFromUtf16ToUtf8(concatted_stdstr);

				if(true)
				{
					{
					//	FPROFILE_FUNCTION("FAssetImg Load");

						if (!LoadTexture(cStrFilenameConcat.c_str()))
							FLOG("Failed loading: %ws", data.cFileName);
					}
					myTextures[cStrFilename].myWidth = fi_width;
					myTextures[cStrFilename].myHeight = fi_height;
					myTextures[cStrFilename].myRawPixelData = transformedBytes2;
//					FLOG("Taking load data from FreeImage for: %ws", data.cFileName);
				}
				else
				{
					read_png_file(cStrFilenameConcat.c_str());

					myTextures[cStrFilename].myWidth = width;
					myTextures[cStrFilename].myHeight = height;
					myTextures[cStrFilename].myRawPixelData = transformedBytes;
//					FLOG("Taking load data from libPNG straight for: %ws", data.cFileName);
				}

				myTextures[cStrFilename].myD3DResource = nullptr;	// will be filled later (init with device) this way you can pre-empt png loading while device is being made
																	// also disconnects from render device, so we can use texture + resource managing with other devices
			} while (FindNextFileW(hFind, &data));
			FindClose(hFind);
		}
	}

	//myTextureMutex.Unlock();
}

void FTextureManager::ReleaseTextures()
{
	for (std::map<std::string, TextureInfo>::iterator it = myTextures.begin(); it != myTextures.end(); ++it)
	{
		if((*it).second.myD3DResource)
			(*it).second.myD3DResource->Release();
	}
}

ID3D12Resource * FTextureManager::GetTextureD3D(const std::string& aTextureName) const
{
	assert(myInitializedD3D && "NOT INIT D3D");

	if (myTextures.find(aTextureName) != myTextures.end())
		return myTextures.find(aTextureName)->second.myD3DResource;

	return nullptr;
}

ID3D12Resource * FTextureManager::GetTextureD3DFallback() const
{
	assert(myInitializedD3D && "NOT INIT D3D");
	return myTextures.find(ourFallbackTextureName)->second.myD3DResource;
}

ID3D12Resource * FTextureManager::GetTextureD3D(const char * aTextureName) const
{
	return GetTextureD3D(std::string(aTextureName));
}

void write_png_file(char *filename) 
{
	FILE *fp;
	fopen_s(&fp, filename, "wb");
	if (!fp) abort();

	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png) abort();

	png_infop info = png_create_info_struct(png);
	if (!info) abort();

	if (setjmp(png_jmpbuf(png))) abort();

	png_init_io(png, fp);

	// Output is 8bit depth, RGBA format.
	png_set_IHDR(
		png,
		info,
		width, height,
		8,
		PNG_COLOR_TYPE_RGBA,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT
	);
	png_write_info(png, info);

	// To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
	// Use png_set_filler().
	//png_set_filler(png, 0, PNG_FILLER_AFTER);

	png_write_image(png, row_pointers);
	png_write_end(png, NULL);

	for (int y = 0; y < height; y++) {
		free(row_pointers[y]);
	}
	free(row_pointers);

	fclose(fp);
}

FTextureManager::FTextureManager()
{
//	HANDLE hFind;
//	WIN32_FIND_DATA data;
	void* tex = static_cast<void*>(FTextureManager::GenerateCheckerBoard());
	
	myTextures[ourFallbackTextureName].myRawPixelData = tex;
	myTextures[ourFallbackTextureName].myWidth = 256;
	myTextures[ourFallbackTextureName].myHeight = 256;
	myTextures[ourFallbackTextureName].myD3DResource = nullptr;
	
	myInitializedD3D = false;
	FreeImage_Initialise();

	myTextureMutex.Init();
}

void FTextureManager::InitD3DResources(ID3D12Device* aDevice, ID3D12GraphicsCommandList* aCommandList)
{
	//myTextureMutex.WaitFor();
	//myTextureMutex.Lock();

	const int maxItems = 5;
	int nrOfItemsDone = 0;
	for (std::map<std::string, TextureInfo>::iterator it = myTextures.begin(); it != myTextures.end(); ++it)
	{
		if (nrOfItemsDone >= maxItems)
		{
			break;
		}

		TextureInfo& texture = it->second;
		if (texture.myRawPixelData && !texture.myD3DResource)
		{
			nrOfItemsDone++;
			FLOG("init: %s", it->first.c_str());
			// Describe and create a Texture2D.
			D3D12_RESOURCE_DESC textureDesc = {};
			textureDesc.MipLevels = 1;
			textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			textureDesc.Width = texture.myWidth;
			textureDesc.Height = texture.myHeight;
			textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			textureDesc.DepthOrArraySize = 1;
			textureDesc.SampleDesc.Count = 1;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

			HRESULT hr = aDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&textureDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&texture.myD3DResource));

			if (FAILED(hr))
			{
				HRESULT hr2 = FD3d12Renderer::GetInstance()->GetDevice()->GetDeviceRemovedReason();
				FLOG("CreateCommitedResource failed %x [%x]", hr, hr2);
			}

			// Create the GPU upload buffer.
			ID3D12Resource* textureUploadHeap;
			const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.myD3DResource, 0, 1);
			hr = aDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&textureUploadHeap));

			if (FAILED(hr))
			{
				HRESULT hr2 = FD3d12Renderer::GetInstance()->GetDevice()->GetDeviceRemovedReason();
				FLOG("CreateCommitedResource failed %x [%x]", hr, hr2);
			}

			// Copy data to the intermediate upload heap and then schedule a copy 
			// from the upload heap to the Texture2D.
			void* texturePixelData = texture.myRawPixelData;
			D3D12_SUBRESOURCE_DATA textureData = {};
			textureData.pData = texturePixelData;
			textureData.RowPitch = texture.myWidth * 4;
			textureData.SlicePitch = textureData.RowPitch * texture.myHeight;

			UpdateSubresources(aCommandList, texture.myD3DResource, textureUploadHeap, 0, 0, 1, &textureData);
			aCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture.myD3DResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
		}
	}

	myInitializedD3D = true;
	
//	myTextureMutex.Unlock();
}

FTextureManager::~FTextureManager()
{
}

FTextureManager * FTextureManager::GetInstance()
{
	if (!ourInstance)
		ourInstance = new FTextureManager();

	return ourInstance;
}
