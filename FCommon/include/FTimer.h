#pragma once
class FTimer
{
public:
	FTimer();
	void Restart();
	void Pause();
	void Unpause();
	double GetTimeMS();
	double GetTimeUS();
	~FTimer();
private:
	__int64 ctr1 = 0, ctr2 = 0, freq = 0;
	int acc = 0, i = 0;
	double myPausedTime;
	double myLastUnpauseTimeStamp;
	bool myIsPaused;
};

