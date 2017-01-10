#include "FJobSystem.h"

static int ourWorkerThreadCounter = 0;
thread_local int FJobSystem::ourThreadIdx = 0;
DWORD WINAPI FWorkerThread(LPVOID aJobSystem)
{
	/*char buff[512];
	sprintf_s(buff, "FWorkerThread kicked off \n");
	OutputDebugStringA(buff);	*/

	FJobSystem* jobSystem = (FJobSystem*)aJobSystem;

	FJobSystem::ourThreadIdx = ourWorkerThreadCounter++;

	// poll for jobs and execute
	while (true)
	{
		if (!jobSystem->IsPaused())
		{
			if (FJobSystem::FJob* job = jobSystem->GetNextJob())
			{
				job->myFunc();
				InterlockedExchange(&job->myFinished, 1);

				// testing queueing from workerthreads
				// jobSystem->QueueJob(job->myFunc);
/*
				char buff[512];
				sprintf_s(buff, "Job done: %p finished: %d(%p) \n", job, job->myFinished, &job->myFinished);
				OutputDebugStringA(buff);*/
			}
			else
			{
				Sleep(0);
			}
		}
	}

	return 0;
};

static FJobSystem* ourInstance = nullptr;
FJobSystem* FJobSystem::GetInstance()
{
	if (!ourInstance)
		ourInstance = new FJobSystem();

	return ourInstance;
}

FJobSystem::FJob * FJobSystem::GetNextJob()
{
	LONG curJobIdx = myNextJobIndex;
	LONG curFreeIdx = myFreeIndex;
	if (curJobIdx < curFreeIdx)
	{
		LONG newJobIdx = curJobIdx + 1;
	
		if (InterlockedCompareExchange(&myNextJobIndex, newJobIdx, curJobIdx) == curJobIdx)
		{
			/*char buff[512];
			sprintf_s(buff, "Get Job: %d %p\n", curJobIdx, &myQueue[curJobIdx]);
			OutputDebugStringA(buff);*/

			return &myQueue[curJobIdx];
		}
		else
		{
			/*char buff[512];
			sprintf_s(buff, "Failed to get job: %d\n", curJobIdx);
			OutputDebugStringA(buff);*/
		}
	}


	return nullptr;
}

bool FJobSystem::QueueJob(const FDelegate& aDelegate)
{
	LONG curFreeIdx = myFreeIndex;
	
	// Check if queue is full
	if (curFreeIdx < myQueue.size())
	{
		LONG nextFreeIdx = curFreeIdx + 1;

		if (InterlockedCompareExchange(&myFreeIndex, nextFreeIdx, curFreeIdx) == curFreeIdx)
		{
			myQueue[curFreeIdx].myFunc = aDelegate;
			InterlockedExchange(&myQueue[curFreeIdx].myFinished, 0);
			return true;
		}
	}

	return false;
}

void FJobSystem::ResetQueue()
{
	myFreeIndex = 0;
	myNextJobIndex = 0;
}

void FJobSystem::WaitForAllJobs()
{
	OutputDebugStringA("Started Waiting\n");

	bool isDone = false;
	while (!isDone)
	{
		isDone = true;
		LONG freeIdx = myFreeIndex; // atomic assignment to cache value
		for (int i = 0; i < freeIdx; i++)
		{
			LONG isFinished = myQueue[i].myFinished; // atomic assignment
			isDone &= isFinished == 1;
		}
	}
}

FJobSystem::FJobSystem()
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	int numCPU = sysinfo.dwNumberOfProcessors;

	//start paused for testing
	myIsPaused = true;

	// grow the pool to sensible size
	myQueue.resize(1024 * 5);

	myNextJobIndex = 0;
	myFreeIndex = 0;

	// use indexes to track completed and queued + free jobs
	// grow pool with mutex if needed
	// how do we wait for jobs? check a finished flag, dependencies could use it too
	// careful to not recycle jobs that are being depended on, e.g checked for finish (waited)

	// probably want numCPU -1, and set affinities so we operate on all cores at full power (high prio), no moving threads on cores
	myNrWorkerThreads = numCPU - 1;
	for (int i = 0; i < myNrWorkerThreads; i++)
	{
		/*char buff[512];
		sprintf_s(buff, "WORKERTHREAD\n");
		OutputDebugStringA(buff);*/

		CreateThread(0, 0, &FWorkerThread, this, 0, NULL);
	}
}

FJobSystem::~FJobSystem()
{
}
