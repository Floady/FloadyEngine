#include "FGameBuildingManager.h"

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <string>



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


FGameBuildingManager::FGameBuildingManager()
{
	HANDLE hFind;
	WIN32_FIND_DATA data;

	myBuildingTemplates.clear();

	hFind = FindFirstFileW(L"configs//buildings//*.json", &data);
	if (hFind != INVALID_HANDLE_VALUE) 
	{
		do
		{
			printf("%ws\n", data.cFileName);
			std::wstring mywstring(data.cFileName);
			std::wstring concatted_stdstr = L"configs//buildings//" + mywstring;
			myBuildingTemplates[ConvertFromUtf16ToUtf8(data.cFileName)] = new FGameBuildingBlueprint(std::string(ConvertFromUtf16ToUtf8(concatted_stdstr.c_str())).c_str());
		}
		while (FindNextFileW(hFind, &data));

		FindClose(hFind);
	}
}


FGameBuildingManager::~FGameBuildingManager()
{
}
