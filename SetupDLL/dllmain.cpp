// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"

bool checkClipboard(std::string comp) {
    bool ret = false;
    if (OpenClipboard(NULL)) {
        char* buffer = (char*)GetClipboardData(CF_TEXT);
        if (buffer) {
            std::string str(buffer);
            if (str.compare(comp) == 0) {
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

std::string flipstring(std::string str) {
    std::string ret = "";
    for (int i = str.size() - 1; i >= 0; i--) {
        ret.push_back(str[i]);
    }
    return ret;
}

//path and file name up to 256 bytes each
std::string cfgfile;
char* cfgp;
//path actually 1024 bytes max
std::string path;
char* pathp;
//reads the clipboard and sets the cfg path variables
bool setcfg() {
    if (OpenClipboard(NULL)) {
        char* buffer = (char*)GetClipboardData(CF_TEXT);
        if (buffer) {
            std::string str(buffer);
            std::string cfg;
            int i = str.size() - 1;
            for (; i >= 0 && str[i] != '/'; i--) {
                cfg.push_back(str[i]);
            }
            cfg = flipstring(cfg);
            cfgfile = cfg;
            cfgp = (char*)cfgfile.c_str();
            std::string p;
            for (; i >= 0; i--) {
                p.push_back(str[i]);
            }
            p = flipstring(p);
            path = p;
            pathp = (char*)path.c_str();
            CloseClipboard();
            {
                OpenClipboard(NULL);
                EmptyClipboard();

                const char* msg = "Finish CFG";
                HGLOBAL glob = GlobalAlloc(GMEM_FIXED, sizeof(char) * 11);
                char* buffer = (char*)GlobalLock(glob);
                if (buffer) {
                    strcpy_s(buffer, sizeof(char) * 11, msg);
                }
                GlobalUnlock(glob);

                SetClipboardData(CF_TEXT, glob);
                CloseClipboard();
            }
            return true;
        }
    }
    CloseClipboard();
    Sleep(8);
    return false;
}


DWORD base;
DWORD jmpback_midcfgload;
void __declspec(naked) MidCfgLoad() {
    __asm {
        add esp, 0x0C;
        push cfgp;
        mov eax, esi;
        push pathp;
        lea eax, [ebp - 0x118];
        push eax;
        jmp[jmpback_midcfgload];
    }
}


DWORD WINAPI MainThread(LPVOID param) {
    bool er = false;
    //getting the clipboard
    while (!setcfg());

    while (!er) {
        if (!first) {
            if (checkClipboard("Injection Start")) {
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
    /*hmod = GetModuleHandleA("SetupDLL.dll");
    if (hmod) {
        FreeLibrary(hmod);
    }*/
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
        base = (DWORD)GetModuleHandleA("DOW2.exe");
        hmod = hModule;

        CreateThread(0, 0, MainThread, hModule, 0, 0);
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        //DetourAttach((void**)&st_org, importdetour);
        DetourAttachEx((void**)&org_menu, menudetour, &trampoline, (void**)&real_menu, (void**)&real_detour);
        DetourTransactionCommit();
        jmpback_midcfgload = (base + 0x1D37D);
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x1D36B), (DWORD)MidCfgLoad, 8);
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

