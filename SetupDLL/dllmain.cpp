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
HMODULE hmod;
bool first = false;
//param1 is null
void __fastcall menudetour(void* ecx, int* param1) {
   
    int* in = (int*)(ecx); //no idea why this works
    org_menu(ecx, in);
    

    if (!first) {
        OpenClipboard(NULL);
        EmptyClipboard();

        const char* msg = "Menu setup";
        HGLOBAL glob = GlobalAlloc(GMEM_FIXED, sizeof(char) * 11);
        char* buffer = (char*)GlobalLock(glob);
        strcpy_s(buffer, sizeof(char) * 11, msg);
        GlobalUnlock(glob);

        SetClipboardData(CF_TEXT, glob);
        CloseClipboard();
        first = true;
    }
    
    //FreeLibrary(hmod);
}



BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    DetourRestoreAfterWith();
    DetourIsHelperProcess();
    
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        hmod = hModule;
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        //DetourAttach((void**)&st_org, importdetour);
        DetourAttachEx((void**)&org_menu, menudetour, &trampoline, (void**)&real_menu, (void**)&real_detour);
        DetourTransactionCommit();
        //CreateThread(0, 0, MainThread, hModule, 0, 0);
    case DLL_PROCESS_DETACH:
        /*error("detach", "got detached");
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach((void**)&org_menu, menudetour);
        DetourTransactionCommit();*/
        break;
    }
    return TRUE;
}

