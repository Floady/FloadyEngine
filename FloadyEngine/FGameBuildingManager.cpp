#include "FGameBuildingManager.h"
#include "FUtilities.h"

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <string>


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
			myBuildingTemplates[FUtilities::ConvertFromUtf16ToUtf8(data.cFileName)] = new FGameBuildingBlueprint(std::string(FUtilities::ConvertFromUtf16ToUtf8(concatted_stdstr.c_str())).c_str());
		}
		while (FindNextFileW(hFind, &data));

		FindClose(hFind);
	}
}


FGameBuildingManager::~FGameBuildingManager()
{
}
