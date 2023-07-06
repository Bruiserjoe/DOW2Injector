// dllmain.cpp : Defines the entry point for the DLL application.
#include "dllmain.h"

//returns a pointer of some kind
typedef DWORD32*(__thiscall *setGamemode)(DWORD32 index, DWORD32* address, char* a_struct);
setGamemode setgame_target = reinterpret_cast<setGamemode>(0x004882c6); //function before hook

GamemodeMap map;

//Exception thrown at 0x00488164 in DOW2.exe: 0xC0000005: Access violation reading location 0x3E70F050.


//http://jbremer.org/x86-api-hooking-demystified/
//https://www.x86matthew.com/view_post?id=stealth_hook

//https://www.unknowncheats.me/forum/c-and-c-/154364-detourfunction-__thiscall.html
//https://www.unknowncheats.me/forum/programming-for-beginners/424330-hook-functions.html
//https://guidedhacking.com/threads/introduction-to-calling-conventions-for-beginners.20041/
// https://www.tripwire.com/state-of-security/ghidra-101-creating-structures-in-ghidra
// 
// 
// 

char g_ffa = 0;
char g_tffa = 0;
int lobby_slots = 0;
DWORD base;

//is a member function of a class so we use fastcall to work around having to use thiscall
//this was crashing because the last parameter wasn't getting cleared because we didnt have it in parameters
DWORD32* __fastcall setgamemodedetour(DWORD32 index, DWORD32* address, char* a_struct) {
    //DWORD32* out = setgame_target(param2, in_ec);
    


    //std::ofstream file;
    //file.open("mod_logs\\gamemode.log");
    //try both eax and ebx
    //file << "Before assembly\n";
    Mode m = map.getMode(index);

    char ffa = m.ffa;
    char t_ffa = m.t_ffa;
    g_ffa = ffa;
    g_tffa = t_ffa;
    //getting access violation on eax access
    __asm {
        mov dl, [ffa];
        mov byte ptr[ebx + 0x5b], dl;
        mov dl, [t_ffa];
        mov byte ptr[ebx + 0x5c], dl;
        mov edx, [address];
        mov dword ptr[ebx + 0x50], edx;
        mov edx, [index];
        mov dword ptr[ebx + 0x54], edx;
    }
   
    
    //file << "Function finished\n";
    //file.close();
    return address;
}


typedef int** (__thiscall *LobbyChangeVals)(void* tis, int** param1, int* param2, int param3, size_t* param4);
LobbyChangeVals lb_org = nullptr;

int count = 2;
//https://guidedhacking.com/threads/how-to-hook-thiscall-function-__thiscall-calling-convention.8542/


int** __fastcall LobbyChangeValsDetour(void* tis, void* unused, int** param1, int* param2, int param3, size_t* param4) {
    int** out = lb_org(tis, param1, param2, param3, param4);
    int* v = (int*)tis;
    v = (v + 4) + 3;
    count = *v;
    return out;
}



DWORD FindDMAAddy(DWORD ptr, std::vector<unsigned int> offsets)
{
    DWORD addr = ptr;
    for (unsigned int i = 0; i < offsets.size(); ++i)
    {
        addr = *(DWORD*)addr;
        addr += offsets[i];
    }
    return addr;
}



//https://stackoverflow.com/questions/45262003/pointer-from-cheat-engine-to-c
typedef size_t(__stdcall *TeamSetup)(int* param1, ULONG param2, void* param3, void** param4, DWORD32* ed, DWORD32* es);
TeamSetup teamset_org = nullptr;
size_t __stdcall TeamSetupDetour(int* param1, ULONG param2, void* param3, void** param4, DWORD32* ed, DWORD32* es) {
    DWORD b = (DWORD)GetModuleHandleA(NULL);
    //param4 the teams request by the map
    DWORD addr = FindDMAAddy(b + 0x00F35A78, { 0x17C });
    lobby_slots = *((DWORD*)addr);
    
    
    size_t param_2 = *(param1 - 3);
    DWORD32 exp_teams;
    
    size_t teams = 2;
    int val = param1[1]; //player count
    if (g_ffa) {
        teams = lobby_slots;
    }
    else if (g_tffa) {
        //make this work for all player counts
        teams = lobby_slots / 2;
    }
    param4 = (void**)teams;

    size_t out = teamset_org(param1, param2, param3, param4, ed, es);
    __asm {
        //and esp, 0xfffffff8;
        //sub esp, 0x750;
        mov eax, dword ptr[esp + 0x8c];
        mov exp_teams, eax;
        //add esp, 0x750;
    }
    return out;
}

