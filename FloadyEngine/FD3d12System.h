#pragma once
#include <windows.h>

class FD3d12Input;
class FD3d12Graphics;
class FCamera;
class FTimer;

class FD3d12System
{
public:
	FD3d12System();
	~FD3d12System();

	bool Initialize();
	void Shutdown();
	void Run();

	LRESULT CALLBACK MessageHandler(HWND, UINT, WPARAM, LPARAM);

private:
	bool Frame();
	void InitializeWindows(int&, int&);
	void ShutdownWindows();

private:
	LPCWSTR m_applicationName;
	HINSTANCE m_hinstance;
	HWND m_hwnd;

	FD3d12Input* m_Input;
	FD3d12Graphics* m_Graphics;
	FCamera* myCamera; 
	bool myIsFocussed;
	int posX, posY;
	int screenHeight, screenWidth;
	FTimer* myFrameTimer;
	double myFrameTime;
};

