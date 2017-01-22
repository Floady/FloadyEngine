#include "FD3d12System.h"
#include "FD3d12Graphics.h"
#include "FD3d12Input.h"
#include "FCamera.h"
#include "FTimer.h"
#include "FD3DClass.h"

static const bool FULL_SCREEN = false;
static FD3d12System* ApplicationHandle = 0;


LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
	switch (umessage)
	{
		// Check if the window is being destroyed.
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}

	// Check if the window is being closed.
	case WM_CLOSE:
	{
		PostQuitMessage(0);
		return 0;
	}

	// All other messages pass to the message handler in the system class.
	default:
	{
		return ApplicationHandle->MessageHandler(hwnd, umessage, wparam, lparam);
	}
	}
}



FD3d12System::FD3d12System()
{
	m_Input = 0;
	m_Graphics = 0;
	myCamera = nullptr;
	myFrameTimer = new FTimer();
	myIsFocussed = false;
}

FD3d12System::~FD3d12System()
{
}

bool FD3d12System::Initialize()
{
	bool result;

	// Initialize the width and height of the screen to zero before sending the variables into the function.
	screenHeight = 0;
	screenWidth = 0;

	// Initialize the windows api.
	InitializeWindows(screenHeight, screenWidth);

	// initialize camera
	myCamera = new FCamera(static_cast<float>(screenWidth), static_cast<float>(screenHeight));

	// Create the input object.  This object will be used to handle reading the keyboard input from the user.
	m_Input = new FD3d12Input();
	if (!m_Input)
	{
		return false;
	}

	// Initialize the input object.
	m_Input->Initialize();

	// Create the graphics object.  This object will handle rendering all the graphics for this application.
	m_Graphics = new FD3d12Graphics();
	if (!m_Graphics)
	{
		return false;
	}

	// Initialize the graphics object.
	result = m_Graphics->Initialize(screenHeight, screenWidth, m_hwnd, FULL_SCREEN);
	
	// propagate camera
	m_Graphics->SetCamera(myCamera);

	if (!result)
	{
		return false;
	}

	return true;
}

void FD3d12System::Shutdown()
{
	// Release the graphics object.
	if (m_Graphics)
	{
		m_Graphics->Shutdown();
		delete m_Graphics;
		m_Graphics = 0;
	}

	// Release the input object.
	if (m_Input)
	{
		delete m_Input;
		m_Input = 0;
	}

	// Shutdown the window.
	ShutdownWindows();

	return;
}

void FD3d12System::Run()
{
	MSG msg;
	bool done, result;

	char buff[512];
	sprintf_s(buff, "load time: %f \n", myFrameTimer->GetTimeMS());
	OutputDebugStringA(buff);
	myFrameTimer->Restart();
	
	m_Input->SetMousePos(screenWidth / 2, screenHeight / 2);
	
	// Initialize the message structure.
	ZeroMemory(&msg, sizeof(MSG));

	// Loop until there is a quit message from the window or the user.
	done = false;
	myFrameTime = 30.0;
	while (!done)
	{
		// Handle the windows messages.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// If windows signals to end the application then exit out.
		if (msg.message == WM_QUIT)
		{
			done = true;
		}
		else
		{
			// Otherwise do the frame processing.
			result = Frame();
			myFrameTime = myFrameTimer->GetTimeMS();

			// never drop under 800 :)
			if(myFrameTime > 1 / 3800.0)
			{
				char buff[512];
				sprintf_s(buff, "Fps: %f \n", 1 / myFrameTime);
				OutputDebugStringA(buff);
			}

			myFrameTimer->Restart();
			
			//Sleep(8.0f - myFrameTime); // frame limiter

			if (!result)
			{
				done = true;
			}
		}

	}

	return;
}

bool FD3d12System::Frame()
{
	bool result;


	// Check if the user pressed escape and wants to exit the application.
	if (m_Input->IsKeyDown(VK_ESCAPE))
	{
		return false;
	}
	
	// Set camera probably should also just be in camera, and calling input iskeydown but whatevs
	float movSpeed = static_cast<float>(myFrameTime * 10);
	if (m_Input->IsKeyDown(65))
		myCamera->Move(-movSpeed, 0, 0);
	if (m_Input->IsKeyDown(68))
		myCamera->Move(movSpeed, 0, 0);
	if (m_Input->IsKeyDown(87))
		myCamera->Move(0, 0, movSpeed);
	if (m_Input->IsKeyDown(83))
	{
		myCamera->Move(0, 0, -movSpeed);
	}
	if (m_Input->IsKeyDown(49))
	{
		m_Graphics->GetD3DClass()->GetShaderManager().ReloadShaders();
	}
	if (myIsFocussed)
	{
		int deltaX = m_Input->GetMouseX() - (screenWidth / 2);
		int deltaY = m_Input->GetMouseY() - (screenHeight / 2);

		SetCursorPos(posX + screenWidth / 2, posY + screenHeight / 2);

		// don't time correct rotatio nsince the mousemove per frame is shorter when frames are shorter times
		// should be direction normalized times frametime?
		float rotSpeed = 0.005f;
		myCamera->Yaw(deltaX * rotSpeed);
		myCamera->Pitch(deltaY * rotSpeed);
		
		myCamera->UpdateViewProj();
	}
	
	// Do the frame processing for the graphics object.
	result = m_Graphics->Frame();
	if (!result)
	{
		return false;
	}

	//Sleep(20);

	return true;
}

