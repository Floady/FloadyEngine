#include <stdio.h>
#include <tchar.h>
#include "FRenderWindow.h"
#include "FGame.h"
#include "FTimer.h"

int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow)
{
	FGame* myGame = new FGame();	

	myGame->Init();

	FTimer frameTimer;
	double frameTime = 0.0;

	while (myGame->Update(frameTime))
	{
		frameTime = frameTimer.GetTimeUS();
		frameTimer.Restart();
		
		if (frameTime > 0.33) // hack: debug breakpoints mess up the timer
			frameTime = 0.033;
	}
	
	return 0;
}

