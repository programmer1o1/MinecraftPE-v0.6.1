#ifndef MAIN_WIN32_H__
#define MAIN_WIN32_H__

/*
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
*/

#include "client/renderer/gles.h"
#include <EGL/egl.h>
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <windowsx.h>

#include <WinSock2.h>
#include <process.h>

#include <cstdio>
#include "platform/input/Mouse.h"
#include "platform/input/Multitouch.h"
#include "util/Mth.h"
#include "AppPlatform_win32.h"

static App* g_app = 0;
static volatile bool g_running = true;

static int getBits(int bits, int startBitInclusive, int endBitExclusive, int shiftTruncate) {
	int sum = 0;
	for (int i = startBitInclusive; i<endBitExclusive; ++i)
		sum += (bits & (2<<i));
	return shiftTruncate? (sum >> startBitInclusive) : sum;
}

// Map Win32 virtual keys to the game's internal key codes.
static unsigned char transformKey_win32(WPARAM wParam) {
	if (wParam == VK_LSHIFT || wParam == VK_SHIFT) return Keyboard::KEY_LSHIFT;
	if (wParam == VK_TAB) return 250;  // internal Tab code (same as macOS/Linux)
	return (unsigned char)wParam;
}

void resizeWindow(HWND hWnd, int nWidth, int nHeight) {
   RECT rcClient, rcWindow;
   POINT ptDiff;
     GetClientRect(hWnd, &rcClient);
     GetWindowRect(hWnd, &rcWindow);
   ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
   ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
   MoveWindow(hWnd,rcWindow.left, rcWindow.top, nWidth + ptDiff.x, nHeight + ptDiff.y, TRUE);
}

void toggleResolutions(HWND hwnd, int direction) {
	static int n = 0;
	static int sizes[][3] = {
		{854, 480, 1},
		{800, 480, 1},
		{480, 320, 1},
		{1024, 768, 1},
		{1280, 800, 1},
		{1024, 580, 1}
	};
	static int count = sizeof(sizes) / sizeof(sizes[0]);
	n = (count + n + direction) % count;
	
	int* size = sizes[n];
	int k = size[2];
	
	resizeWindow(hwnd, k * size[0], k * size[1]);
}

