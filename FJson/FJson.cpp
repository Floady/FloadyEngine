#include "FJson.h"
#include "reader.h"
#include <iostream>
#include "FJsonObject.h"
#include "FAny.h"

using namespace rapidjson;
using namespace std;

// create a map
struct MyHandlerMap {
	MyHandlerMap() {
		myCurrentKey = "_root";
		myCurrentObject = nullptr;
	}
	bool Null() { cout << "Null()" << endl; return true; } // do we care about null objects?
	bool Bool(bool b) {
		myCurrentObject->AddItem(myCurrentKey, FAny(b)); return true;
	}
	bool Int(int i) {
		myCurrentObject->AddItem(myCurrentKey, FAny(i)); return true;
	}
	bool Uint(unsigned u) {
		myCurrentObject->AddItem(myCurrentKey, FAny(u)); return true;
	}
	bool Int64(int64_t i) {
		myCurrentObject->AddItem(myCurrentKey, FAny(i)); return true;
	}
	bool Uint64(uint64_t u) {
		myCurrentObject->AddItem(myCurrentKey, FAny(u)); return true;
	}
	bool Double(double d) {
		myCurrentObject->AddItem(myCurrentKey, FAny(d)); return true;
	}
	bool RawNumber(const char* str, SizeType length, bool copy) {
		cout << "Number(" << str << ", " << length << ", " << boolalpha << copy << ")" << endl;
		return true;
	}
	bool String(const char* str, SizeType length, bool copy) {
		myCurrentObject->AddItem(myCurrentKey, FAny(str));
		return true;
	}
	bool StartObject()
	{
		if (!myCurrentObject)
		{
			myCurrentObject = new FJsonObject(myCurrentKey);
			myRoot = myCurrentObject;
		}
		else
			myCurrentObject = myCurrentObject->AddChild(myCurrentKey);

		return true;
	}
	bool Key(const char* str, SizeType length, bool copy) {
		myCurrentKey = str;
		return true;
	}
	bool EndObject(SizeType memberCount) { myCurrentObject = myCurrentObject->GetParent(); return true; }
	bool StartArray() { cout << "StartArray()" << endl; return true; }
	bool EndArray(SizeType elementCount) { cout << "EndArray(" << elementCount << ")" << endl; return true; }
	FJsonObject* GetRoot() { return myRoot; }

private:
	FJsonObject* myCurrentObject;
	FJsonObject* myRoot;
	std::string myCurrentKey;
};

FJsonObject* FJson::Parse(const char * aFile)
{
	string jsonfile;
	string line;
	ifstream myfile(aFile);
	if (myfile.is_open())
	{
		while (getline(myfile, line))
		{
			cout << line << '\n';
			jsonfile += line;
		}
		myfile.close();
		jsonfile += '\0';
	}

	MyHandlerMap handler;
	Reader reader;
	StringStream ss(jsonfile.c_str());
	reader.Parse(ss, handler);
	return handler.GetRoot();
}

FJson::FJson()
{
}

FJson::~FJson()
{
}
