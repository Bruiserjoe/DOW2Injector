#include "Injector.h"




void onPaint(HDC hdc) {
    Gdiplus::Bitmap bmp(L"test.bmp");
    Gdiplus::Graphics graphics(hdc);
    Gdiplus::Pen pen(Gdiplus::Color(255, 0, 0, 255));
    graphics.DrawImage(&bmp, 0, 0, 603, 300);
    
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HDC hdc, hmemdc;
    PAINTSTRUCT ps;
    RECT client;
    switch (message)
    {
    case WM_CREATE:
        //InvalidateRect(hwnd, NULL, TRUE);
        break;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        onPaint(hdc);
        EndPaint(hwnd, &ps);
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
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
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&plus_token, &gdiplusStartupInput, NULL);

	class_name = name;
	WNDCLASS wnd = {};
    wnd.style = CS_HREDRAW | CS_VREDRAW;
	wnd.lpszClassName = class_name;
	wnd.hInstance = instance;
	wnd.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wnd.hCursor = LoadCursorA(NULL, IDC_ARROW);
	wnd.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
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
    //| WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX
    AdjustWindowRect(&rect, WS_POPUP, false);


	wind = CreateWindowEx(0, class_name, "DOW2 Injector", WS_POPUP,
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
    UpdateWindow(wind);
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
    Gdiplus::GdiplusShutdown(plus_token);
    return true;
}