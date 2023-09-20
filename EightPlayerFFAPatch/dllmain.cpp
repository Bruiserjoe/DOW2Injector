// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"
int lobby_slots = 0;
int l_lobby = 0;
DWORD base;
DWORD slots_addr;

//https://guidedhacking.com/threads/how-to-hook-thiscall-function-__thiscall-calling-convention.8542/

//fix the crash on changing to 6p map from full 8p
//fix dropdown on 8p ffa
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


typedef void(__stdcall *LobbyTeamsSet)(int param1, char param2);
LobbyTeamsSet teams_org = nullptr;
void __stdcall TeamsDetour(int param1, char param2) {
    teams_org(param1, param2);
}

typedef void** (__stdcall *MultiLobbyUpdate)(void* tis);
MultiLobbyUpdate pos1_org = nullptr;
void** __stdcall Pos1Detour(void* tis) {
    
    void** out = pos1_org(tis);
    return out;
}
//typedefs for all the functions we have to call in Populate
typedef void(__fastcall *SetCxxFHandle)(void* t, void* t2);
SetCxxFHandle SetCxxFrameHandle = nullptr;

typedef int* (__stdcall *SetupData)(void* t);
SetupData set_data = nullptr;

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
            src = (BYTE*)"\x80\x7D\x0C\x00\x74\x12\x80\x7D\x10\x00\x75\x0C";
            MemPatch(reinterpret_cast<BYTE*>(base + 0x921D0), src, 12);
            src = (BYTE*)"\xE8\xD7\x00\x00\x00";
            MemPatch(reinterpret_cast<BYTE*>(base + 0x91EE6), src, 5);
        }
        else {
            src = (BYTE*)"\xc7\x45\xb4\x08\x00\x00\x00";
            MemPatch(reinterpret_cast<BYTE*>(base + 0x9148c), src, 7);
            //00491EA2 - regular slot
            //00491EC2 - closed slot
            jmpback_midobserv = (base + 0x91EC2);
            JmpPatch(reinterpret_cast<BYTE*>(base + 0x91EE6), (DWORD)MidObserv, 5);
            NopPatch(reinterpret_cast<BYTE*>(base + 0x921D0), 12);

        }
    }

    l_lobby = lobby_slots;
    pop_org(tis);
}

typedef void(__stdcall *GetMaxFrameTime)(size_t player_count);
GetMaxFrameTime maxframe = nullptr;
void __stdcall MaxFrameDetour(size_t player_count) {
    maxframe(player_count);
}
std::ofstream file;

typedef void(__stdcall *PassListGFX)(DWORD32 param1);
PassListGFX plist_org = nullptr;
void __stdcall PassListDetour(DWORD32 param1) {
    //std::string str = "F: " + std::to_string(param1) + "\n";
    //file << str;
    plist_org(param1);
}

typedef void(__fastcall *FFAUIInit)(int param1);
FFAUIInit ffa_org = nullptr;
void __fastcall FFAUIDetour(int param1, void* unu) {
    ffa_org(param1);
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
    BYTE* src;
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        file.open("mod_logs\\patch.log");
        base = (DWORD)GetModuleHandleA("DOW2.exe");
        //functions for detouring
        teamset_org = reinterpret_cast<TeamSetup>(base + 0x39df40);
        teams_org = reinterpret_cast<LobbyTeamsSet>(base + 0x47b9a);
        pos1_org = reinterpret_cast<MultiLobbyUpdate>(base + 0x86e0d);
        pop_org = reinterpret_cast<PopulatePlayerList>(base + 0x911c4);
        maxframe = reinterpret_cast<GetMaxFrameTime>(base + 0x3df9e);
        plist_org = reinterpret_cast<PassListGFX>(base + 0x63ca0);
        ffa_org = reinterpret_cast<FFAUIInit>(base + 0x76bf70);

        //functions we need to be able to call
        SetCxxFrameHandle = reinterpret_cast<SetCxxFHandle>(base + 0x9aaed4);
        set_data = reinterpret_cast<SetupData>(base + 0x59004);

        slots_addr = FindDMAAddy(base + 0x00F35A78, { 0x17C });

        //only needed ones
        src = (BYTE*)"\xc7\x45\xb4\x08\x00\x00\x00";
        MemPatch(reinterpret_cast<BYTE*>(base + 0x9148c), src, 7);

        funez = reinterpret_cast<N1>(base + 0x39dc90);
        jmpbackaddr = (base + 0x39e26a);
        JmpPatch(reinterpret_cast<BYTE*>((base + 0x39e265)), (DWORD)MidTeamSetupDetour, 5);

        
      



        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach((void**)&teamset_org, TeamSetupDetour);
        DetourAttach((void**)&pop_org, PopulateDetour);
        DetourTransactionCommit();
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