typedef DWORD32* (__stdcall *LobbyChangeSlotCount)(DWORD32* param1, DWORD32* param2);
LobbyChangeSlotCount slot_org = nullptr;
//find pointer to actual value since this 
DWORD32* __stdcall LobbySlotDetour(DWORD32* param1, DWORD32* param2) {
    DWORD32* out = slot_org(param1, param2);
    lobby_slots = out[0];
    return out;
}

/*
typedef DWORD32(__stdcall *ModAssignPlayers)(int* param1, size_t param2, size_t param3, size_t param4, void* param5, void* ed, void* es, void* eb);
//typedef DWORD32(__stdcall* ModAssignPlayers)(void* eb, void* es, void* ed, void* param5, size_t param4, size_t param3, size_t param2, int* param1);
ModAssignPlayers mod_org = nullptr;

DWORD32 __stdcall ModAssignDetour(int* param1, size_t param2, size_t param3, size_t param4, DWORD32* param5, void* ed, void* es, void* eb) {
    //param_2 = param2;
    //std::cout << param_2 << "\n";
    DWORD32 out = mod_org(param1, param2, param3, param4, param5, ed, es, eb); // this is crashing
    return out;
}

typedef void(__stdcall *CountTeams)(int* param1);
CountTeams ct_org = nullptr;
void __stdcall CountTeamsDetour(int* param1) {
    DWORD32* nt;
    __asm {
        mov [nt], edi;
    }

    ct_org(param1); //reading out of bounds of something
}
*/
//hook the function which edits the displayed gamemode value and save that string for use

typedef void(__stdcall *GamemodeChange)();
GamemodeChange gc = reinterpret_cast<GamemodeChange>(0x00486f6a);

//recreate locstring so we can get the locstring value from ebp like in possiblegamemodestring


BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    //DetourRestoreAfterWith();
    //DetourIsHelperProcess();
    std::ofstream f;
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        base = (DWORD)GetModuleHandleA("DOW2.exe");
        setgame_target = reinterpret_cast<setGamemode>(base + 0x882c6);
        //mod_org = reinterpret_cast<ModAssignPlayers>(base + 0x39e880);
        teamset_org = reinterpret_cast<TeamSetup>(base + 0x39df40);
        lb_org = reinterpret_cast<LobbyChangeVals>(base + 0x9ba210);
        slot_org = reinterpret_cast<LobbyChangeSlotCount>(base + 0x7a2b80);
        //ct_org = reinterpret_cast<CountTeams>(base + 0x39dc20);
        map.readConfig("mods\\gmd.cfg");
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach((void**)&setgame_target, setgamemodedetour);
        //DetourAttach((void**)&mod_org, ModAssignDetour);
        DetourAttach((void**)&teamset_org, TeamSetupDetour);
        //DetourAttach((void**)&lb_org, LobbyChangeValsDetour);
        DetourAttach((void**)&slot_org, LobbySlotDetour);
        //DetourAttach((void**)&ct_org, CountTeamsDetour);
        DetourTransactionCommit();
        

        //MessageBoxA(NULL, "DLL Injected", "DLL injected", MB_OK);
        //CreateThread(0, 0, MainThread, hModule, 0, 0);
        break;
    case DLL_PROCESS_DETACH:
        //f.open("mod_logs\\detachg.log");
        //f << "Detached gamemode\n";
        /*DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach((void**)&setgame_target, setgamemodedetour);
        DetourTransactionCommit();*/
        //file.close();
        break;
    }


    return TRUE;
}

