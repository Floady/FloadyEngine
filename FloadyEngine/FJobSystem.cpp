#include "FJobSystem.h"
#include "FUtilities.h"

static FJobSystem* ourInstance = nullptr;
static int ourWorkerThreadCounter = 0;
thread_local int FJobSystem::ourThreadIdx = 0;

#define JOB_DEBUG 0

#if JOB_DEBUG
#pragma optimize("", off)
#endif 

#if JOB_DEBUG
#define JOB_DEBUG_LOG(x, ...) FLOG("Thread: %d: "##x, FJobSystem::ourThreadIdx, __VA_ARGS__);
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
				FJob* job = jobSystem->GetJobFromQueue(nextJob, isLong);
				if(!job->myStarted)
				{
					if (!InterlockedExchange(&job->myStarted, 1))
					{
						JOB_DEBUG_LOG("Starting %s job [%i]", isLong ? "long" : "short", nextJob);
						job->myFunc();
						InterlockedExchange(&job->myFinished, 1);

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
	
	// try get the next job if its not locked by a dependency
	LONG newJobIdx = anIsLong ? myLongTasksQueue[myNextJobIndexLong] : myShortTasksQueue[myNextJobIndexShort];
	bool isDependencyOk = false;
	bool jobWantsToRun = !queue[curJobIdx].myStarted && queue[curJobIdx].myQueued;
	bool jobWasDependant = false;
	
	FJob* depJob = queue[curJobIdx].myJobIDependOn;
	jobWasDependant = depJob != nullptr;

	if(jobWantsToRun)
	{
		isDependencyOk = depJob ? !(depJob->myQueued && !depJob->myFinished) : true;
		jobWantsToRun &= isDependencyOk;

		if(jobWantsToRun)
		{
			if (InterlockedCompareExchange(anIsLong ? &myNextJobIndexLong : &myNextJobIndexShort, newJobIdx, curJobIdx) == curJobIdx)
			{
				JOB_DEBUG_LOG("Get %s Job: %d %p (next job: %d)", anIsLong ? "long" : "short", curJobIdx, &queue[curJobIdx], newJobIdx);
				return curJobIdx;
			}
		}
	}

	// only look ahead if this was a dependant job, otherwise it was just a race fail (and the worker will regrab latest, that is faster than traversing and relinking)
	if(jobWasDependant && !isDependencyOk)
	{
		// find next available job
		LONG nextAvailableJob = anIsLong ? myLongTasksQueue[curJobIdx] : myShortTasksQueue[curJobIdx];
		LONG lastJob = curJobIdx;
		bool isJobAvailable = false;
		while (!isJobAvailable)
		{
			// check if this job is queued or not (if not, we reached the end of the queue) - no available jobs
			if (!queue[nextAvailableJob].myQueued)
				break;

			// check next job, if it is not started yet, and has no dependencies - take it
			FJob* depJob = queue[nextAvailableJob].myJobIDependOn;

			bool jobHasADependantJobRunning = depJob ? depJob->myQueued && !depJob->myFinished : false;
			isJobAvailable = !jobHasADependantJobRunning && !queue[nextAvailableJob].myStarted;
			if (isJobAvailable)
			{
				// we return here without moving the nextjob index so that the locked job will still be picked up ASAP
				anIsLong ? myLongTasksQueue[lastJob] : myShortTasksQueue[lastJob] = anIsLong ? myLongTasksQueue[nextAvailableJob] : myShortTasksQueue[nextAvailableJob]; // todo, thread safety
				JOB_DEBUG_LOG("Relink %s Job: %d to %d", anIsLong ? "long" : "short", lastJob, anIsLong ? myLongTasksQueue[nextAvailableJob] : myShortTasksQueue[nextAvailableJob]);
			
				JOB_DEBUG_LOG("Get next available %s Job: %d %p", anIsLong ? "long" : "short", nextAvailableJob, &queue[nextAvailableJob]);
				return nextAvailableJob;
			}

			// try next
			lastJob = nextAvailableJob;
			nextAvailableJob = anIsLong ? myLongTasksQueue[nextAvailableJob] : myShortTasksQueue[nextAvailableJob];
		}
	}

	if(jobWantsToRun)
		JOB_DEBUG_LOG("Failed to get job: %d", curJobIdx);

	return -1;
}

bool FJobSystem::SetJobIdFree(int anIdx, bool anIsLong)
{
	std::vector<FJob>& queue = anIsLong ? myQueueLong : myQueueShort;

	LONG curFreeIdx = anIsLong ? myFreeIndexLong : myFreeIndexShort;
	LONG nextFreeIdx = anIdx;
	volatile LONG* newFreeIdx = anIsLong ? &myFreeIndexLong : &myFreeIndexShort;

	// check if we were dependant and clear it before we release it to the queue
	if (queue[anIdx].myJobIDependOn)
	{
		InterlockedDecrement(&queue[anIdx].myJobIDependOn->myDependencyCounter);
	}

	// don't free jobs that others depend on to avoid them being recycled
	if (true || queue[anIdx].myDependencyCounter > 0) // hack - dont recycle
	{
		queue[anIdx].myJobIDependOn = nullptr;
		JOB_DEBUG_LOG("%s job not set to free as it has dependencies: %d", anIsLong ? "long" : "short", anIdx);
		return true;
	}

	if (InterlockedCompareExchange(newFreeIdx, nextFreeIdx, curFreeIdx) == curFreeIdx)
	{
		InterlockedExchange(&queue[anIdx].myQueued, 0);
		queue[anIdx].myJobIDependOn = nullptr;
		JOB_DEBUG_LOG("Set %s job to free: %d (linking to %d)", anIsLong ? "long" : "short", nextFreeIdx, curFreeIdx);
		anIsLong ? myFreeLongTasks[curFreeIdx] : myFreeShortTasks[curFreeIdx] = nextFreeIdx;
		return true;
	}

	JOB_DEBUG_LOG("Failed to set %s job to free: %d", anIsLong ? "long" : "short", nextFreeIdx);
	return false;
}

FJob * FJobSystem::GetJobFromQueue(int anIdx, bool anIsLong)
{
	return anIsLong ? &myQueueLong[anIdx] : &myQueueShort[anIdx];
}

FJob* FJobSystem::QueueJob(const FDelegate2<void()>& aDelegate, bool anIsLong, FJob* aJobToDependOn)
{
	LONG curFreeIdx = anIsLong ? myFreeIndexLong : myFreeIndexShort;
	LONG curJobIdx = anIsLong ? myNextJobIndexLong : myNextJobIndexShort;
	
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
				if(aJobToDependOn)
					InterlockedIncrement(&aJobToDependOn->myDependencyCounter); // let the job know we depend on it
		
				JOB_DEBUG_LOG("Queued %s Job: %d (next free = %d)", anIsLong ? "long" : "short", curFreeIdx, nextFreeIdx);
	
				return &queue[curFreeIdx];
			}
		}
	}

	JOB_DEBUG_LOG("Failed to queue Job: %d", curFreeIdx);
	return nullptr;
}

