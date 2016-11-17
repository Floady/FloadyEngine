#pragma once

class FD3d12Input
{
public:
	FD3d12Input();
	FD3d12Input(const FD3d12Input&);
	~FD3d12Input();

	void Initialize();

	void KeyDown(unsigned int);
	void KeyUp(unsigned int);
	void MouseMove(unsigned int x, unsigned int y);
	bool IsKeyDown(unsigned int);
	unsigned int GetMouseX() { return myMouseX; }
	unsigned int GetMouseY() { return myMouseY; }
	void SetMousePos(unsigned int x, unsigned int y);

private:
	bool m_keys[256];
	unsigned int myMouseX;
	unsigned int myMouseY;
};


