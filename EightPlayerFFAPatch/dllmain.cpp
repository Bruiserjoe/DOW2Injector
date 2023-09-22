// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"
int lobby_slots = 0;
int l_lobby = 0;
DWORD base;
DWORD slots_addr;

//https://guidedhacking.com/threads/how-to-hook-thiscall-function-__thiscall-calling-convention.8542/

//fix replays somehow


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
    //param4 is teams requested by front end
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

DWORD jmpback_midobserv;
void __declspec(naked) MidObserv() {
    __asm {
        pop esi;
        pop ebx;
        jmp[jmpback_midobserv];
    }
}


//ebp stays the same before and after call
//just rewrite the whole function in here and maybe we figure it out
typedef void(__stdcall *PopulatePlayerList)(void* tis);
PopulatePlayerList pop_org = nullptr;
void __stdcall PopulateDetour(void* tis) {
    lobby_slots = *((DWORD*)slots_addr);
    BYTE* src;
    if (l_lobby != lobby_slots) {
        if (lobby_slots < 8) {
            src = (BYTE*)"\xc7\x45\xb4\x06\x00\x00\x00";
            MemPatch(reinterpret_cast<BYTE*>(base + 0x9148c), src, 7);
        }
        else {
            src = (BYTE*)"\xc7\x45\xb4\x08\x00\x00\x00";
            MemPatch(reinterpret_cast<BYTE*>(base + 0x9148c), src, 7);
        }
    }

    l_lobby = lobby_slots;
    pop_org(tis);
}
//hooking GenerateSlotDropDowns
typedef void(__stdcall *GenerateSlotDropdown)(int a1, int a2);
GenerateSlotDropdown slotdrop_org = nullptr;
void __stdcall SlotDropDetour(int a1, int a2) {
    BYTE* src;
    int slots = *((DWORD*)slots_addr);
    if (slots < 8) {
        src = (BYTE*)"\x80\x7D\x0C\x00\x74\x12\x80\x7D\x10\x00\x75\x0C";
        MemPatch(reinterpret_cast<BYTE*>(base + 0x921D0), src, 12);
        src = (BYTE*)"\xE8\xD7\x00\x00\x00";
        MemPatch(reinterpret_cast<BYTE*>(base + 0x91EE6), src, 5);
    }
    else {
        //00491EA2 - regular slot
        //00491EC2 - closed slot
        jmpback_midobserv = (base + 0x91EC2);
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x91EE6), (DWORD)MidObserv, 5);
        NopPatch(reinterpret_cast<BYTE*>(base + 0x921D0), 12);

    }
    slotdrop_org(a1, a2);
}



//https://defuse.ca/online-x86-assembler.htm#disassembly
//https://shell-storm.org/x86doc/

//look into UpdateLobbyOnClick

DWORD jmpbackaddr = 0;
DWORD32 team_num = 0;

typedef void(__stdcall *N1)(size_t param1, int* param2, size_t param3, int *param4);
N1 funez = nullptr;

void __declspec(naked) MidTeamSetupDetour() {
    __asm {
        call funez;
        push ebx;
        mov ebx, dword ptr[esp + 0x244];
        mov team_num, ebx;
        pop ebx;
        jmp[jmpbackaddr];
    }
}


//try MapPreferencesPanel::invokecreatemaplist and MultiplayerLobbyMenuUpdate
BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        base = (DWORD)GetModuleHandleA("DOW2.exe");
        //functions for detouring
        teamset_org = reinterpret_cast<TeamSetup>(base + 0x39df40);
        pop_org = reinterpret_cast<PopulatePlayerList>(base + 0x911c4);
        slotdrop_org = reinterpret_cast<GenerateSlotDropdown>(base + 0x91D51);


        slots_addr = FindDMAAddy(base + 0x00F35A78, { 0x17C });

        funez = reinterpret_cast<N1>(base + 0x39dc90);
        jmpbackaddr = (base + 0x39e26a);
        JmpPatch(reinterpret_cast<BYTE*>((base + 0x39e265)), (DWORD)MidTeamSetupDetour, 5);

        
      



        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach((void**)&teamset_org, TeamSetupDetour);
        DetourAttach((void**)&pop_org, PopulateDetour);
        DetourAttach((void**)&slotdrop_org, SlotDropDetour);
        DetourTransactionCommit();
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

