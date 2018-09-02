#include "FImage.h"

#include "FreeImage.h"

class FImage_FreeImageLoader
{
public:

	static FImage_FreeImageLoader* GetInstance() {
		if (!ourInstance)
			ourInstance = new FImage_FreeImageLoader();
		return ourInstance;
	}

private:
	FImage_FreeImageLoader()
	{
		FreeImage_Initialise();
	}

	static FImage_FreeImageLoader* ourInstance;
};

FImage_FreeImageLoader* FImage_FreeImageLoader::ourInstance = nullptr;

bool FImage::Load(const char * aPath)
{	
	// init freeImage
	FImage_FreeImageLoader::GetInstance();

	//image format
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	//pointer to the image, once loaded
	FIBITMAP *dib(0);
	//pointer to the image data
	BYTE* bits = 0;

	//image width and height
	unsigned int width(0), height(0);

	//check the file signature and deduce its format
	fif = FreeImage_GetFileType(aPath, 0);
	//if still unknown, try to guess the file format from the file extension
	if (fif == FIF_UNKNOWN)
		fif = FreeImage_GetFIFFromFilename(aPath);
	//if still unkown, return failure
	if (fif == FIF_UNKNOWN)
		return false;

	//check that the plugin has reading capabilities and load the file
	if (FreeImage_FIFSupportsReading(fif))
		dib = FreeImage_Load(fif, aPath);
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
	myPixelData = new unsigned char[texsize];
	myWidth = width;
	myHeight = height;

	for (size_t j = 0; j < height; j++)
	{
		for (size_t i = 0; i < width; i++)
		{
			tagRGBQUAD val;
			FreeImage_GetPixelColor(dib, i, (height - j), &val);
			myPixelData[(j * width + i)*texpixelsize] = val.rgbRed;
			myPixelData[(j * width + i)*texpixelsize + 1] = val.rgbGreen;
			myPixelData[(j * width + i)*texpixelsize + 2] = val.rgbBlue;
			myPixelData[(j * width + i)*texpixelsize + 3] = 255;
		}
	}

	//Free FreeImage's copy of the data
	FreeImage_Unload(dib);

	//return success
	return true;
}

FImage::FImage()
{
	myPixelData = nullptr;
	myWidth = 0;
	myHeight = 0;
}


FImage::~FImage()
{
	delete myPixelData;
}
