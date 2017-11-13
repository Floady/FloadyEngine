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
	static FProfiler* GetInstanceNoCreate();
	~FProfiler();
	void AddTiming(const char* aName, double aTime);
	void StartFrame();
	void SetPause(bool aPaused) { myIsPaused = aPaused; }
	void Render();
	void SetVisible(bool aVisible);

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

	struct TimerInfo
	{
		std::vector<FrameTimer> myFrameTimings;
		FrameTimer myTotalTime;
	};

	FProfiler();
	std::map<const char*, TimerInfo> myTimings;
	std::vector<FDynamicText*> myLabels;
	unsigned int myCurrentFrame;
	bool myIsPaused;
	bool myIsVisible;
};

struct scopedMarker
{
	scopedMarker() {};

	scopedMarker(const char* aName)
	{
		myName = aName;
		myTimer.Restart();
		myIsGPU = false;
	}

	void Start(bool aIsGPU = false);

	~scopedMarker();

	bool myIsGPU;
	FTimer myTimer;
	const char* myName;
};

#define FPROFILE_FUNCTION(aName) scopedMarker __someMarker = scopedMarker(aName); __someMarker.Start();
#define FPROFILE_FUNCTION_GPU(aName) scopedMarker __someMarker = scopedMarker(aName); __someMarker.Start(true);
//#define FPROFILE_FUNCTION(aName) void();