void FJobSystem::ResetQueue(bool aResetLong)
{
	JOB_DEBUG_LOG("Reset Queue: Short%s", aResetLong ? " and Long" : "");

	if (aResetLong)
	{
		myFreeIndexLong = 0;
		myNextJobIndexLong = 0;
		myLastQueuedJobLong = 0;

		for (size_t i = 0; i < 4096-1; i++)
		{
			myFreeLongTasks[i] = i + 1;
			myLongTasksQueue[i] = i + 1;
			myQueueLong[i].Reset();
		}

		myQueueLong[4095].Reset();
		myFreeLongTasks[4095] = 0;
		myLongTasksQueue[4095] = 0;
	}

	bool wasPaused = myIsPaused;
	Pause();
	WaitForAllJobs();

	myFreeIndexShort = 0;
	myNextJobIndexShort = 0;
	myLastQueuedJobShort = 0;
	
	for (size_t i = 0; i < 4096-1; i++)
	{
		myFreeShortTasks[i] = i + 1;
		myShortTasksQueue[i] = i + 1;
		myQueueShort[i].Reset();
	}

	myQueueShort[4095].Reset();
	myFreeShortTasks[4095] = 0;
	myShortTasksQueue[4095] = 0;

	if(!wasPaused)
		UnPause();
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
			if (myQueueShort[i].myFunc && myQueueShort[i].myQueued && !(myQueueShort[i].myStarted && myQueueShort[i].myFinished))
			{
				isDone = false;
				break;
			}
		}
	}

	JOB_DEBUG_LOG("Waiting For short tasks done");

	return;
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
		if (i < 50)
		{
			newJob = QueueJob((FDelegate2<void()>(this, &FJobSystem::CountUpWithSleep)), false, job);
		}
		else
		{
			newJob = QueueJob((FDelegate2<void()>(this, &FJobSystem::CountUpWithSleep)));
		}

		job = newJob;
	}
	UnPause();
	WaitForAllJobs();
	ResetQueue();

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

	ResetQueue(true);

	// use indexes to track completed and queued + free jobs
	// grow pool with mutex if needed
	// how do we wait for jobs? check a finished flag, dependencies could use it too
	// careful to not recycle jobs that are being depended on, e.g checked for finish (waited)

	// probably want numCPU -1, and set affinities so we operate on all cores at full power (high prio), no moving threads on cores
	//myNrWorkerThreads = numCPU - 1;
	myNrWorkerThreadsShort = aNrWorkerThreadsShort;
	for (int i = 0; i < myNrWorkerThreadsShort; i++)
	{
		HANDLE h = CreateThread(0, 0, &FWorkerThread, this, 0, NULL);
		myShortTaskThreads.push_back(h);
	}

	myNrWorkerThreadsLong = aNrWorkerThreadsLong;
	for (int i = 0; i < myNrWorkerThreadsLong; i++)
	{
		HANDLE h = CreateThread(0, 0, &FWorkerThread, this, 0, NULL);
		myLongTaskThreads.push_back(h);
	}

	DWORD_PTR mask = 1;
	SetThreadAffinityMask(myShortTaskThreads[0], mask);
	SetThreadPriority(myShortTaskThreads[0], THREAD_PRIORITY_ABOVE_NORMAL);
	mask = 2;
	SetThreadAffinityMask(myShortTaskThreads[1], mask);
	SetThreadPriority(myShortTaskThreads[1], THREAD_PRIORITY_ABOVE_NORMAL);
	//mask = 4;
	//SetThreadAffinityMask(myShortTaskThreads[2], mask);
	//SetThreadPriority(myShortTaskThreads[2], THREAD_PRIORITY_ABOVE_NORMAL);

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
		ourInstance = new FJobSystem(2, 1);
		ourInstance->Test();
	}

	return ourInstance;
}


#if JOB_DEBUG
#pragma optimize("", on)
#endif 
