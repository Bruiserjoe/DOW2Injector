// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"
DWORD base;
HMODULE plat;
HWND* hwnd_global = nullptr;

DWORD plat_window_setup_orig = 0;
// only called for windowed windows, fullscreen has seperate function for some reason
DWORD jmpback = 0;
void __declspec(naked) PlatWindowModeSet() {
    __asm {
        and eax, 0xB0000000;
        add eax, 0x90000000;
        jmp[jmpback];
    }
}

void __stdcall ResizeWindowToMonitor() {
    if (!hwnd_global || !*hwnd_global) {
        return;
    }

    HWND hwnd = *hwnd_global;
    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitor_info{};
    monitor_info.cbSize = sizeof(monitor_info);

    if (!GetMonitorInfoA(monitor, &monitor_info)) {
        return;
    }

    const RECT& monitor_rect = monitor_info.rcMonitor;
    SetWindowPos(
        hwnd,
        HWND_NOTOPMOST,
        monitor_rect.left,
        monitor_rect.top,
        monitor_rect.right - monitor_rect.left,
        monitor_rect.bottom - monitor_rect.top,
        SWP_FRAMECHANGED |
        SWP_SHOWWINDOW |
        SWP_NOOWNERZORDER
    );
}

DWORD plat_window_screen_detour_jmp_back = 0;
void __declspec(naked) PlatWindowSizeToScreenDetour() {
    __asm {
        pushfd;
        pushad;
        call ResizeWindowToMonitor;
        popad;
        popfd;

        // MoveWindow is __stdcall and would have removed its six arguments.
        add esp, 0x18;
        jmp[plat_window_screen_detour_jmp_back];
    }
}


DWORD plat_window_setWindowSize_orig = 0;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        base = (DWORD)GetModuleHandleA("DOW2.exe");

        plat = GetModuleHandleA("Platform.dll");
        if (plat) {
            hwnd_global = reinterpret_cast<HWND*>(reinterpret_cast<BYTE*>(plat) + 0x15810);
            plat_window_setup_orig = (DWORD)GetProcAddress(plat, MAKEINTRESOURCEA(184));
            plat_window_setWindowSize_orig = (DWORD)GetProcAddress(plat, MAKEINTRESOURCEA(180));
            jmpback = plat_window_setWindowSize_orig + 0x1D;
            JmpPatch(reinterpret_cast<BYTE*>(plat_window_setWindowSize_orig + 0x13), (DWORD)PlatWindowModeSet, 10);
            plat_window_screen_detour_jmp_back = plat_window_setWindowSize_orig + 0xD7;
            //JmpPatch(reinterpret_cast<BYTE*>(plat_window_setWindowSize_orig + 0xD1), (DWORD)PlatWindowSizeToScreenDetour, 6);
        }
    }
    return TRUE;
}
