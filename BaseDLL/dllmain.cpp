// dllmain.cpp : Defines the entry point for the DLL application.
#include "dllmain.h"

int lobby_slots = 0;
DWORD base;
DWORD slots_addr;
char g_ffa = 0;
char g_tffa = 0;

MapLoader map_list;
size_t cur_index = 0;

//returns a pointer of some kind
typedef DWORD32*(__thiscall *setGamemode)(DWORD32 index, DWORD32* address, char* a_struct);
setGamemode setgame_target = reinterpret_cast<setGamemode>(0x004882c6); //function before hook

GamemodeMap map;

//Exception thrown at 0x00488164 in DOW2.exe: 0xC0000005: Access violation reading location 0x3E70F050.
//caused by wrong handling of parameters

//http://jbremer.org/x86-api-hooking-demystified/
//https://www.x86matthew.com/view_post?id=stealth_hook

//https://www.unknowncheats.me/forum/c-and-c-/154364-detourfunction-__thiscall.html
//https://www.unknowncheats.me/forum/programming-for-beginners/424330-hook-functions.html
//https://guidedhacking.com/threads/introduction-to-calling-conventions-for-beginners.20041/
// https://www.tripwire.com/state-of-security/ghidra-101-creating-structures-in-ghidra

//is a member function of a class so we use fastcall to work around having to use thiscall
//this was crashing because the last parameter wasn't getting cleared because we didnt have it in parameters
DWORD32* __fastcall setgamemodedetour(DWORD32 index, DWORD32* address, char* a_struct) {
    //DWORD32* out = setgame_target(param2, in_ec);
    
    cur_index = index;

    //std::ofstream file;
    //file.open("mod_logs\\gamemode.log");
    //try both eax and ebx
    //file << "Before assembly\n";
    Mode m = map.getMode(index);

    char ffa = m.ffa;
    char t_ffa = m.t_ffa;
    //g_ffa = ffa;
    //g_tffa = t_ffa;
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
    g_ffa = ffa;
    g_tffa = t_ffa;
    //file << "Function finished\n";
    //file.close();
    return address;
}


//hook the function which edits the displayed gamemode value and save that string for use

typedef void(__stdcall *GamemodeChange)();
GamemodeChange gc = reinterpret_cast<GamemodeChange>(0x00486f6a);

//recreate locstring so we can get the locstring value from ebp like in possiblegamemodestring



//PossibleMapListGFXGlobal + 0x30 is last stand maps
//PossibleMapListGFXGlobal + 0xc is regular pvp maps
//PossibleMapListGFXGlobal + 0x3c is ffa maps
//PossibleMapListGFXGlobal + 0x18 is benchmark maps?
//PossibleMapListGFXGlobal + 0x0 is campaign maps

//function to be mempatched into MultiplayerUpdateMapList
//do a mid function hook retard
void* __stdcall ReplaceAddr() {
    DWORD t = map_list.getMapList(cur_index);
    return (void*)t;
}


typedef void(__stdcall *UpdateMapList)(void* param1);
UpdateMapList maplist_org = nullptr;

void __stdcall UpdateMapListDetour(void* param1) {
    int* ivar4;
    __asm {
        push ebx;
        mov ebx, dword ptr[ebp + 0x8];
        mov ivar4, ebx;
        pop ebx;
    }
    void** ppv = (void**)(ivar4 + 100);
    //UISetMapList();
    *(ivar4 + 0x60) = 0xffffffff;


}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    //DetourRestoreAfterWith();
    //DetourIsHelperProcess();
    std::ofstream f;
    BYTE* src;
    BYTE* tp;
    DWORD32 fp;
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        base = (DWORD)GetModuleHandleA("DOW2.exe");
        setgame_target = reinterpret_cast<setGamemode>(base + 0x882c6);
        maplist_org = reinterpret_cast<UpdateMapList>(base + 0x86e0d);
        //mod_org = reinterpret_cast<ModAssignPlayers>(base + 0x39e880);
        

        fp = (DWORD32)ReplaceAddr;
        tp = (BYTE*)ReplaceAddr;
        src = (BYTE*)"\x9a" + tp[0] + tp[1] + tp[2] + tp[3]; //now concat fp onto end
        MemPatch(reinterpret_cast<BYTE*>(base + 0x86e42), src, 5); //first part of if
        MemPatch(reinterpret_cast<BYTE*>(base + 0x86e4e), src, 5); //second part of if
        //patching other 5 bytes
        //src = (BYTE*)"\x89\xc0\x";
        NopPatch(reinterpret_cast<BYTE*>(base + 0x86e47), 5);


        NopPatch(reinterpret_cast<BYTE*>(base + 0x86e53), 5);

        ldmaps_org = reinterpret_cast<LoadMaps>(base + 0x7a42d0);
        map_list = MapLoader(base);
        map_list.generateMapList("mods\\maps\\glorb", 4);

        map.readConfig("mods\\gmd.cfg");
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach((void**)&setgame_target, setgamemodedetour);
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

