#include "FTextureManager.h"
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <vector>
#include <stdio.h>

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
FTextureManager* FTextureManager::ourInstance = nullptr;

int width, height;
png_byte color_type;
png_byte bit_depth;
png_bytep *row_pointers;
UINT8* transformedBytes;
void read_png_file(const char *filename) {
	FILE *fp = fopen(filename, "rb");

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
		row_pointers[y] = (png_byte*)malloc(rowcounter);
	}

	png_read_image(png, row_pointers);
	const int texpixelsize = 4;
	const int texsize = height * width * texpixelsize;
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

void FTextureManager::ReloadTextures()
{
	HANDLE hFind;
	WIN32_FIND_DATA data;

	myTextures.clear();

	hFind = FindFirstFileW(L"Textures//*.png", &data);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			printf("%ws\n", data.cFileName);

			std::wstring mywstring(data.cFileName);
			std::wstring concatted_stdstr = L"Textures//" + mywstring;

			std::string cStrFilename = ConvertFromUtf16ToUtf8(mywstring);
			std::string cStrFilenameConcat = ConvertFromUtf16ToUtf8(concatted_stdstr);
			read_png_file(cStrFilenameConcat.c_str());
			myTextures[cStrFilename] = transformedBytes;
		} while (FindNextFileW(hFind, &data));
		FindClose(hFind);
	}
}

void write_png_file(char *filename) 
{
	FILE *fp = fopen(filename, "wb");
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

	ReloadTextures();
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
