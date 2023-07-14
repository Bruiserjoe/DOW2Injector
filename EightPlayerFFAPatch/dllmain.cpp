// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"
int lobby_slots = 0;
int l_lobby = 0;
DWORD base;
DWORD slots_addr;

//https://guidedhacking.com/threads/how-to-hook-thiscall-function-__thiscall-calling-convention.8542/


void MemPatch(BYTE* dst, BYTE* src, size_t size) {
    DWORD prot;
    VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &prot);
    std::memcpy(dst, src, size);
    VirtualProtect(dst, size, prot, &prot);
}

void NopPatch(BYTE* dst, size_t size) {
    DWORD prot;
    VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &prot);
    std::memset(dst, 0x90, size);
    VirtualProtect(dst, size, prot, &prot);
}



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

//ebp stays the same before and after call
//just rewrite the whole function in here and maybe we figure it out
typedef void(__stdcall *PopulatePlayerList)(void* tis);
PopulatePlayerList pop_org = nullptr;
void __stdcall PopulateDetour(void* tis) {
    /*void* t = reinterpret_cast<void*>(base + 0xb127db);
    __asm {
        mov eax, t;
        call SetCxxFrameHandle;
        mov eax, 0x168;
        //call alloca probe here
        push ebx;
        push esi;
        push edi;
        lea eax, [ebp + -0x34];
        mov t, eax;
    }
    set_data(t);
    int ebp_4 = 0;

    __asm {
        pop edi;
        pop esi;
        pop ebx;
    }*/
    /*
    void* eb;
    __asm {
        push eax;
        mov eax, ebp;
        mov eb, eax;
        pop eax;
    }

    int* t = (int*)eb;

    int* t1 = t + -0x90;
    int* t2 = t + -0x94;
    */
    /*int* eb;
    __asm {
        push eax;
        mov eax, dword ptr[ebp + 0xfffffe98];
        mov eb, eax;
        pop eax;
    }
    int val = eb[2];*/

    lobby_slots = *((DWORD*)slots_addr);
    /*BYTE* src;
    if (l_lobby != lobby_slots) {
        if (lobby_slots >= 8) {
            src = (BYTE*)"\xBE\x0A\x00\x00\x00\x90";
            MemPatch(reinterpret_cast<BYTE*>(base + 0x9146f), src, 6);

            //src = (BYTE*)"\x90\x90\x90\x90\x90\x90";
            //MemPatch(reinterpret_cast<BYTE*>(base + 0x2b16), src, 6);
        }
        else {
            src = (BYTE*)"\x8b\xb5\xa0\xfe\xff\xff";
            MemPatch(reinterpret_cast<BYTE*>(base + 0x9146f), src, 6);
        }
    }*/
    BYTE* src;
    if (l_lobby != lobby_slots) {
        
    }

    l_lobby = lobby_slots;
    pop_org(tis);
    if (lobby_slots >= 8) {
        //src = (BYTE*)"\xff\x15\x34\x70\xf8\x00";
        //MemPatch(reinterpret_cast<BYTE*>(base + 0x2b16), src, 6);
    }
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
        //MemPatch(reinterpret_cast<BYTE*>(base + 0x76c137), src, 3);
        //src = (BYTE*)"\xc7\x44\x24\x38\x4\x0\x0\x0";
        src = (BYTE*)"\x83\xff\x10";
        //MemPatch(reinterpret_cast<BYTE*>(base + 0x47cd7), src, 3);
        src = (BYTE*)"\x83\xbe\x98\x0\x0\x0\x8";
        //MemPatch(reinterpret_cast<BYTE*>(base + 0x47c47), src, 7);
        src = (BYTE*)"\x83\xff\x10";
        //MemPatch(reinterpret_cast<BYTE*>(base + 0x47da3), src, 3);
        //this mempatches which map list is supposed to be used, could be useful in other things
        src = (BYTE*)"\x80\x7b\x5c\x1";
        //MemPatch(reinterpret_cast<BYTE*>(base + 0x86e3c), src, 4);
        src = (BYTE*)"\xc7\x45\xb4\x8\x0\x0\x0";
        //MemPatch(reinterpret_cast<BYTE*>(base + 0x9148c), src, 8);
        src = (BYTE*)"\xc7\x45\xec\x8\x0\0\0";
        //MemPatch(reinterpret_cast<BYTE*>(base + 0x915ab), src, 7);

        src = (BYTE*)"\xc7\x45\xb4\x08\x00\x00\x00";
        MemPatch(reinterpret_cast<BYTE*>(base + 0x9148c), src, 7);
        src = (BYTE*)"\x83\xbe\x98\x00\x00\x00\x08";
        MemPatch(reinterpret_cast<BYTE*>(base + 0x47c47), src, 7);

        src = (BYTE*)"\x6a\x02";
        //MemPatch(reinterpret_cast<BYTE*>(base + 0x47ccf), src, 2);

        src = (BYTE*)"\x83\xf8\x08";
        //MemPatch(reinterpret_cast<BYTE*>(base + 0x76c137), src, 3);

        src = (BYTE*)"\x6a\x08";
        //MemPatch(reinterpret_cast<BYTE*>(base + 0x87fb6), src, 2);

        //src = (BYTE*)"\xc7\x44\x24\x20\x08\x00\x00\x00";
        //MemPatch(reinterpret_cast<BYTE*>(base + 0x66bee), src, 8);
        //src = (BYTE*)"\xc6\x44\x24\x13\x01";
        //MemPatch(reinterpret_cast<BYTE*>(base + 0x76bfcc), src, 5);
        //src = (BYTE*)"\x83\xf8\x08";
        //MemPatch(reinterpret_cast<BYTE*>(base + 0x76c137), src, 3);
        //NopPatch(reinterpret_cast<BYTE*>(base + 0x91689), 6);


        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach((void**)&teamset_org, TeamSetupDetour);
        //DetourAttach((void**)&teams_org, TeamsDetour);
        //DetourAttach((void**)&pos1_org, Pos1Detour);
        DetourAttach((void**)&pop_org, PopulateDetour);
        //DetourAttach((void**)&maxframe, MaxFrameDetour);
        //DetourAttach((void**)&plist_org, PassListDetour);
        //DetourAttach((void**)&ffa_org, FFAUIDetour);
        DetourTransactionCommit();
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

