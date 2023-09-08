// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"

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
        JmpPatch(reinterpret_cast<BYTE*>((base + 0x1EE40E)), (DWORD)MidLobbyType, 6);
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

