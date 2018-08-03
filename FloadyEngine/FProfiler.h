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

	scopedMarker(const char* aName, bool anIsGPU = false, unsigned __int64 aColor = 0xFF00FF00)
	{
		myName = aName;
		myTimer.Restart();
		myIsGPU = anIsGPU;
		myColor = aColor;
	}

	void Start();

	~scopedMarker();

	bool myIsGPU;
	FTimer myTimer;
	const char* myName;
	unsigned __int64 myColor;
};

#define FPROFILE_FUNCTION_CUSTOM(aName, aColor) scopedMarker __someMarker = scopedMarker(aName, false, aColor); __someMarker.Start();
#define FPROFILE_FUNCTION(aName) FPROFILE_FUNCTION_CUSTOM(aName, 0xFF00FF00)
#define FPROFILE_FUNCTION_GPU_CUSTOM(aName, aColor) scopedMarker __someMarker = scopedMarker(aName, true, aColor); __someMarker.Start();
#define FPROFILE_FUNCTION_GPU(aName) FPROFILE_FUNCTION_GPU_CUSTOM(aName, 0xFF00FFFF)
//#define FPROFILE_FUNCTION(aName) void();


