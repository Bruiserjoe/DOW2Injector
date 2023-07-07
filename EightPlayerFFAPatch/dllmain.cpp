// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"
int lobby_slots = 0;
DWORD base;
DWORD slots_addr;

//https://guidedhacking.com/threads/how-to-hook-thiscall-function-__thiscall-calling-convention.8542/





uintptr_t FindDMAAddy(uintptr_t ptr, std::vector<unsigned int> offsets)
{
    uintptr_t addr = ptr;
    for (unsigned int i = 0; i < offsets.size(); ++i)
    {
        addr = *(uintptr_t*)addr;
        addr += offsets[i];
    }
    return addr;
}



//https://stackoverflow.com/questions/45262003/pointer-from-cheat-engine-to-c
typedef size_t(__stdcall* TeamSetup)(int* param1, ULONG param2, void* param3, void** param4, DWORD32* ed, DWORD32* es);
TeamSetup teamset_org = nullptr;
size_t __stdcall TeamSetupDetour(int* param1, ULONG param2, void* param3, void** param4, DWORD32* ed, DWORD32* es) {
    //DWORD b = (DWORD)GetModuleHandleA(NULL);
    //param4 the teams request by the map
    //DWORD addr = FindDMAAddy(b + 0x00F35A78, { 0x17C });
    lobby_slots = *((DWORD*)slots_addr);
    size_t param_2 = *(param1 - 3);

    size_t teams = 2;
    int val = param1[1]; //player count
    if ((param_2 & 0x600) == 0) {
        if ((param_2 & 0x180) != 0) {
            teams = lobby_slots;
        }
    }
    else {
        teams = lobby_slots / 2;
    }
    param4 = (void**)teams;

    size_t out = teamset_org(param1, param2, param3, param4, ed, es);
    return out;
}

typedef DWORD32* (__stdcall* LobbyChangeSlotCount)(DWORD32* param1, DWORD32* param2);
LobbyChangeSlotCount slot_org = nullptr;
//find pointer to actual value since this 
DWORD32* __stdcall LobbySlotDetour(DWORD32* param1, DWORD32* param2) {
    DWORD32* out = slot_org(param1, param2);
    lobby_slots = out[0];
    return out;
}




BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        base = (DWORD)GetModuleHandleA("DOW2.exe");
        teamset_org = reinterpret_cast<TeamSetup>(base + 0x39df40);
        slot_org = reinterpret_cast<LobbyChangeSlotCount>(base + 0x7a2b80);
        slots_addr = FindDMAAddy(base + 0x00F35A78, { 0x17C });
       
       
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach((void**)&teamset_org, TeamSetupDetour);
        DetourTransactionCommit();
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

