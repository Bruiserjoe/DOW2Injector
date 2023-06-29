// dllmain.cpp : Defines the entry point for the DLL application.
#include "dllmain.h"


typedef void(__thiscall *setGamemode)(void* ecx, DWORD32 param2);
setGamemode setgame_target = reinterpret_cast<setGamemode>(0x004882c6); //function before hook


//http://jbremer.org/x86-api-hooking-demystified/
//https://www.x86matthew.com/view_post?id=stealth_hook

//https://www.unknowncheats.me/forum/c-and-c-/154364-detourfunction-__thiscall.html
//https://www.unknowncheats.me/forum/programming-for-beginners/424330-hook-functions.html
//https://guidedhacking.com/threads/introduction-to-calling-conventions-for-beginners.20041/
// 
//is a member function of a class so we use fastcall to work around having to use thiscall
void __fastcall setgamemodedetour(void* ecx, DWORD32 param2) {
    //try both eax and ebx

    char* stor;
    __asm mov [stor], ebx;

    if (ecx == (void*)1) {
        *(stor + 0x5b) = 1;
        *(stor + 0x5c) = 0;
    }
    else if (ecx == (void*)2) {
        *(stor + 0x5b) = 0;
        *(stor + 0x5c) = 1;
    }
    else if (ecx == (void*)4) {
        *(stor + 0x5b) = 1;
        *(stor + 0x5c) = 0;
    }
    else if (ecx == (void*)5) {
        *(stor + 0x5b) = 0;
        *(stor + 0x5c) = 1;
    }
    else {
        *(stor + 0x5b) = 0; //ffa 
        *(stor + 0x5c) = 0; //team ffa
    }

    DWORD32* dw = (DWORD32*)(stor + 0x50);
    *dw = param2;
    dw = (DWORD32*)(stor + 0x54);
    *dw = (DWORD32)ecx;

    return;
    //return setgame_target(ecx, param2);
}

//find where the strings for the gamemodes are stored so we can utilize them
//004882c6


/*typedef void(__fastcall* CullRetrieval)(int* param1);
CullRetrieval org_retcull = reinterpret_cast<CullRetrieval>(0x007513b0);

PDETOUR_TRAMPOLINE trampoline = nullptr; //final address of trampoline
CullRetrieval real_target = nullptr; //final address of target
CullRetrieval real_detour = nullptr; //final address of detour

HMODULE util;
typedef void(__thiscall* Set)(void* ecx, char const* param1, float param2);
Set f_set = nullptr;

typedef float(__thiscall* Retrieve)(void* ecx, char const* param1);
Retrieve f_ret = nullptr;

typedef void* (__fastcall* GetResourceData)(void* ecx);
GetResourceData gr_data = nullptr;
//keep * on the right to the function name or else get weird errors
//now set the cull area scale everytime this detour is called
void __fastcall culldetour(int* ecx) {
    int* tis = ecx + 0x26; //use this to get the resource data from util
    char* r_data = (char*)gr_data(tis); //getting the resource data structure
    //float scale = f_ret((void*)(r_data + 0x78), "cull_area_scale");
    //std::string str = "Scale: " + std::to_string(scale);
    //setting the value of cull_area_scale in the keyvaluecontainer
    f_set((void*)(r_data + 0x78), "cull_area_scale", 1000.0f); //this was causing the crash
    org_retcull(ecx); //calling original function
    //again for safety
    f_set((void*)(r_data + 0x78), "cull_area_scale", 1000.0f); //this was causing the crash

}
*/
HMODULE test;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    DetourRestoreAfterWith();
    DetourIsHelperProcess();
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        //test = GetModuleHandleA("DOW2.exe");
        //setgame_target = reinterpret_cast<setGamemode>(test + 0x882c6);
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach((void**)&setgame_target, setgamemodedetour);
        DetourTransactionCommit();
        
        //attaching the hook
       /* DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttachEx((void**)&org_retcull, culldetour, &trampoline, (void**)&real_target, (void**)&real_detour);
        DetourTransactionCommit();

        util = GetModuleHandleA("Util.dll");
        if (util) {
            //https://stackoverflow.com/questions/3598108/how-can-i-call-a-exported-function-using-ordinal-number
            //getting location of functions using their ordinals which I got from decomping the DLL
            gr_data = reinterpret_cast<GetResourceData>(GetProcAddress(util, MAKEINTRESOURCEA(446)));
            f_set = reinterpret_cast<Set>(GetProcAddress(util, MAKEINTRESOURCEA(676)));
            f_ret = reinterpret_cast<Retrieve>(GetProcAddress(util, MAKEINTRESOURCEA(662)));
        }*/

        //MessageBoxA(NULL, "DLL Injected", "DLL injected", MB_OK);
        //CreateThread(0, 0, MainThread, hModule, 0, 0);
        break;
    case DLL_PROCESS_DETACH:
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach((void**)&setgame_target, setgamemodedetour);
        DetourTransactionCommit();

        /*DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach((void**)&org_retcull, culldetour);
        DetourTransactionCommit();
        */
        break;
    }


    return TRUE;
}

