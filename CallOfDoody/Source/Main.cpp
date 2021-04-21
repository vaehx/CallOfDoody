#include "CallOfDoody.h"
#include <GameFramework\Implementation\SpeedPointEngine.h>
#include <Windows.h>

using namespace SpeedPoint;

// -------------------------------------------------------------------------------------------
void Error(const char* msg)
{
	MessageBox(NULL, msg, "Error", MB_ICONERROR | MB_OK);
}

// -------------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch (iMessage)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			PostQuitMessage(0);
		break;
	default:
		return ::DefWindowProc(hWnd, iMessage, wParam, lParam);
	}

	return 0;
}

// -------------------------------------------------------------------------------------------
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int nParam)
{
	bool fullscreen = false;
	unsigned int viewportRes[2] = { 1920, 1080 };
	unsigned int windowedViewportRes[2] = { 1366, 768 };
	
	bool showLogWindow = false;
	unsigned int logWindowWidth = 550;

	// Initialize window
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(wcex);
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hInstance = hInstance;
	wcex.lpfnWndProc = WndProc;
	wcex.lpszClassName = "CallOfDoody";
	wcex.lpszMenuName = 0;
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClassEx(&wcex);

	RECT mainWndRect = { 0, 0, viewportRes[0], viewportRes[1] };
	if (!fullscreen)
	{
		mainWndRect.left = 100;
		mainWndRect.top = 100;
		mainWndRect.right = windowedViewportRes[0];
		mainWndRect.bottom = windowedViewportRes[1];
		if (showLogWindow)
		{
			mainWndRect.right += logWindowWidth + 20;
			mainWndRect.bottom += 45;
		}
	}

	HWND hWndMain = CreateWindow(wcex.lpszClassName, "CallOfDoody", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		mainWndRect.left, mainWndRect.top, mainWndRect.right, mainWndRect.bottom, NULL, NULL, hInstance, 0);
	
	HWND hWndLog, hWndViewport;
	if (fullscreen)
	{
		hWndViewport = hWndMain;
		hWndLog = NULL;
	}
	else
	{
		// Initialize view window
		wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wcex.hCursor = NULL;
		wcex.lpszClassName = "CallOfDoodyView";
		RegisterClassEx(&wcex);

		hWndViewport = CreateWindowEx(0, wcex.lpszClassName, "CallOfDoodyView", WS_CHILD | WS_VISIBLE,
			0, 0, windowedViewportRes[0], windowedViewportRes[1], hWndMain, 0, hInstance, 0);

		// Initialize logging window
		hWndLog = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
			WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL,
			viewportRes[0], 0, logWindowWidth, viewportRes[1],
			hWndMain, 0, hInstance, 0);

		NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS) };
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
		ncm.lfMessageFont.lfHeight = 5;
		char faceName[32] = "Courier\0";
		memcpy((void*)ncm.lfMessageFont.lfFaceName, (void*)faceName, 32);
		HFONT hFont = CreateFontIndirect(&ncm.lfMessageFont);
		::SendMessageA(hWndLog, WM_SETFONT, (WPARAM)hFont, TRUE);

		if (!showLogWindow)
			::ShowWindow(hWndLog, SW_HIDE);
	}




	// Initialize engine
	CallOfDoody game(hInstance, hWndViewport, hWndLog);

	CLog::SetLogLevel(eLOGLEVEL_DEBUG);
	CLog::RegisterListener(&game);

	SpeedPointEngine engine;
	SGameEngineInitParams engineParams;
	engineParams.logFilename = "CallOfDoody.log";
	engineParams.rendererImplementation = S_DIRECTX11;
	engineParams.rendererParams.enableVSync = true;
	engineParams.rendererParams.hWnd = hWndViewport;
	engineParams.rendererParams.multithreaded = false;
	engineParams.rendererParams.resolution[0] = viewportRes[0];
	engineParams.rendererParams.resolution[1] = viewportRes[1];
	engineParams.rendererParams.shaderResourceDirectory = engine.GetShaderDirectoryPath();
	engineParams.rendererParams.windowed = !fullscreen;

	if (Failure(engine.Initialize(engineParams)))
	{
		Error("Failed initialize engine!");
		return 1;
	}

	engine.RegisterApplication(&game);
	engine.Start();

	ShowCursor(FALSE);

	// Main loop
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));
	bool bRunning = true;	
	while (bRunning)
	{
		while (PeekMessage(&msg, hWndMain, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			switch (msg.message)
			{
			case WM_KEYDOWN:
				switch (msg.wParam)
				{
				case VK_ESCAPE:
					{
						bRunning = false;
						PostQuitMessage(0);
						break;
					}
				case VK_F3:
					{
						CPlayer* pPlayer = game.GetPlayer();
						pPlayer->EnableFreeCam(!pPlayer->IsFreeCamEnabled());
						break;
					}
				case VK_F2:
					{
						engine.GetPhysics()->ShowHelpers(!engine.GetPhysics()->HelpersShown());
						break;
					}
				case VK_F8:
					{
						engine.GetPhysics()->Pause(!engine.GetPhysics()->IsPaused());
						break;
					}
				}
				break;
			case WM_QUIT:
				bRunning = false;
				break;
			case WM_SETCURSOR:
				if (msg.hwnd == hWndViewport)
				{
					SetCursor(NULL);
					ShowCursor(FALSE);
				}
				break;
			}
		}

		if (::GetFocus() != hWndMain)
		{
			WaitMessage();
			continue;
		}

		engine.DoFrame();
	}

	engine.Shutdown();
	DestroyWindow(hWndMain);

	return 0;
}
