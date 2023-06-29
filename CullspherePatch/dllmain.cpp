// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"

void error(const char* err_title, const char* err_message) {
    MessageBoxA(0, err_message, err_title, 0);
    //exit(-1);
}

typedef void(__fastcall *CullRetrieval)(int* param1);
CullRetrieval org_retcull = reinterpret_cast<CullRetrieval>(0x007513b0);

PDETOUR_TRAMPOLINE trampoline = nullptr; //final address of trampoline
CullRetrieval real_target = nullptr; //final address of target
CullRetrieval real_detour = nullptr; //final address of detour

HMODULE util;
typedef void(__thiscall *Set)(void* ecx, char const* param1, float param2);
Set f_set = nullptr;

typedef float(__thiscall *Retrieve)(void* ecx, char const* param1);
Retrieve f_ret = nullptr;

typedef void* (__fastcall *GetResourceData)(void* ecx);
GetResourceData gr_data = nullptr;
//keep * on the right to the function name or else get weird errors
//now set the cull area scale everytime this detour is called
void __fastcall culldetour(int* ecx) {
    int* tis = ecx + 0x26; //use this to get the resource data from util
    char* r_data = (char*)gr_data(tis);
    float scale = f_ret((void*)(r_data + 0x78), "cull_area_scale");
    std::string str = "Scale: " + std::to_string(scale);
    //error("Scale", str.c_str());
    f_set((void*)(r_data + 0x78), "cull_area_scale", 1000.0f); //this was causing the crash

    org_retcull(ecx);
    //reading value of eax into pointer so we can access value of cullsphere
    //char* stor;
    //__asm mov[stor], ebx;
    //float* cl = (float*)(stor + 0x34);
    //*cl = 0.01f;
    f_set((void*)(r_data + 0x78), "cull_area_scale", 1000.0f); //this was causing the crash


   


    //std::string str = "Cull: " + std::to_string(*cl);
    //error("Works", str.c_str());
    //std::string str = "Address: " + *stor;
    //error(str.c_str(), "Works");
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

    DetourRestoreAfterWith();
    DetourIsHelperProcess();
    
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        util = GetModuleHandleA("Util.dll");
        if (util) {
            //https://stackoverflow.com/questions/3598108/how-can-i-call-a-exported-function-using-ordinal-number
            //gr_data = reinterpret_cast<GetResourceData>(DetourFindFunction("Util.dll", "Inv::RefBase::GetResourceData"));
            gr_data = reinterpret_cast<GetResourceData>(GetProcAddress(util, MAKEINTRESOURCEA(446)));
            f_set = reinterpret_cast<Set>(GetProcAddress(util, MAKEINTRESOURCEA(676)));
            f_ret = reinterpret_cast<Retrieve>(GetProcAddress(util, MAKEINTRESOURCEA(662)));
            //f_set = reinterpret_cast<Set>(util + 0x11c30);
        }
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        //DetourAttach((void**)&org_retcull, culldetour);
        DetourAttachEx((void**)&org_retcull, culldetour, &trampoline, (void**)&real_target, (void**)&real_detour);
        DetourTransactionCommit();
        

        break;
    case DLL_PROCESS_DETACH:
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach((void**)&org_retcull, culldetour);
        DetourTransactionCommit();
        break;
    }
    return TRUE;
}
