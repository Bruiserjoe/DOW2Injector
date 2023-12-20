// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"
DWORD base;
HMODULE util;
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




//the shells are stored in 000_data.sga
//ui/textures/space_marines/hud/selection_panel/x_bg.dds - will have x_bg_{name}.dds for factions, ork uses w_bg.dds

//e1 is the class, a2 is the target, a3 is memory to write to
char* test1 = nullptr;
int CreateWidget(int e1, const char* e2, int e3) {
    int t = 0;
    __asm {
        mov esi, e3;
        mov edx, e2;
        mov eax, e1;
        call sfw;
        mov t, eax;
    }
    std::ofstream f;
    f.open("predic.txt", std::ios::binary);
    for (size_t i = 0; i < 0x3C8; i++) {
        f << test1[i];
    }
    f.close();
    return t;
}
DWORD addToDic = 0;

void callAddToDic(int m1, int m2) {
    __asm {
        push m2;
        mov esi, m1;
        call addToDic;
    }
    std::ofstream f;
    f.open("predic.txt", std::ios::binary);
    for (size_t i = 0; i < 900; i++) {
        f << test1[i];
    }
    f.close();
}


//all of the random callbacks in the function all call to the same function

const char* shell;

//functions called for setting up texture being drawn too?
DWORD sub_AD3960 = 0;
DWORD sub_AD3990 = 0;

//function which all races call besides orks
typedef void(__thiscall *RevealWaaaghUI)(uint8_t* c);
RevealWaaaghUI reveal_waaagh = nullptr;
DWORD revwaaagh = 0;


//dictionary instance function
typedef int*(__stdcall *DInstance)(void);
DInstance DicInstance = nullptr;

//dictionary key function
typedef int* (__thiscall *DGetKey)(int* esi, const char* p1);
DGetKey DicGetKey = nullptr;
DWORD DrawUIElement = 0;


char* v1; //class value for waaagh meter
int v32; //eax before we change it to the shell we want

DWORD jmpback_drawui;
DWORD jmpback_drawui_t;
void __declspec(naked) MidDrawUI() {
    __asm {
        mov [v32], eax; //saving eax value for use later
        add eax, 0x70; //aligning eax to the race name
        mov [shell], eax; //putting into shell var
        mov [v1], esi; //putting esi into v1 for use later

        
        jmp [jmpback_drawui_t];
    }
}
typedef int(__stdcall *sub_681B50)(int a1, int a2);
sub_681B50 call_681B50 = nullptr;
//shit that actually matters lol
DWORD* a1 = nullptr;
DWORD SPVFT = 0;

DWORD selectuielement = 0;


const char* gnn = "/waaagh_meter_shell/meter_mc/gn";
DWORD test_jmpback = 0;
void __declspec(naked) TestMidDrawShell() {
    __asm {
        //lea ecx, [esi+0x48];
        mov ecx, dword ptr[test1];
        //mov ecx, dword ptr[esi + 0x54];
        //mov ecx, dword ptr[esi + 0x48];
        mov eax, 0x1;
        //mov edx, dword ptr[ecx];
        //mov ecx, dword ptr [ecx];
        //call revwaaagh;
        call selectuielement;
        jmp [test_jmpback];
    }
}
//const char* sm1 = "/waaagh_meter_shell/meter_mc/sm";
void __declspec(naked) shellfunc() {
    __asm {
        mov esi, test1;
        mov edx, gnn;
        mov eax, a1;
        mov byte ptr[esp + 0x2A4], 0x6E;
        call sfw;
        push ebp;
        mov esi, eax;
        mov byte ptr[esp + 0x2A8], 0;
        call addToDic;
    }
}

DWORD jmpback_changesm = 0;
void __declspec(naked) MidChangeSM() {
    __asm {
        push gnn;
        jmp [jmpback_changesm];
     }
}


DWORD shellgen_jmpback = 0;
void __declspec(naked) MidShellGenerate() {
    __asm {
        mov a1, ebp;
        mov esi, test1;
        mov edx, gnn;
        mov eax, ebp;
        mov byte ptr[esp + 0x2A4], 0x5C;
        call sfw;
        push ebp;
        mov esi, eax;
        mov byte ptr[esp + 0x2A8], 0;
        call addToDic;
        //call shellfunc;
        jmp[shellgen_jmpback];
    }
}
//maybe just change the image loaded everytime the player loads in?
    //-use the space marine space to load the image we want?
    //-maybe change the lookup name at top of function
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
        sub_AD3960 = base + 0x6D3960;
        sub_AD3990 = base + 0x6D3990;
        reveal_waaagh = reinterpret_cast<RevealWaaaghUI>(base + 0x285660);
        revwaaagh = (base + 0x285660);
        sf_widg = reinterpret_cast<CreateSFWidget>(base + 0x285050);
        sfw = (base + 0x285050);
        //testing
        //NopPatch(reinterpret_cast<BYTE*>(base + 0x3A9B63), 5);
        jmpback_midstats = (base + 0x64D64);
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x64CB8), (DWORD)MidStatsPopGenerate, 7);

        //testing for the waaagh meter patch
        DrawUIElement = base + 0x6D39C0;
        jmpback_drawui = base + 0x76D1A5;
        jmpback_drawui_t = base + 0x76D1C1;
        //JmpPatch(reinterpret_cast<BYTE*>(base + 0x76D04D), (DWORD)MidDrawUI, 6);
        test_jmpback = (base + 0x76D18F); //00B6D0C9

        //used
        selectuielement = (base + 0x285450);
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x76D0E0), (DWORD)TestMidDrawShell, 7);
        NopPatch(reinterpret_cast<BYTE*>(base + 0x76D0BE), 4);
        NopPatch(reinterpret_cast<BYTE*>(base + 0x76D0C5), 4);

        //used
        //shellgen_jmpback = base + 0x73C5E8;
        //shellgen_jmpback = base + 0x73C1B9;
        shellgen_jmpback = base + 0x73C1C9;
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x73C18A), (DWORD)MidShellGenerate, 58);
        //JmpPatch(reinterpret_cast<BYTE*>(base + 0x73C1C9), (DWORD)MidShellGenerate, 16);
        addToDic = base + 0x282760;
        
        jmpback_changesm = base + 0x76CF50;
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x76CF4B), (DWORD)MidChangeSM, 5);
        test1 = new char[0x38C];
        for (size_t i = 0; i < 0x38C; i++) {
            test1[i] = 0;
        }
        util = GetModuleHandleA("Util.dll");
        if (util) {
            DicInstance = reinterpret_cast<DInstance>(GetProcAddress(util, MAKEINTRESOURCEA(533)));
            DicGetKey = reinterpret_cast<DGetKey>(GetProcAddress(util, MAKEINTRESOURCEA(385)));
        }
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

