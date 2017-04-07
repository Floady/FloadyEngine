#pragma once
#include "FJsonObject.h"

class FJson
{
public:
	static FJsonObject* Parse(const char* aFile);
	FJson();
	~FJson();
};

