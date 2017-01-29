#include "FD3d12Input.h"
#include <windows.h>

FD3d12Input::FD3d12Input()
{
	myMouseX = 0;
	myMouseY = 0;
	myMouseFrameDeltaX = 0;
	myMouseFrameDeltaY = 0;
	Initialize();
}


FD3d12Input::~FD3d12Input()
{
}


FD3d12Input::FD3d12Input(const FD3d12Input& other)
{
	Initialize();
}


void FD3d12Input::Initialize()
{
	int i;
	myFocusFrame = 1;

	// Initialize all the keys to being released and not pressed.
	for (i = 0; i<256; i++)
	{
		m_keys[i] = false;
	}

	return;
}

void FD3d12Input::Update()
{
	if (myFocusFrame > 0)
	{
		myFocusFrame--;
		return;
	}

	POINT p;
	GetCursorPos(&p);
	myMouseFrameDeltaX = p.x - (1920 / 2);
	myMouseFrameDeltaY = p.y - (1080 / 2);
	myMouseX = p.x;
	myMouseY = p.y;
}

void FD3d12Input::KeyDown(unsigned int input)
{
	// If a key is pressed then save that state in the key array.
	m_keys[input] = true;
	return;
}


void FD3d12Input::KeyUp(unsigned int input)
{
	// If a key is released then clear that state in the key array.
	m_keys[input] = false;
	return;
}

void FD3d12Input::MouseMove(unsigned int x, unsigned int y)
{
	//SetMousePos(1920 / 2, 1080 / 2);
}


bool FD3d12Input::IsKeyDown(unsigned int key)
{
	// Return what state the key is in (pressed/not pressed).
	return m_keys[key];
}

void FD3d12Input::SetMousePos(unsigned int x, unsigned int y)
{
	myMouseWasJustSet = true;
	SetCursorPos(x, y);
}


void FD3d12Input::MessageHandler(UINT umsg, WPARAM wparam, LPARAM lparam)
{
	switch (umsg)
	{
		case WM_SETFOCUS:
		{
			myFocusFrame = 1;
		}
		break;
		case WM_KEYDOWN:
		{
			KeyDown((unsigned int)wparam);
		}
		break;
		case WM_MOUSEMOVE:
		{
			if(myMouseWasJustSet)
				myMouseWasJustSet = false;
			else
				MouseMove((unsigned int)lparam & 0x0000FFFF, (unsigned int)lparam >> 16);
		}
		break;
		case WM_KEYUP:
		{
			KeyUp((unsigned int)wparam);
		}
		break;
	}
}
