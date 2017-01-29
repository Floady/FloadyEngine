#pragma once
#include <windows.h>

class FD3d12Input
{
public:
	FD3d12Input();
	FD3d12Input(const FD3d12Input&);
	~FD3d12Input();

	void Initialize();
	void Update();
	void KeyDown(unsigned int);
	void KeyUp(unsigned int);
	void MouseMove(unsigned int x, unsigned int y);
	bool IsKeyDown(unsigned int);
	void MessageHandler(UINT umsg, WPARAM wparam, LPARAM lparam);
	int GetDeltaMouseX() { return myMouseFrameDeltaX; }
	int GetDeltaMouseY() { return myMouseFrameDeltaY; }
	void SetMousePos(unsigned int x, unsigned int y);

private:
	bool m_keys[256];
	unsigned int myMouseX;
	unsigned int myMouseY;
	int myMouseFrameDeltaX;
	int myMouseFrameDeltaY;
	bool myMouseWasJustSet;
	int myFocusFrame;
};


