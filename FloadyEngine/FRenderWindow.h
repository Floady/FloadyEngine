#pragma once
#include <windows.h>
#include "FDelegate.h"

class FD3d12Input;
class FCamera;
class FTimer;
class FD3d12Renderer;

class FRenderWindow
{
public:
	FRenderWindow(FDelegate2<void(UINT, WPARAM, LPARAM)>& anInputCallback);
	~FRenderWindow();

	bool Initialize();
	void Shutdown();
	bool IsFocussed() { return myIsFocussed; }
	int GetWindowWidth() { return myWindowWidth; }
	int GetWindowHeight() { return myWindowHeight; }

	int GetScreenWidth() { return myScreenWidth; }
	int GetScreenHeight() { return myScreenHeight; }

	int GetPosX() { return posX; }
	int GetPosY() { return posY; }

	void SetCursorVisible(bool aVisible);

	LRESULT CALLBACK MessageHandler(HWND, UINT, WPARAM, LPARAM);

	bool CheckForQuit();
	HWND GetHWND() { return m_hwnd; }

private:
	void InitializeWindows(int&, int&);
	void ShutdownWindows();

private:
	LPCWSTR m_applicationName;
	HINSTANCE m_hinstance;
	HWND m_hwnd;

	bool myIsFocussed;
	int posX, posY;
	int myWindowHeight, myWindowWidth;
	FDelegate2<void(UINT, WPARAM, LPARAM)> myInputCallback;
	int myScreenHeight;
	int myScreenWidth;
	bool myShouldShowCursor;
};

