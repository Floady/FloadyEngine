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
			InterlockedExchange(&myFinished, 0);
		}

		FJob()
		{
			InterlockedExchange(&myFinished, 0);
		}

		FDelegate myFunc; // this is never read+write concurrently (we increment the job idx after assignment)
		volatile LONG myFinished;
	};

	static thread_local int ourThreadIdx;

	FJobSystem();
	~FJobSystem();
	static FJobSystem* GetInstance();
	FJob* GetNextJob();
	bool QueueJob(const FDelegate& aDelegate);
	void ResetQueue();
	void WaitForAllJobs();
	void UnPause() { myIsPaused = false; }
	void Pause() { myIsPaused = true; }
	bool IsPaused() { return myIsPaused; }
	int GetNrWorkerThreads() { return myNrWorkerThreads; }
private:
	std::vector<FJob> myQueue;

	volatile LONG myNextJobIndex;
	volatile LONG myFreeIndex;
	bool myIsPaused;
	int myNrWorkerThreads;
};

