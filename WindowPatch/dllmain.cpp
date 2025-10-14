// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"
DWORD base;
HMODULE plat;

DWORD plat_window_setup_orig = 0;

DWORD jmpback = 0;
void __declspec(naked) PlatWindowSizeDetour() {
    __asm {
        mov eax, ds:0x10015834;

        jmp[jmpback];
    }
}

DWORD plat_window_setWindowSize_orig = 0;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        base = (DWORD)GetModuleHandleA("DOW2.exe");

        plat = GetModuleHandleA("Platform.dll");
        if (plat) {
            plat_window_setup_orig = (DWORD)GetProcAddress(plat, MAKEINTRESOURCEA(184));
            plat_window_setWindowSize_orig = (DWORD)GetProcAddress(plat, MAKEINTRESOURCEA(180));
            jmpback = plat_window_setWindowSize_orig + 8;
            JmpPatch(reinterpret_cast<BYTE*>(plat_window_setWindowSize_orig + 3), (DWORD)PlatWindowSizeDetour, 5);
        }
    }
    return TRUE;
}