LRESULT CALLBACK FD3d12System::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	switch (umsg)
	{
		// Check if a key has been pressed on the keyboard.
	case WM_KEYDOWN:
	{
		// If a key is pressed send it to the input object so it can record that state.
		m_Input->KeyDown((unsigned int)wparam);
		return 0;
	}

	case WM_MOUSEMOVE:
	{
		// If a key is pressed send it to the input object so it can record that state.
		m_Input->MouseMove((unsigned int)lparam & 0x0000FFFF, (unsigned int)lparam >> 16);
		return 0;
	}

	case WM_SETFOCUS:
	{
		myIsFocussed = true;
		return 0;
	}

	case WM_KILLFOCUS:
	{
		myIsFocussed = false;
		return 0;
	}

	// Check if a key has been released on the keyboard.
	case WM_KEYUP:
	{
		// If a key is released then send it to the input object so it can unset the state for that key.
		m_Input->KeyUp((unsigned int)wparam);
		return 0;
	}

	// Any other messages send to the default message handler as our application won't make use of them.
	default:
	{
		return DefWindowProc(hwnd, umsg, wparam, lparam);
	}
	}
}

void FD3d12System::InitializeWindows(int& screenHeight, int& screenWidth)
{
	WNDCLASSEX wc;
	DEVMODE dmScreenSettings;


	// Get an external pointer to this object.	
	ApplicationHandle = this;

	// Get the instance of this application.
	m_hinstance = GetModuleHandle(NULL);

	// Give the application a name.
	m_applicationName = L"Engine";

	// Setup the windows class with default settings.
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = m_hinstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = m_applicationName;
	wc.cbSize = sizeof(WNDCLASSEX);

	// Register the window class.
	RegisterClassEx(&wc);

	// Determine the resolution of the clients desktop screen.
	screenHeight = GetSystemMetrics(SM_CYSCREEN);
	screenWidth = GetSystemMetrics(SM_CXSCREEN);

	// Setup the screen settings depending on whether it is running in full screen or in windowed mode.
	if (FULL_SCREEN)
	{
		// If full screen set the screen to maximum size of the users desktop and 32bit.
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsHeight = (unsigned long)screenHeight;
		dmScreenSettings.dmPelsWidth = (unsigned long)screenWidth;
		dmScreenSettings.dmBitsPerPel = 32;
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		// Change the display settings to full screen.
		ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

		// Set the position of the window to the top left corner.
		posX = posY = 0;
	}
	else
	{
		// If windowed then set it to 800x600 resolution.
		screenWidth = 800;
		screenHeight = 600;

		// Place the window in the middle of the screen.
		posX = (GetSystemMetrics(SM_CXSCREEN) - screenWidth) / 2;
		posY = (GetSystemMetrics(SM_CYSCREEN) - screenHeight) / 2;
	}

	// Create the window with the screen settings and get the handle to it.
	m_hwnd = CreateWindowEx(WS_EX_APPWINDOW, m_applicationName, m_applicationName,
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP,
		posX, posY, screenWidth, screenHeight, NULL, NULL, m_hinstance, NULL);

	// Bring the window up on the screen and set it as main focus.
	ShowWindow(m_hwnd, SW_SHOW);
	SetForegroundWindow(m_hwnd);
	SetFocus(m_hwnd);

	// Hide the mouse cursor.
	ShowCursor(false);

	return;
}

void FD3d12System::ShutdownWindows()
{
	// Show the mouse cursor.
	ShowCursor(true);

	// Fix the display settings if leaving full screen mode.
	if (FULL_SCREEN)
	{
		ChangeDisplaySettings(NULL, 0);
	}

	// Remove the window.
	DestroyWindow(m_hwnd);
	m_hwnd = NULL;

	// Remove the application instance.
	UnregisterClass(m_applicationName, m_hinstance);
	m_hinstance = NULL;

	// Release the pointer to this class.
	ApplicationHandle = NULL;

	return;
}