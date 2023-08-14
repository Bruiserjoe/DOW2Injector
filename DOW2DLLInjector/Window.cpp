#include "Injector.h"


HBITMAP hbitmap;

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message)
    {
    case WM_CREATE:
        
        break;
    case WM_PAINT:
        PAINTSTRUCT     ps;
        HDC             hdc;
        BITMAP          bitmap;
        HDC             hdcMem;
        HGDIOBJ         oldBitmap;

        hdc = BeginPaint(hwnd, &ps);

        hdcMem = CreateCompatibleDC(hdc);
        oldBitmap = SelectObject(hdcMem, hbitmap);

        GetObject(hbitmap, sizeof(bitmap), &bitmap);
        BitBlt(hdc, 0, 0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);

        SelectObject(hdcMem, oldBitmap);
        DeleteDC(hdcMem);

        EndPaint(hwnd, &ps);
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        DeleteObject(hbitmap);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;

}




//https://learn.microsoft.com/en-us/windows/win32/winmsg/window-class-styles
Window::Window(LPCSTR name, size_t w, size_t h)
	: instance(GetModuleHandle(nullptr)) {
	width = w;
	height = h;
	
	class_name = name;
	WNDCLASS wnd = {};
    wnd.style = CS_HREDRAW | CS_VREDRAW;
	wnd.lpszClassName = class_name;
	wnd.hInstance = instance;
	wnd.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wnd.hCursor = LoadCursorA(NULL, IDC_ARROW);
	wnd.hbrBackground = NULL;
	wnd.lpszMenuName = NULL;
	wnd.cbClsExtra = 0;
	wnd.cbWndExtra = 0;
	wnd.lpfnWndProc = WndProc;

    RegisterClass(&wnd);

    int sc_w = GetSystemMetrics(SM_CXSCREEN);
    int sc_h = GetSystemMetrics(SM_CYSCREEN);

    

    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = rect.left + width;
    rect.bottom = rect.top + height;

    AdjustWindowRect(&rect, WS_POPUP | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX, false);


	wind = CreateWindowEx(0, class_name, "DOW2 Injector", WS_POPUP | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
		rect.left, rect.right, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, instance, NULL);
	if (wind == NULL) {
		std::cout << "Failed to create window\n";
        return;
	}
    HMONITOR mon = MonitorFromWindow(wind, 0);
    MONITORINFO mon_info;
    mon_info.cbSize = sizeof(MONITORINFO);
    GetMonitorInfoA(mon, &mon_info);
    RECT mon_rect = mon_info.rcMonitor;
    rect.left = (mon_rect.right / 2) - (width / 2);
    rect.top = (mon_rect.bottom / 2) - (height / 2);
    rect.right = rect.left + width;
    rect.bottom = rect.top + height;
    MoveWindow(wind, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, true);

    ShowWindow(wind, SW_SHOW);
    hbitmap = (HBITMAP)LoadImage(instance, "mods\\test.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
}


Window::~Window() {
    UnregisterClass(class_name, instance);
}

bool Window::processMessages() {
    MSG msg = {};

    while (PeekMessage(&msg, nullptr, 0u, 0u, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return true;
}