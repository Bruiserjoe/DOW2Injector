// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"

void error(const char* err_title, const char* err_message) {
    MessageBoxA(0, err_message, err_title, 0);
    //exit(-1);
}

typedef void(__thiscall *MainMenu)(void* ecx, int* param1);
MainMenu org_menu = reinterpret_cast<MainMenu>(0x0047254f);

PDETOUR_TRAMPOLINE trampoline = nullptr;
MainMenu real_menu = nullptr;
MainMenu real_detour = nullptr;

void __fastcall menudetour(void* ecx, int* param1) {
    //error("menu", "menu called");
    org_menu(ecx, param1);
    const char* msg = "Menu setup";
    HGLOBAL glob = GlobalAlloc(GMEM_FIXED, sizeof(msg));
    strcpy_s((char*)glob, sizeof(msg), msg);
    OpenClipboard(NULL);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, glob);
    CloseClipboard();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach((void**)&org_menu, menudetour);
    DetourTransactionCommit();
}

DWORD WINAPI MainThread(LPVOID param) {
    while (true) {
        
    }

    return 0;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    DetourRestoreAfterWith();
    DetourIsHelperProcess();
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttachEx((void**)&org_menu, menudetour, &trampoline, (void**)&real_menu, (void**)&real_detour);
        DetourTransactionCommit();
    case DLL_PROCESS_DETACH:
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach((void**)&org_menu, menudetour);
        DetourTransactionCommit();
        break;
    }
    return TRUE;
}

