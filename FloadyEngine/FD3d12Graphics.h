#pragma once

#include <windows.h>
class FD3DClass;

class FCamera;

class FD3d12Graphics
{
public:
	FD3d12Graphics();
	~FD3d12Graphics(); public:
	FD3d12Graphics(const FD3d12Graphics&);

	bool Initialize(int, int, HWND, bool);
	void SetCamera(FCamera* aCam);
	void Shutdown();
	bool Frame();

	FD3DClass* GetD3DClass() { return m_Direct3D; }

private:
	bool Render();

private:
	FD3DClass* m_Direct3D;
	FCamera* myCamera;
};

