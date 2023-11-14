// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"
DWORD base;

//traced slot popup menu functions
/*
00493408 - SetSlotExpertAI
004933AE - SetSlotEasyAI
004933CC - SetSlotNormalAI
004933EA - SetSlotHardAI
00492EF1 - MoveToThisSlot
0049334D - CloseThisSlot
0049326C - InviteFriendSlot
0049336B - OpenThisSlot
00464E15 - ViewPLayerStatsSlot
00464D72 - PlayerProfileSlot
00492E02 - requestswapslot
*/

/*
004590F6 - PopupMenu_Setup
00459595 - PopupMenu_OnClickItem
004592FB - PopupMenu_OnExit
*/

/*
slot dropdown generators
00491FD5 - observer drop down generation
0049210A - regular slot drop down generation
0049265A - closed slot drop down generation
*/

DWORD jmpback_midstats;
void __declspec(naked) MidStatsPopGenerate() {
    __asm {
        jmp[jmpback_midstats];
    }
}


typedef int(__fastcall *CreateSFWidget)(int e1, const char* a2, int a3);
CreateSFWidget sf_widg = nullptr;
DWORD sfw = 0;


typedef int(__stdcall *GrabA1)(int e1, int e2);
GrabA1 g_a1 = nullptr;

//the shells are stored in 000_data.sga
//ui/textures/space_marines/hud/selection_panel/x_bg.dds - will have x_bg_{name}.dds for factions, ork uses w_bg.dds

void CreateWidget(int e1, const char* a2, int a3) {
    __asm {
        mov esi, a3;
        mov edx, a2;
        mov eax, e1;
        call sfw;
        pop eax;
        pop edx;
        pop esi;
    }
}


//all of the random callbacks in the function all call to the same function

const char* shell;

extern "C" void drawUI() {
    std::string s(shell);
    if (s.compare("race_ork") == 0) {
        //call the two functions, need to figure out how to call a __userpurge function, https://stackoverflow.com/questions/4099026/how-to-hook-usercall-userpurge-spoils-functions
        
    }
    else {
        //call the the function that the other factions use, and set that one variable
    }
    std::ofstream file;
    file.open("shell.txt");
    file << s;
    file.close();
}


DWORD jmpback_drawui;
void __declspec(naked) MidDrawUI() {
    __asm {
        add eax, 0x70;
        mov [shell], eax;
        call drawUI;
        jmp [jmpback_drawui];
    }
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    BYTE* src = (BYTE*)"";
    //DWORD e = 0;
    //DWORD* a = &e;
    //char* a2 = (char*)new int[0x3C8u];
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        base = (DWORD)GetModuleHandleA("DOW2.exe");
        sf_widg = reinterpret_cast<CreateSFWidget>(base + 0x285050);
        sfw = (base + 0x285050);
        g_a1 = reinterpret_cast<GrabA1>(base + 0x281B50);
        //testing
        //NopPatch(reinterpret_cast<BYTE*>(base + 0x3A9B63), 5);
        jmpback_midstats = (base + 0x64D64);
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x64CB8), (DWORD)MidStatsPopGenerate, 7);

        //testing for the waaagh meter patch
        //find buttons using change in waaagh with cheat engine
        //B6D18A
        //g_a1((int)a, (int)"ui\\movies\\hud\\selection_panel");
        //*(DWORD*)a = (base + 0xD15490);
        //CreateWidget((int)a, "/waaagh_meter_shell/meter_mc/sm", (int)a2);
        
        //NopPatch(reinterpret_cast<BYTE*>(base + 0x76D1BA), 7); //try changing the instance we get at this memory location, and see what happens, did nothing lol
        jmpback_drawui = base + 0x76D1A5;
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x76D04D), (DWORD)MidDrawUI, 6);

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

