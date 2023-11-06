// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"

//hook updating list of lan games
//search lan network with the dll instead of relying on game exe
//update list using lan

//following net_hostmatch up - 5EDF80
//0055919C
//00548C92
//004C913D
//004C90B0
DWORD base;
DWORD jmpback_lobbytype;
void __declspec(naked) MidLobbyType() {
    __asm {
        mov[ebp + 0xa4], 0x1;
        mov eax, [ebp + 0xa4];
        jmp[jmpback_lobbytype];
    }
}
//following GameListForm::Update up the call stack
//005412DD - callback 
//0054DC41 - passes result of some function up into next function
//00615BD5 - calls linked function with global parameter, 
//005E52AC - ida wont decompile but ghidra shows that it passes the param1 and an offset up, maybe where advertisments are found?
//0055919C
//005AB97E

//look into AddMatchAdvertisment
//-005F0D80
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{

    BYTE* src = (BYTE*)"";
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        base = (DWORD)GetModuleHandleA("DOW2.exe");
        jmpback_lobbytype = (base + 0x1EE414);
        //patches lobby code to always be setup as a lan lobby
        JmpPatch(reinterpret_cast<BYTE*>((base + 0x1EE40E)), (DWORD)MidLobbyType, 6);
        //removes updating of game list
        //NopPatch(reinterpret_cast<BYTE*>(base + 0x6B916), 6);
        
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

