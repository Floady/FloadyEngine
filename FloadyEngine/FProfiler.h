#pragma once
#include "FTimer.h"
#include <map>
#include <vector>

class FDynamicText;

class FProfiler
{
public:
	static unsigned int ourHistoryBufferCount;
	static FProfiler* GetInstance();
	~FProfiler();
	void AddTiming(const char* aName, double aTime);
	void StartFrame();
	void SetPause(bool aPaused) { myIsPaused = aPaused; }
	void Render();

protected:
	static FProfiler* ourInstance;

private:
	struct FrameTimer
	{
		FrameTimer() { Reset(); }
		void Reset() { myTime = 0; myOccurences = 0; }
		double myTime;
		unsigned int myOccurences;
	};

	FProfiler();
	std::map<const char*, std::vector<FrameTimer>> myTimings;
	std::vector<FDynamicText*> myLabels;
	unsigned int myCurrentFrame;
	bool myIsPaused;
};

struct scopedMarker
{
	scopedMarker() {};

	scopedMarker(const char* aName)
	{
		myName = aName;
		myTimer.Restart();
	}

	void Start();

	~scopedMarker();

	FTimer myTimer;
	const char* myName;
};

#define FPROFILE_FUNCTION(aName) scopedMarker __someMarker = scopedMarker(aName); __someMarker.Start();
//#define FPROFILE_FUNCTION(aName) void();


