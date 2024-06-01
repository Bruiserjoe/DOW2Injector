// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"
#include "Exports.h"
DWORD base;
HMODULE plat;
std::atomic_bool first = ATOMIC_VAR_INIT(true);
std::ofstream file;

//path and file name up to 256 bytes each
std::string cfgfile;
char* cfgp;
//path actually 1024 bytes max
std::string path;
char* pathp;

std::string flipstring(std::string str) {
    std::string ret = "";
    for (int i = str.size() - 1; i >= 0; i--) {
        ret.push_back(str[i]);
    }
    return ret;
}

bool setcfg(Injector* in, std::string module) {
        std::string t = in->createcfg(module);
        std::string cfg;
        int i = t.size() - 1;
        for (; i >= 0 && t[i] != '/'; i--) {
            cfg.push_back(t[i]);
        }
        cfg = flipstring(cfg);
        cfgfile = cfg;
        cfgp = (char*)cfgfile.c_str();
        std::string p;
        for (; i >= 0; i--) {
            p.push_back(t[i]);
        }
        p = flipstring(p);
        path = p;
        pathp = (char*)path.c_str();
        return true;
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
std::string module;
typedef bool(__stdcall* PlatGetOption)(const char* option, char* str, unsigned int size);
PlatGetOption plat_getoption = nullptr;
DWORD WINAPI MainThread(LPVOID param) {
    Injector in;
    char mod1[0x200];
    bool ret = plat_getoption("modname", mod1, 0x200);
    module = std::string(mod1);
    module = module + ".config";
    setcfg(&in, module);
    bool er = false;
    while (!er) {
        if (!first) {
            er = true;
        }
    }
    ret = plat_getoption("modname", mod1, 0x200);
    module = std::string(mod1);
    module = module + ".config";
    in.start(module);
    BYTE* src = (BYTE*)"\x55\x8B\xEC\x83\xE4\xF8";
    MemPatch(reinterpret_cast<BYTE*>(base + 0x7254F), src, 6);
    src = (BYTE*)"\x83\xC4\x0C\x68\x08\x57\x08\x01";
    MemPatch(reinterpret_cast<BYTE*>(base + 0x1D36B), src, 8);
    return 0;
}

//fix corruption of dow2 exe path when invalid folder read

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    char mod1[0x200];
    bool ret;
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        base = (DWORD)GetModuleHandleA("DOW2.exe");

        plat = GetModuleHandleA("Platform.dll");
        if (plat) {
            plat_getoption = reinterpret_cast<PlatGetOption>(GetProcAddress(plat, MAKEINTRESOURCEA(78)));
        }
        ret = plat_getoption("modname", mod1, 0x200);
        module = std::string(mod1);
        module = module + ".config";
        
        menumidhookjmp = base + 0x72555;
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x7254F), (DWORD)MenuMidHook, 6);

        jmpback_midcfgload = (base + 0x1D37D);
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x1D36B), (DWORD)MidCfgLoad, 8);

        CreateThread(0, 0, MainThread, hModule, 0, 0);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

