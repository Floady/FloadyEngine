#include "FUtilities.h"
#include <vector>
#include "d3dcommon.h"

void FUtilities::FLog(const char * fmt, ...)
{
	int len = strlen(fmt);
	if (len > 1024)
	{
		OutputDebugStringA("<Log message to long>\n");
		return;
	}

	//char buff2[1024 * 16];
	//sprintf(buff2, "%s(%d): %s\n", __FILE__, __LINE__, fmt);

	char buff[1024 * 16];
	va_list args;
	va_start(args, fmt);
	vsprintf(buff, fmt, args);
	va_end(args);
	
	OutputDebugStringA(buff);
}

std::string FUtilities::ConvertFromUtf16ToUtf8(const std::wstring & wstr)
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

std::wstring FUtilities::ConvertFromUtf8ToUtf16(const std::string & str)
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

void FUtilities::OutputShaderErrorMessage(ID3D10Blob * errorMessage)
{
	if (!errorMessage)
		return;

	char* compileErrors;
	size_t bufferSize;

	// Get a pointer to the error message text buffer.
	compileErrors = (char*)(errorMessage->GetBufferPointer());

	// Get the length of the message.
	bufferSize = errorMessage->GetBufferSize();
	FLOG("Shader Error: %s", compileErrors);

	// Release the error message.
	errorMessage->Release();
	errorMessage = 0;

	return;
}