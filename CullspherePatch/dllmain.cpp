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
DWORD cull_callback_slot = 0;
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
        mov ebp, dword ptr [cull_callback_slot];
        mov ebp, dword ptr [ebp];
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

#ifdef DOW2_SIGSCAN
    static size_t GetModuleImageSize(HMODULE module) {
        if (!module) {
            return 0;
        }
        const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(module);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
            return 0;
        }
        const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(
            reinterpret_cast<const BYTE*>(module) + dos->e_lfanew);
        return nt->Signature == IMAGE_NT_SIGNATURE ? nt->OptionalHeader.SizeOfImage : 0;
    }

    static bool ScanUnique(
        BYTE* module,
        size_t module_size,
        const char* pattern,
        const char* mask,
        DWORD* address) {
        const SigScanResult first = SigScan(module, module_size, pattern, mask);
        if (!first.found) {
            return false;
        }
        const size_t next_offset = first.address - reinterpret_cast<DWORD>(module) + 1;
        if (next_offset < module_size) {
            const SigScanResult second =
                SigScan(module + next_offset, module_size - next_offset, pattern, mask);
            if (second.found) {
                return false;
            }
        }
        *address = first.address;
        return true;
    }

    static bool ResolveAddresses() {
        BYTE* module = reinterpret_cast<BYTE*>(base);
        const size_t module_size = GetModuleImageSize(reinterpret_cast<HMODULE>(base));
        DWORD cull_patch = 0;
        DWORD camera_patch = 0;

        static const char cull_pattern[] =
            "\x51\x53\x55\x8B\x2D\x00\x00\x00\x00\x56\x8B\xF1\x57\x8D\xBE"
            "\x98\x00\x00\x00\x8B\xCF\xFF\xD5\x8B\x1D\x00\x00\x00\x00\x68"
            "\x00\x00\x00\x00";
        static const char cull_mask[] = "xxxxx????xxxxxxxxxxxxxxxx????x????";
        static const char camera_pattern[] =
            "\x53\x8B\x5D\x08\x56\x57\x51\x8D\x7C\x24\x1C\xD9\x1C\x24\x8B"
            "\xF3\xE8\x00\x00\x00\x00\xD9\x45\x0C";
        static const char camera_mask[] = "xxxxxxxxxxxxxxxxx????xxx";

        if (!module_size ||
            !ScanUnique(module, module_size, cull_pattern, cull_mask, &cull_patch) ||
            !ScanUnique(module, module_size, camera_pattern, camera_mask, &camera_patch)) {
            return false;
        }
        if (memcmp(reinterpret_cast<void*>(cull_patch), "\x51\x53\x55\x8B\x2D", 5) != 0 ||
            memcmp(reinterpret_cast<void*>(camera_patch), "\x53\x8B\x5D\x08\x56", 5) != 0) {
            return false;
        }

        cull_callback_slot = *reinterpret_cast<DWORD*>(cull_patch + 5);
        if (cull_callback_slot < base || cull_callback_slot + sizeof(DWORD) > base + module_size) {
            return false;
        }
        cull_jmp_back = cull_patch + 9;
        camera_jmp_back = camera_patch + 5;
        return true;
    }
#endif

static BOOL FailInitialization(const char* message) {
    if (Fatal_f) {
        Fatal_f("%s", message);
    }
    else {
        MessageBoxA(nullptr, message, "CullspherePatch", MB_OK | MB_ICONERROR);
    }
    return FALSE;
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
        if (!base || !plat_getoption) {
            return FailInitialization("Cullsphere Patch: required game modules or exports were not found.");
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

#ifdef DOW2_SIGSCAN
        if (!ResolveAddresses()) {
            return FailInitialization("Cullsphere Patch: required DOW2.exe signatures were not found uniquely.");
        }
#else
        cull_callback_slot = 0x00F89390;
        cull_jmp_back = base + 0x3513B9;
        camera_jmp_back = base + 0x7ACC39;
#endif

        if (gr_data && f_set) {
#ifdef DOW2_SIGSCAN
            if (!JmpPatch(reinterpret_cast<BYTE*>(cull_jmp_back - 9), (DWORD)cullJmpPatch, 9)) {
                return FailInitialization("Cullsphere Patch: failed to install culling hook.");
            }
#else
            if (!JmpPatch((BYTE*)(base + 0x3513B0), (DWORD)cullJmpPatch, 9)) {
                return FailInitialization("Cullsphere Patch: failed to install culling hook.");
            }
#endif
        }
        else {
            return FailInitialization("Cullsphere Patch: required Util.dll exports were not found.");
        }

#ifdef DOW2_SIGSCAN
        if (!JmpPatch(reinterpret_cast<BYTE*>(camera_jmp_back - 5), (DWORD)cameraDrawJmpPatch, 5)) {
            return FailInitialization("Cullsphere Patch: failed to install camera hook.");
        }
#else
        if (!JmpPatch((BYTE*)(base + 0x7ACC34), (DWORD)cameraDrawJmpPatch, 5)) {
            return FailInitialization("Cullsphere Patch: failed to install camera hook.");
        }
#endif

        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
