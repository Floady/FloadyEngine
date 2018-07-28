#pragma once
#include "FDelegate.h"
#include <vector>
#include "windows.h"


// Usage:
// WaitForAllJobs -> ResetQueue -> Queue up some jobs -> (Unpause if needed), WaitForAllJobs, etc.

class FJobSystem
{
public:
	struct FJob
	{
		FJob(const FJob& aJob)
		{
			myFunc = aJob.myFunc;
			myJobIDependOn = nullptr;
			InterlockedExchange(&myFinished, 0);
			InterlockedExchange(&myStarted, 1);
			InterlockedExchange(&myQueued, 0);
			InterlockedExchange(&myDependencyCounter, 0);
		}

		FJob()
		{
			myJobIDependOn = nullptr;
			InterlockedExchange(&myStarted, 1);
			InterlockedExchange(&myFinished, 0);
			InterlockedExchange(&myQueued, 0);
			InterlockedExchange(&myDependencyCounter, 0);
		}

		FDelegate2<void()> myFunc; // this is never read+write concurrently (we increment the job idx after assignment)
		volatile LONG myFinished;
		volatile LONG myStarted;;
		volatile LONG myQueued;;
		volatile LONG myDependencyCounter;
		FJob* myJobIDependOn;
	};

	static thread_local int ourThreadIdx;

	
	bool Initialize(int aNrWorkerThreadsShort, int aNrWorkerThreadsLong);
	static FJobSystem* GetInstance();
	int GetNextJob(bool anIsLong);
	bool SetJobIdFree(int anIdx, bool anIsLong);
	FJob* GetJobFromQueue(int anIdx, bool anIsLong);
	FJob* QueueJob(const FDelegate2<void()>& aDelegate, bool anIsLong = false, FJobSystem::FJob* aJobToDependOn = nullptr);
	void ResetQueue();
	void WaitForAllJobs();
	void UnPause() { myIsPaused = false; }
	void Pause() { myIsPaused = true; }
	bool IsPaused() { return myIsPaused; }
	int GetNrWorkerThreadsShort() { return myNrWorkerThreadsShort; }
	int GetNrWorkerThreadsLong() { return myNrWorkerThreadsLong; }
	void Test();
	void CountUp();
	void CountUpWithSleep();
private:
	FJobSystem(int aNrWorkerThreadsShort, int aNrWorkerThreadsLong);
	~FJobSystem();
	
	std::vector<FJob> myQueueShort;
	std::vector<FJob> myQueueLong;
	int myFreeShortTasks[4096];
	int myShortTasksQueue[4096];

	int myFreeLongTasks[4096];
	int myLongTasksQueue[4096];
	
	volatile LONG myNextJobIndexShort;
	volatile LONG myNextJobIndexLong;
	volatile LONG myFreeIndexShort;
	volatile LONG myFreeIndexLong;

	volatile LONG myLastQueuedJobShort;
	volatile LONG myLastQueuedJobLong;
	
	bool myIsPaused;
	int myNrWorkerThreadsShort;
	int myNrWorkerThreadsLong;

	// Test
	LONG myCounter;
	int myExpectedTotal;
};

