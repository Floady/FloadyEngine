#include "FAny.h"

static unsigned int stringId;
std::map<unsigned int, string> FAny::ourStringDict;

int FAny::AddString(const string & aString)
{
	for (auto it = ourStringDict.begin(); it != ourStringDict.end(); ++it)
	{
		if ((*it).second == aString)
			return (*it).first;
	}

	ourStringDict[++stringId] = aString;
	return stringId;
}