LRESULT WINAPI windowProc ( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	LRESULT retval = 1;
	
	switch (uMsg)
	{
	case WM_KEYDOWN: {
		if (wParam == 33) toggleResolutions(hWnd, -1);
		if (wParam == 34) toggleResolutions(hWnd, +1);
		unsigned char k = transformKey_win32(wParam);
		if (k) Keyboard::feed(k, 1);
		return 0;
	}
	case WM_KEYUP: {
		unsigned char k = transformKey_win32(wParam);
		if (k) Keyboard::feed(k, 0);
		return 0;
	}
	case WM_CHAR: {
		//LOGW("WM_CHAR: %d\n", wParam);
		if(wParam >= 32)
			Keyboard::feedText(wParam);
		return 0;
	}
	case WM_LBUTTONDOWN: {
		Mouse::feed( MouseAction::ACTION_LEFT, 1, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		Multitouch::feed(1, 1, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0);
		break;
	}
	case WM_LBUTTONUP: {
		Mouse::feed( MouseAction::ACTION_LEFT, 0, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		Multitouch::feed(1, 0, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0);
		break;
	}
	case WM_RBUTTONDOWN: {
		Mouse::feed( MouseAction::ACTION_RIGHT, 1, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;
	}
	case WM_RBUTTONUP: {
		Mouse::feed( MouseAction::ACTION_RIGHT, 0, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;
	}
	case WM_MOUSEMOVE: {
		if (!g_win32MouseCaptured) {
			Mouse::feed(MouseAction::ACTION_MOVE, 0, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			Multitouch::feed(0, 0, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0);
		}
		break;
	}
	case WM_MOUSEWHEEL: {
		short delta = GET_WHEEL_DELTA_WPARAM(wParam);
		Mouse::feed(MouseAction::ACTION_WHEEL, (delta != 0) ? 1 : 0,
		            GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, (short)(delta / 120));
		break;
	}
	case WM_INPUT: {
		if (g_win32MouseCaptured) {
			UINT size = sizeof(RAWINPUT);
			RAWINPUT raw;
			if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER)) != (UINT)-1) {
				if (raw.header.dwType == RIM_TYPEMOUSE &&
					!(raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)) {
					short dx = (short)raw.data.mouse.lLastX;
					short dy = (short)raw.data.mouse.lLastY;
					if (dx != 0 || dy != 0) {
						RECT r;
						GetClientRect(hWnd, &r);
						Mouse::feed(MouseAction::ACTION_MOVE, 0,
						            (short)((r.right - r.left) / 2),
						            (short)((r.bottom - r.top) / 2),
						            dx, dy);
					}
				}
			}
		}
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	case WM_KILLFOCUS: {
		if (g_win32MouseCaptured) {
			g_win32MouseCaptured = false;
			ShowCursor(TRUE);
			ClipCursor(NULL);
		}
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	default:
		if (uMsg == WM_NCDESTROY) g_running = false;
		else {
			if (uMsg == WM_SIZE) {
				if (g_app) g_app->setSize( GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) );
			}
		}
		retval = DefWindowProc (hWnd, uMsg, wParam, lParam);
		break;
	}
	return retval;
}

void platform(HWND *result, int width, int height) {
	WNDCLASS wc;
	RECT wRect;
	HWND hwnd;
	HINSTANCE hInstance;

	wRect.left = 0L;
	wRect.right = (long)width;
	wRect.top = 0L;
	wRect.bottom = (long)height;

	hInstance = GetModuleHandle(NULL);

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = (WNDPROC)windowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "OGLES";

	RegisterClass(&wc);

	AdjustWindowRectEx(&wRect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);

	hwnd = CreateWindowEx(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE, "OGLES", "main", WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, wRect.right-wRect.left, wRect.bottom-wRect.top, NULL, NULL, hInstance, NULL);
	*result = hwnd;
}

/** Thread that reads input data via UDP network datagrams
    and fills Mouse and Keyboard structures accordingly.
	@note: The bound local net address is unfortunately
	       hard coded right now (to prevent wrong Interface) */
void inputNetworkThread(void* userdata)
{
	// set up an UDP socket for listening
	WSADATA wsaData;
	if (WSAStartup(0x101, &wsaData)) {
		printf("Couldn't initialize winsock\n");
		return;
	}

	SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s == INVALID_SOCKET) {
		printf("Couldn't create socket\n");
		return;
	}
	
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(9991);
	addr.sin_addr.s_addr = inet_addr("192.168.0.119");

	if (bind(s, (sockaddr*)&addr, sizeof(addr))) {
		printf("Couldn't bind socket to port 9991\n");
		return;
	}
	
	sockaddr fromAddr;
	int fromAddrLen = sizeof(fromAddr);

	char buf[1500];
	int* iptrBuf = (int*)buf;

	printf("input-server listening...\n");

	while (1) {
		int read = recvfrom(s, buf, 1500, 0, &fromAddr, &fromAddrLen);
		if (read < 0)
		{
			printf("recvfrom failed with code: %d\n", WSAGetLastError());
			return;
		}
		// Keyboard
		if (read == 2) {
			Keyboard::feed((unsigned char) buf[0], (int)buf[1]);
		}
		// Mouse
		else if (read == 16) {
			Mouse::feed(iptrBuf[0], iptrBuf[1], iptrBuf[2], iptrBuf[2]);
		}
	}
}

int main(void) {
	AppContext appContext;
	MSG sMessage;

#ifndef STANDALONE_SERVER

	EGLint aEGLAttributes[] = {
		EGL_RED_SIZE,		8,
		EGL_GREEN_SIZE,		8,
		EGL_BLUE_SIZE,		8,
		EGL_ALPHA_SIZE,		8,
		EGL_DEPTH_SIZE,		16,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
		EGL_NONE
	};
	EGLint aEGLContextAttributes[] = {
		EGL_CONTEXT_CLIENT_VERSION, 1,
		EGL_NONE
	};

	EGLConfig m_eglConfig[1];
	EGLint nConfigs;

	HWND hwnd;
	g_running = true;

	// Platform init.
	appContext.platform = new AppPlatform_win32();
	platform(&hwnd, appContext.platform->getScreenWidth(), appContext.platform->getScreenHeight());
	ShowWindow(hwnd, SW_SHOW);
	SetForegroundWindow(hwnd);
	SetFocus(hwnd);
	g_win32Hwnd = hwnd;
	// Register for raw mouse input (enables in-game relative mouse look)
	{
		RAWINPUTDEVICE rid;
		rid.usUsagePage = 0x01;  // HID_USAGE_PAGE_GENERIC
		rid.usUsage     = 0x02;  // HID_USAGE_GENERIC_MOUSE
		rid.dwFlags     = 0;
		rid.hwndTarget  = hwnd;
		RegisterRawInputDevices(&rid, 1, sizeof(rid));
	}

	// EGL init.
	appContext.display = eglGetDisplay(GetDC(hwnd));
	//m_eglDisplay = eglGetDisplay((EGLNativeDisplayType) EGL_DEFAULT_DISPLAY);

	eglInitialize(appContext.display, NULL, NULL);

	eglChooseConfig(appContext.display, aEGLAttributes, m_eglConfig, 1, &nConfigs);
	printf("EGLConfig = %p\n", m_eglConfig[0]);

	appContext.surface = eglCreateWindowSurface(appContext.display, m_eglConfig[0], (NativeWindowType)hwnd, 0);
	printf("EGLSurface = %p\n", appContext.surface);

	appContext.context = eglCreateContext(appContext.display, m_eglConfig[0], EGL_NO_CONTEXT, NULL);//aEGLContextAttributes);
	printf("EGLContext = %p\n", appContext.context);
	if (!appContext.context) {
		printf("EGL error: %d\n", eglGetError());
	}

	eglMakeCurrent(appContext.display, appContext.surface, appContext.surface, appContext.context);
	
	glInit();

#endif
	App* app = new MAIN_CLASS();

	g_app = app;
	((MAIN_CLASS*)g_app)->externalStoragePath = ".";
	((MAIN_CLASS*)g_app)->externalCacheStoragePath = ".";
	g_app->init(appContext);
	g_app->setSize(appContext.platform->getScreenWidth(), appContext.platform->getScreenHeight());

	//_beginthread(inputNetworkThread, 0, 0);
	
	// Main event loop
	while(g_running && !app->wantToQuit())
	{
		// Do Windows stuff:
		while (PeekMessage (&sMessage, NULL, 0, 0, PM_REMOVE) > 0) {
			if(sMessage.message == WM_QUIT) {
				g_running = false;
				break;
			}
			else {
				TranslateMessage(&sMessage);
				DispatchMessage(&sMessage);
			}
		}
		app->update();
		
		//Sleep(30);
	}

	Sleep(50);
	delete app;
	Sleep(50);
	appContext.platform->finish();
	Sleep(50);
	delete appContext.platform;
	Sleep(50);
	//printf("_crtDumpMemoryLeaks: %d\n", _CrtDumpMemoryLeaks());
	
#ifndef STANDALONE_SERVER
	// Exit.
	eglMakeCurrent(appContext.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(appContext.display, appContext.context);
	eglDestroySurface(appContext.display, appContext.surface);
	eglTerminate(appContext.display);
#endif

	return 0;
}

#endif /*MAIN_WIN32_H__*/
