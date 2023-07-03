// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"

bool checkClipboard() {
    bool ret = false;
    if (OpenClipboard(NULL)) {
        char* buffer = (char*)GetClipboardData(CF_TEXT);
        if (buffer) {
            std::string str(buffer);
            if (str.compare("Injection Start") == 0) {
                ret =  true;
            }
        }
    }
    CloseClipboard();
    return ret;
}



typedef void(__thiscall *MainMenu)(void* ecx, int* param1);
MainMenu org_menu = reinterpret_cast<MainMenu>(0x0047254f);

PDETOUR_TRAMPOLINE trampoline = nullptr;
MainMenu real_menu = nullptr;
MainMenu real_detour = nullptr;
HMODULE hmod;
std::atomic_bool first = ATOMIC_VAR_INIT(true);
std::ofstream file;
//param1 is null
void __fastcall menudetour(void* ecx, int* param1) {
    file << "StartupDLL: Menu before call\n";
    int* in = (int*)(ecx); //no idea why this works
    org_menu(ecx, in);
    file << "StartupDLL: Menu called\n";
    first = false;
   
}

DWORD WINAPI MainThread(LPVOID param) {
    bool er = false;
    while (!er) {
        if (!first) {
            if (checkClipboard()) {
                file << "StartupDLL: exiting clipboard thread\n";
                er = true;
                break;
            }
            OpenClipboard(NULL);
            EmptyClipboard();

            const char* msg = "Menu setup";
            HGLOBAL glob = GlobalAlloc(GMEM_FIXED, sizeof(char) * 11);
            char* buffer = (char*)GlobalLock(glob);
            strcpy_s(buffer, sizeof(char) * 11, msg);
            GlobalUnlock(glob);

            SetClipboardData(CF_TEXT, glob);
            CloseClipboard();
            //try waiting for the injector to write something back that's probably what causes the catch
            file << "StartupDLL: Startup clipboard set\n";
            Sleep(100);
        }
    }
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach((void**)&org_menu, menudetour);
    DetourTransactionCommit();
    file.close();
    //FreeLibrary(hmod);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    //DetourRestoreAfterWith();
    //DetourIsHelperProcess();
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        file.open("mod_logs\\startup.log");
        hmod = hModule;
        CreateThread(0, 0, MainThread, hModule, 0, 0);
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

