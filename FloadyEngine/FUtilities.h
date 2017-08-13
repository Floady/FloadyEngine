#pragma once
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <string>

struct ID3D10Blob;

namespace FUtilities
{
	void FLog(const char* fmt, ...);

	std::string ConvertFromUtf16ToUtf8(const std::wstring& wstr);
	std::wstring ConvertFromUtf8ToUtf16(const std::string& str);
	void OutputShaderErrorMessage(ID3D10Blob* errorMessage);
}