#include "FD3d12Input.h"
#include <windows.h>

FD3d12Input::FD3d12Input()
{
	myMouseX = 0;
	myMouseY = 0;
}


FD3d12Input::~FD3d12Input()
{
}


FD3d12Input::FD3d12Input(const FD3d12Input& other)
{
}


void FD3d12Input::Initialize()
{
	int i;


	// Initialize all the keys to being released and not pressed.
	for (i = 0; i<256; i++)
	{
		m_keys[i] = false;
	}

	return;
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
	myMouseX = x;
	myMouseY = y;
}


bool FD3d12Input::IsKeyDown(unsigned int key)
{
	// Return what state the key is in (pressed/not pressed).
	return m_keys[key];
}

void FD3d12Input::SetMousePos(unsigned int x, unsigned int y)
{
	myMouseX = x;
	myMouseY = y;
}
