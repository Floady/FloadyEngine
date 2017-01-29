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
	double frameTime = 30.0;

	while (myGame->Update(frameTime))
	{
		myGame->Render();
		frameTime = frameTimer.GetTimeMS();
		frameTimer.Restart();
	}
	
	return 0;
}

