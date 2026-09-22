// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"
DWORD base;
HMODULE plat;
HMODULE debug;
Config cfg;
typedef bool(__stdcall* PlatGetOption)(const char* option, char* str, unsigned int size);
PlatGetOption plat_getoption = nullptr;

float camera_distance = 47.0f;

//util.dll functions
HMODULE util;
typedef void(__thiscall *Set)(void* ecx, char const* param1, float param2);
Set f_set = nullptr;

typedef float(__thiscall *Retrieve)(void* ecx, char const* param1);
Retrieve f_ret = nullptr;

typedef void* (__fastcall *GetResourceData)(void* ecx);
GetResourceData gr_data = nullptr;

Timestampedf Timestampedtracef;
Fatalf Fatal_f;

DWORD cull_jmp_back = 0;
float cull_rate = 0.0037f;
float cull_max = 800.0f;
float float_half = 0.5f;
float float_one = 1.0f;
float float_thirty = 30.0f;
char cull_area_scale_name[] = "cull_area_scale";

void __declspec(naked) setCullScale() {
    __asm {
        pushfd;
        pushad;
        sub esp, 4;

        // int* arithmetic in the old detour made 0x26 equal to 0x98 bytes.
        mov ecx, [esp + 28];
        add ecx, 0x98;
        call dword ptr [gr_data];
        mov edi, eax;

        fld dword ptr [camera_distance];
        fcomp dword ptr [cull_max];
        fnstsw ax;
        sahf;
        jp use_minimum;
        jae use_minimum;

        // (sinf(camera_distance * cull_rate - 0.5f) + 1.0f) * 30.0f
        fld dword ptr [camera_distance];
        fmul dword ptr [cull_rate];
        fsub dword ptr [float_half];
        fsin;
        fadd dword ptr [float_one];
        fmul dword ptr [float_thirty];
        fstp dword ptr [esp];
        jmp apply_scale;

    use_minimum:
        fld dword ptr [float_half];
        fstp dword ptr [esp];

    apply_scale:
        push dword ptr [esp];
        push offset cull_area_scale_name;
        lea ecx, [edi + 0x78];
        call dword ptr [f_set];

        add esp, 4;
        popad;
        popfd;
        ret;
    }
}

void __declspec(naked) cullJmpPatch() {
    __asm {
        call setCullScale;

        // Make the original function return through our post-call patch.
        push ecx;
        push offset after_cull;

        // Original bytes at DOW2.exe+0x3513B0.
        push ecx;
        push ebx;
        push ebp;
        mov ebp, dword ptr ds:[0x00F89390];
        jmp dword ptr [cull_jmp_back];

    after_cull:
        pop ecx;
        call setCullScale;
        ret;
    }
}

//found the camera draw function
//00bacc10
//ebx should contain the camera struct, just gotta extract the camera distance from it

typedef void(__stdcall *CameraDraw)(int param1, float param2);
CameraDraw cm_draw = reinterpret_cast<CameraDraw>(0x00bacc10); //original camera draw function

void __stdcall cameradrawdetour(int param1, float param2) {
    cm_draw(param1, param2);
    float* cm = (float*)(int*)(param1 + 0x300);
    //std::string str = "Distance: " + std::to_string(*cm);
    //error("Camera", str.c_str());
    camera_distance = *cm;
}

DWORD camera_jmp_back = 0;
void __declspec(naked) cameraDrawJmpPatch() {
    __asm {
        push ecx;
        mov ecx, [ebp + 0x8];
        add ecx, 0x300;
		mov ecx, [ecx];
        mov [camera_distance], ecx;
        pop ecx;
    }
    __asm {
        push ebx;
        mov ebx, [ebp + 0x8];
        push esi;
        jmp[camera_jmp_back];
    }
}

//base
//10000000
//target set function in Util.dll
//10011c30
//resource data
//10028340
//https://reverseengineering.stackexchange.com/questions/18676/how-can-i-access-an-internal-dll-function-or-piece-of-data-externally

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{

    //DetourRestoreAfterWith();
    //DetourIsHelperProcess();
    bool ret = false;
    char mod1[0x200];
    std::string modu;
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        
        //getting the module
        base = (DWORD)GetModuleHandleA("DOW2.exe");

        plat = GetModuleHandleA("Platform.dll");
        if (plat) {
            plat_getoption = reinterpret_cast<PlatGetOption>(GetProcAddress(plat, MAKEINTRESOURCEA(78)));
        }
        debug = GetModuleHandleA("Debug.dll");
        if (debug) {
            Timestampedtracef = reinterpret_cast<Timestampedf>(GetProcAddress(debug, MAKEINTRESOURCEA(50)));
            Fatal_f = reinterpret_cast<Fatalf>(GetProcAddress(debug, MAKEINTRESOURCEA(31)));
        }
        //getting the module name
        ret = plat_getoption("modname", mod1, 0x200);
        modu = std::string(mod1);
        modu = modu + ".cullsphere";
        cfg = Config(modu);
        cull_rate = cfg.getRate();
        cull_max = cfg.getMax();

        util = GetModuleHandleA("Util.dll");
        if (util) {
            //https://stackoverflow.com/questions/3598108/how-can-i-call-a-exported-function-using-ordinal-number
            //getting location of functions using their ordinals which I got from decomping the DLL
            gr_data = reinterpret_cast<GetResourceData>(GetProcAddress(util, MAKEINTRESOURCEA(446)));
            f_set = reinterpret_cast<Set>(GetProcAddress(util, MAKEINTRESOURCEA(676)));
            f_ret = reinterpret_cast<Retrieve>(GetProcAddress(util, MAKEINTRESOURCEA(662)));
        }

        if (gr_data && f_set) {
            cull_jmp_back = base + 0x3513B9;
            JmpPatch((BYTE*)(base + 0x3513B0), (DWORD)cullJmpPatch, 9);
        }

        camera_jmp_back = base + 0x7ACC39;
        JmpPatch((BYTE*)(base + 0x7ACC34), (DWORD)cameraDrawJmpPatch, 5);

        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
