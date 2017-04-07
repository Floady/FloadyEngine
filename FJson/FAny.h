#pragma once
#include "reader.h"
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <string>

using namespace rapidjson;
using namespace std;

class FAny
{
private:
	FAny(const FAny& anOther) { myType = anOther.myType;  memcpy(&myIntegerStorage, &anOther.myIntegerStorage, sizeof(unsigned int)); }
	static int AddString(const string& aString);
	static std::map<unsigned int, string> ourStringDict;

public:
	// optimize this  (union assignment for numbers+bool?)
	FAny() { myType = FAny_Type::Invalid; }
	FAny(bool aBool) { myType = FAny_Type::Bool; myIntegerStorage = aBool; }
	FAny(unsigned int anUnsignedInt) { myType = FAny_Type::Uint;  myIntegerStorage = anUnsignedInt; }
	FAny(int aSignedInt) { myType = FAny_Type::Int; myIntegerStorage = aSignedInt; }
	FAny(int64_t anInt64) { myType = FAny_Type::Int64;  myIntegerStorage = anInt64; }
	FAny(uint64_t aUInt64) { myType = FAny_Type::Uint64; myIntegerStorage = aUInt64; } // memcpy(&myItemPtr, &aUInt64, sizeof(uint64_t)); 
	FAny(const char* aString) { myType = FAny_Type::String; myIntegerStorage = AddString(string(aString)); } // fix this up with some global stringdictionary
	FAny(double aDouble) { myType = FAny_Type::Double;  myDoubleStorage = aDouble; }

	template <typename T>
	const T  GetAs() const { return static_cast<const T>(myIntegerStorage); }

	template <>
	const double  GetAs() const { return static_cast<const double>(myDoubleStorage); }

	template <>
	const string GetAs() const { return ourStringDict[myIntegerStorage]; }


	~FAny() {}

	bool IsBool() { return myType == FAny_Type::Bool; }
	bool IsUint() { return myType == FAny_Type::Uint; }
	bool IsInt() { return myType == FAny_Type::Int; }
	bool IsUint64() { return myType == FAny_Type::Uint64; }
	bool IsInt64() { return myType == FAny_Type::Int64; }
	bool IsString() { return myType == FAny_Type::String; }
	bool IsDouble() { return myType == FAny_Type::Double; }

protected:
	enum class FAny_Type : char
	{
		Bool = 0,
		Uint,
		Int,
		Uint64,
		Int64,
		String,
		Double,
		Invalid
	};
	union
	{
		uint64_t myIntegerStorage;
		double myDoubleStorage;
	};

	FAny_Type myType;
};
