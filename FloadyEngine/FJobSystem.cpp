#include "FJobSystem.h"
#include "FUtilities.h"

static FJobSystem* ourInstance = nullptr;
static int ourWorkerThreadCounter = 0;
thread_local int FJobSystem::ourThreadIdx = 0;

#define JOB_DEBUG 1

#if JOB_DEBUG
#pragma optimize("", off)
#endif 

#if JOB_DEBUG
#define JOB_DEBUG_LOG(x, ...) FLOG(x, __VA_ARGS__);
#else
#define JOB_DEBUG_LOG(x, ...) (void)(x);
#endif

DWORD WINAPI FWorkerThread(LPVOID aJobSystem)
{
	JOB_DEBUG_LOG("FWorkerThread kicked off");

	FJobSystem* jobSystem = (FJobSystem*)aJobSystem;
	FJobSystem::ourThreadIdx = ++ourWorkerThreadCounter;
	bool isLong = FJobSystem::ourThreadIdx > jobSystem->GetNrWorkerThreadsShort();

	// poll for jobs and execute
	while (true)
	{
		if (!jobSystem->IsPaused())
		{
			int nextJob = jobSystem->GetNextJob(isLong);
			if (nextJob != -1)
			{
				FJobSystem::FJob* job = jobSystem->GetJobFromQueue(nextJob, isLong);
				if(!job->myStarted)
				{
					if (!InterlockedExchange(&job->myStarted, 1))
					{
						JOB_DEBUG_LOG("Starting %s job [%i]", isLong ? "long" : "short", nextJob);
						job->myFunc();
						job->myJobIDependOn = nullptr; // clear dependency before re-use
						InterlockedExchange(&job->myFinished, 1);
						InterlockedExchange(&job->myQueued, 0);
						while (!jobSystem->SetJobIdFree(nextJob, isLong))
						{
							JOB_DEBUG_LOG("waiting for job[%i] to be set to free - retry", nextJob);
						}

						// testing queueing from workerthreads
						// jobSystem->QueueJob(job->myFunc);
	
						JOB_DEBUG_LOG("%s job done: %i[%p] finished: %d(%p)", isLong ? "long" : "short", nextJob, job, job->myFinished, &job->myFinished);
					}
				}
			}
			else
			{
				Sleep(0);
			}
		}
		else
		{
			Sleep(0);
		}
	}

	return 0;
};

int FJobSystem::GetNextJob(bool anIsLong)
{
	LONG curJobIdx = anIsLong ? myNextJobIndexLong : myNextJobIndexShort;
	LONG curFreeIdx = anIsLong ? myFreeIndexLong : myFreeIndexShort;
	std::vector<FJob>& queue = anIsLong ? myQueueLong : myQueueShort;

	//if (curJobIdx < curFreeIdx)
	{
		LONG newJobIdx = anIsLong ? myLongTasksQueue[myNextJobIndexShort] : myShortTasksQueue[myNextJobIndexShort];
		bool isDependencyOk = queue[curJobIdx].myJobIDependOn ? !(queue[curJobIdx].myJobIDependOn->myQueued && !queue[curJobIdx].myJobIDependOn->myFinished) : true;
		if(!myQueueShort[curJobIdx].myStarted && isDependencyOk)
		{
			if (InterlockedCompareExchange(anIsLong ? &myNextJobIndexLong : &myNextJobIndexShort, newJobIdx, curJobIdx) == curJobIdx)
			{
				JOB_DEBUG_LOG("Get %s Job: %d %p", anIsLong ? "long" : "short", curJobIdx, anIsLong ? &myQueueLong[curJobIdx] : &myQueueShort[curJobIdx]);
				return curJobIdx;
			}
			else
			{
				JOB_DEBUG_LOG("Failed to get job: %d", curJobIdx);
			}
		}
	}


	return -1;
}

bool FJobSystem::SetJobIdFree(int anIdx, bool anIsLong)
{
	std::vector<FJob>& queue = anIsLong ? myQueueLong : myQueueShort;

	LONG curFreeIdx = anIsLong ? myFreeIndexLong : myFreeIndexShort;
	LONG nextFreeIdx = anIdx;
	volatile LONG* newFreeIdx = anIsLong ? &myFreeIndexLong : &myFreeIndexShort;

	if (InterlockedCompareExchange(newFreeIdx, nextFreeIdx, curFreeIdx) == curFreeIdx)
	{
		JOB_DEBUG_LOG("Set %s job to free: %d", anIsLong ? "long" : "short", nextFreeIdx);
		anIsLong ? myFreeLongTasks[nextFreeIdx] : myFreeShortTasks[nextFreeIdx] = curFreeIdx;
		return true;
	}

	return false;
}

FJobSystem::FJob * FJobSystem::GetJobFromQueue(int anIdx, bool anIsLong)
{
	return anIsLong ? &myQueueLong[anIdx] : &myQueueShort[anIdx];
}

