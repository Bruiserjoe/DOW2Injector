// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"
#include "Exports.h"
DWORD base;
HMODULE plat;
HMODULE debug;
std::atomic_bool first = ATOMIC_VAR_INIT(true);
std::ofstream file;

Timestampedf Timestampedtracef = nullptr;
Fatalf Fatal_f = nullptr;

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


Injector in;
char mod1[0x200];
std::string module;
typedef bool(__stdcall* PlatGetOption)(const char* option, char* str, unsigned int size);
PlatGetOption plat_getoption = nullptr;
extern "C" void setmodule() {
    bool ret = plat_getoption("modname", mod1, 0x200);
    module = std::string(mod1);
    module = module + ".config";
    if (module.compare(".config") == 0) {
        module = "default_config.config";
    }
    setcfg(&in, module);
}

DWORD jmpback_midcfgload;
void __declspec(naked) MidCfgLoad() {
    __asm {
        add esp, 0x0C;
        call setmodule;
        push cfgp;
        mov eax, esi;
        push pathp;
        lea eax, [ebp - 0x118];
        push eax;
        jmp[jmpback_midcfgload];
    }
}


//https://github.com/maximumgame/DOW2CoreFix/blob/master/DOW2CoreFix/main.cpp
//via Maximumgame
static void (WINAPI* RealGetSystemInfo)(LPSYSTEM_INFO info) = GetSystemInfo;

void WINAPI GetSystemInfoDetour(LPSYSTEM_INFO info)
{
    RealGetSystemInfo(info);

    //dow2 will hang if greater than 12 cores
    if (info->dwNumberOfProcessors > 12)
        info->dwNumberOfProcessors = 12;
}

bool Init(HINSTANCE hModule)
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    //hook GetSystemInfo
    DetourAttach(&(PVOID&)RealGetSystemInfo, GetSystemInfoDetour);
    DetourTransactionCommit();
    return true;
}

DWORD WINAPI MainThread(LPVOID param) {

    bool er = false;
    while (!er) {
        if (!first) {
            er = true;
        }
    }
    er = plat_getoption("modname", mod1, 0x200);
    module = std::string(mod1);
    module = module + ".config";
    if (module.compare(".config") != 0) {
        in.start(module);
        Timestampedtracef("PROFILER: Finished injecting dlls");
    }
    BYTE* src = (BYTE*)"\x55\x8B\xEC\x83\xE4\xF8";
    MemPatch(reinterpret_cast<BYTE*>(base + 0x7254F), src, 6);
    src = (BYTE*)"\x83\xC4\x0C\x68\x08\x57\x08\x01";
    MemPatch(reinterpret_cast<BYTE*>(base + 0x1D36B), src, 8);

    return 0;
}


typedef bool(__stdcall *Plat_Options_Setup)(char* tis, const char* a2);
Plat_Options_Setup plat_options_setup = nullptr;


DWORD jmpback_midoptions;
void __declspec(naked) MidOptionsSetup() {
    __asm {
        call plat_options_setup;
        jmp[jmpback_midoptions];
    }
}

//if sxstrace problem ever comes up again
//https://superuser.com/questions/1057460/error-the-application-has-failed-to-start-because-the-side-by-side-configuratio
//400000
BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    bool ret;
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        ret = Init(hModule);
        if (in.getExe().find("DOW2.exe") != std::string::npos) {
            base = (DWORD)GetModuleHandleA("DOW2.exe");

            plat = GetModuleHandleA("Platform.dll");
            debug = GetModuleHandleA("Debug.dll");
            if (plat) {
                plat_getoption = reinterpret_cast<PlatGetOption>(GetProcAddress(plat, MAKEINTRESOURCEA(78)));
                plat_options_setup = reinterpret_cast<Plat_Options_Setup>(GetProcAddress(plat, MAKEINTRESOURCEA(183)));
            }
            if (debug) {
                Timestampedtracef = reinterpret_cast<Timestampedf>(GetProcAddress(debug, MAKEINTRESOURCEA(50)));
                Fatal_f = reinterpret_cast<Fatalf>(GetProcAddress(debug, MAKEINTRESOURCEA(31)));
            }
            menumidhookjmp = base + 0x72555;
            JmpPatch(reinterpret_cast<BYTE*>(base + 0x7254F), (DWORD)MenuMidHook, 6);

            jmpback_midcfgload = (base + 0x1D37D);
            JmpPatch(reinterpret_cast<BYTE*>(base + 0x1D36B), (DWORD)MidCfgLoad, 8);

            jmpback_midoptions = (base + 0x1D85A);
            JmpPatch(reinterpret_cast<BYTE*>(base + 0x1D854), (DWORD)MidOptionsSetup, 6);

            CreateThread(0, 0, MainThread, hModule, 0, 0);
        }
        return ret;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

