#include "FD3d12Graphics.h"

#include "FD3DClass.h"

FD3d12Graphics::FD3d12Graphics()
{
	m_Direct3D = 0;
}


FD3d12Graphics::~FD3d12Graphics()
{
}


FD3d12Graphics::FD3d12Graphics(const FD3d12Graphics& other)
{
}


static const bool VSYNC_ENABLED = false;

bool FD3d12Graphics::Initialize(int screenHeight, int screenWidth, HWND hwnd, bool fullScreen)
{
	bool result;


	// Create the Direct3D object.
	m_Direct3D = new FD3DClass;
	if (!m_Direct3D)
	{
		return false;
	}

	// Initialize the Direct3D object.
	result = m_Direct3D->Initialize(screenHeight, screenWidth, hwnd, VSYNC_ENABLED, fullScreen);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize Direct3D", L"Error", MB_OK);
		return false;
	}

	return true;
}

void FD3d12Graphics::SetCamera(FCamera * aCam)
{ 
	myCamera = aCam;
	m_Direct3D->SetCamera(aCam);
}

void FD3d12Graphics::Shutdown()
{
	// Release the Direct3D object.
	if (m_Direct3D)
	{
		m_Direct3D->Shutdown();
		delete m_Direct3D;
		m_Direct3D = 0;
	}
	return;
}


bool FD3d12Graphics::Frame()
{
	bool result;
	
	// Render the graphics scene.
	result = Render();
	if (!result)
	{
		return false;
	}
	
	return true;
}


bool FD3d12Graphics::Render()
{
	bool result;
	
	// Use the Direct3D object to render the scene.
	result = m_Direct3D->Render();
	if (!result)
	{
		return false;
	}
	
	return true;
}
