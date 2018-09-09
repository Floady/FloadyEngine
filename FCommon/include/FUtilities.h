#pragma once
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <string>

struct ID3D10Blob;

#define FLOG(x, ...) { \
char buff2[1024 * 16]; \
sprintf(buff2, "%s(%d): %s\n", __FILE__, __LINE__, x); \
FUtilities::FLog(buff2, __VA_ARGS__); \
}

namespace FUtilities
{
	void FLog(const char* fmt, ...);

	std::string ConvertFromUtf16ToUtf8(const std::wstring& wstr);
	std::wstring ConvertFromUtf8ToUtf16(const std::string& str);
	void OutputShaderErrorMessage(ID3D10Blob* errorMessage);
}