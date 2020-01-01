#pragma once
#include "FTimer.h"
#include <map>
#include <vector>

struct scopedMarker
{
	scopedMarker() {};

	scopedMarker(const char* aName, bool anIsGPU = false, unsigned __int64 aColor = 0xFF00FF00)
	{
		myName = aName;
		myIsGPU = anIsGPU;
		myColor = aColor;
	}

	void Start();

	~scopedMarker();

	bool myIsGPU;
	static FTimer myTimer;
	double myStartTime;
	double myEndTime;
	const char* myName;
	unsigned __int64 myColor;
	//int myThreadIndex;
};

class FProfiler
{
public:
	struct FrameTimer
	{
		FrameTimer() { Reset(); }
		void Reset() { myTime = 0; myOccurences = 0; }
		double myTime;
		double myStartTime;
		double myEndTime;
		unsigned int myOccurences;
	};

	struct TimerInfo
	{
		std::vector<FrameTimer> myFrameTimings;
		FrameTimer myTotalTime;
	};

	struct MarkerInfo
	{
		const char* myName;
		double myStartTime;
	};

	static unsigned int ourHistoryBufferCount;
	static FProfiler* GetInstance();
	static FProfiler* GetInstanceNoCreate();
	~FProfiler();
	void AddTiming(const char* aName, double aTime);
	void ProcessMarker(const scopedMarker& aMarker);
	void AddTimedMarker(const char* aMarkerName);
	void StartFrame();
	void SetPause(bool aPaused);
	void SetVisible(bool aVisible) { myIsVisible = aVisible; }

	void RenderIMGUI();

	const std::map<const char*, FProfiler::TimerInfo>& GetTimings() { return myTimings; }
	unsigned int GetCurrentHistoryFrameIndex() { return (myCurrentFrame -1) % ourHistoryBufferCount; }

protected:
	static FProfiler* ourInstance;

private:
	FProfiler();
	std::map<const char*, TimerInfo> myTimings;
	std::vector<MarkerInfo> myMarkers;
	unsigned int myCurrentFrame;
	bool myIsPaused;
	bool myIsVisible;
};

#define FPROFILE_FUNCTION_CUSTOM(aName, aColor) scopedMarker __someMarker = scopedMarker(aName, false, aColor); __someMarker.Start();
#define FPROFILE_FUNCTION(aName) FPROFILE_FUNCTION_CUSTOM(aName, 0xFF00FF00)
#define FPROFILE_FUNCTION_GPU_CUSTOM(aName, aColor) scopedMarker __someMarker = scopedMarker(aName, true, aColor); __someMarker.Start();
#define FPROFILE_FUNCTION_GPU(aName) FPROFILE_FUNCTION_GPU_CUSTOM(aName, 0xFF00FFFF)
#define FPROFILE_MARKER(aName) if(FProfiler* profiler = FProfiler::GetInstanceNoCreate()) profiler->AddTimedMarker(aName);
//#define FPROFILE_FUNCTION(aName) void();


