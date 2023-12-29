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

DWORD menumidhookjmp = 0;
void __declspec(naked) MenuMidHook() {
    __asm {
        push ebp;
        mov ebp, esp;
        and esp, 0x0FFFFFFF8;
        mov first, 0x0;
        jmp[menumidhookjmp];
    }
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
    BYTE* src = (BYTE*)"\x55\x8B\xEC\x83\xE4\xF8";
    MemPatch(reinterpret_cast<BYTE*>(base + 0x7254F), src, 6);
    src = (BYTE*)"\x83\xC4\x0C\x68\x08\x57\x08\x01";
    MemPatch(reinterpret_cast<BYTE*>(base + 0x1D36B), src, 8);
    file.close();
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        file.open("mod_logs\\startup.log");
        base = (DWORD)GetModuleHandleA("DOW2.exe");
        menumidhookjmp = base + 0x72555;
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x7254F), (DWORD)MenuMidHook, 6);
        hmod = hModule;

        CreateThread(0, 0, MainThread, hModule, 0, 0);
        
        jmpback_midcfgload = (base + 0x1D37D);
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x1D36B), (DWORD)MidCfgLoad, 8);
    case DLL_PROCESS_DETACH:
        
        break;
    }
    return TRUE;
}