FJobSystem::FJob* FJobSystem::QueueJob(const FDelegate2<void()>& aDelegate, bool anIsLong, FJobSystem::FJob* aJobToDependOn)
{
	LONG curFreeIdx = anIsLong ? myFreeIndexLong : myFreeIndexShort;
	
	// Check if queue is full
	std::vector<FJob>& queue = anIsLong ? myQueueLong : myQueueShort;
	if (curFreeIdx < queue.size())
	{
		LONG nextFreeIdx = anIsLong ? myFreeLongTasks[curFreeIdx] : myFreeShortTasks[curFreeIdx];

		if (InterlockedCompareExchange(anIsLong ? &myFreeIndexLong : &myFreeIndexShort, nextFreeIdx, curFreeIdx) == curFreeIdx)
		{
			LONG curLastQueuedJob = anIsLong ? myLastQueuedJobLong : myLastQueuedJobShort;
			if (InterlockedCompareExchange(anIsLong ? &myLastQueuedJobLong : &myLastQueuedJobShort, curFreeIdx, curLastQueuedJob) == curLastQueuedJob)
			{
				queue[curFreeIdx].myFunc = aDelegate;
				queue[curFreeIdx].myJobIDependOn = aJobToDependOn;
				InterlockedExchange(&queue[curFreeIdx].myStarted, 0);
				InterlockedExchange(&queue[curFreeIdx].myFinished, 0);
				InterlockedExchange(&queue[curFreeIdx].myQueued, 1);
				anIsLong ? myLongTasksQueue[curLastQueuedJob] : myShortTasksQueue[curLastQueuedJob] = curFreeIdx;

				JOB_DEBUG_LOG("Queued %s Job: %d", anIsLong ? "long" : "short", curFreeIdx);
				return &queue[curFreeIdx];
			}
		}
	}

	JOB_DEBUG_LOG("Failed to queue Job: %d", curFreeIdx);
	return nullptr;
}

void FJobSystem::ResetQueue()
{
	myFreeIndexShort = 0;
	myFreeIndexLong = 0;
	myNextJobIndexShort = 0;
	myNextJobIndexLong = 0;
	myLastQueuedJobShort = 0;
	myLastQueuedJobLong = 0;

	for (size_t i = 0; i < 4096; i++)
	{
		myFreeShortTasks[i] = i + 1;
		myShortTasksQueue[i] = 0;
		myFreeLongTasks[i] = i + 1;
		myLongTasksQueue[i] = 0;
	}

}

void FJobSystem::WaitForAllJobs() // waits for short queue, never for long
{
	JOB_DEBUG_LOG("Waiting For short tasks");
	bool isDone = false;
	while (!isDone)
	{
		isDone = true;
		for (size_t i = 0; i < 4096; i++)
		{
			if (myQueueShort[i].myFunc && !(myQueueShort[i].myStarted && myQueueShort[i].myFinished))
			{
				isDone = false;
				break;
			}
		}
	}

	return;

	//bool isDone = false;
	//while (!isDone)
	//{
	//	isDone = true;
	//	LONG freeIdx = myFreeIndexShort; // atomic assignment to cache value
	//	for (int i = 0; i < freeIdx; i++)
	//	{
	//		LONG isFinished = myQueueShort[i].myFinished; // atomic assignment
	//		isDone &= isFinished == 1;
	//	}
	//}
}

void FJobSystem::Test()
{
	//*
	myExpectedTotal = 1000;
	Pause();
	for (size_t i = 0; i < myExpectedTotal; i++)
	{
		QueueJob((FDelegate2<void()>(this, &FJobSystem::CountUp)));
	}
	UnPause();
	WaitForAllJobs();

	FLOG("Test 1: %i == %i", myExpectedTotal, myCounter);
	//*/

	myExpectedTotal = 3200;
	myCounter = 0;
	Pause();
	ResetQueue();
	FJob* job = nullptr;
	FJob* newJob = nullptr;
	for (size_t i = 0; i < myExpectedTotal; i++)
	{
		/*if (i < 50)
		{
			newJob = QueueJob((FDelegate2<void()>(this, &FJobSystem::CountUpWithSleep)), false, job);
		}
		else*/
		{
			newJob = QueueJob((FDelegate2<void()>(this, &FJobSystem::CountUpWithSleep)));
		}

		job = newJob;
	}
	UnPause();
	WaitForAllJobs();

	FLOG("Test 2: %i == %i", myExpectedTotal, myCounter);
}

void FJobSystem::CountUp()
{
	InterlockedIncrement(&myCounter);
}

void FJobSystem::CountUpWithSleep()
{
	InterlockedIncrement(&myCounter);
	Sleep(5);
}

FJobSystem::FJobSystem(int aNrWorkerThreadsShort, int aNrWorkerThreadsLong)
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	int numCPU = sysinfo.dwNumberOfProcessors;

	//start paused for testing
	myIsPaused = true;

	// grow the pool to sensible size
	myQueueShort.resize(4096);
	myQueueLong.resize(4096);

	ResetQueue();

	// use indexes to track completed and queued + free jobs
	// grow pool with mutex if needed
	// how do we wait for jobs? check a finished flag, dependencies could use it too
	// careful to not recycle jobs that are being depended on, e.g checked for finish (waited)

	// probably want numCPU -1, and set affinities so we operate on all cores at full power (high prio), no moving threads on cores
	//myNrWorkerThreads = numCPU - 1;
	myNrWorkerThreadsShort = aNrWorkerThreadsShort;
	for (int i = 0; i < myNrWorkerThreadsShort; i++)
	{
		CreateThread(0, 0, &FWorkerThread, this, 0, NULL);
	}

	myNrWorkerThreadsLong = aNrWorkerThreadsLong;
	for (int i = 0; i < myNrWorkerThreadsLong; i++)
	{
		CreateThread(0, 0, &FWorkerThread, this, 0, NULL);
	}

	JOB_DEBUG_LOG("Created job system: %i short, %i long", myNrWorkerThreadsShort, myNrWorkerThreadsLong);
}

bool FJobSystem::Initialize(int aNrWorkerThreadsShort, int aNrWorkerThreadsLong)
{
	delete ourInstance;
	ourInstance = new FJobSystem(aNrWorkerThreadsShort, aNrWorkerThreadsLong);

	return true;
}

FJobSystem::~FJobSystem()
{
}

FJobSystem* FJobSystem::GetInstance()
{
	if (!ourInstance)
	{
		ourInstance = new FJobSystem(3, 2);
		ourInstance->Test();
	}

	return ourInstance;
}


#if JOB_DEBUG
#pragma optimize("", on)
#endif 
