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


typedef int(__stdcall *GrabA1)(int e1, int e2);
GrabA1 g_a1 = nullptr;

//the shells are stored in 000_data.sga
//ui/textures/space_marines/hud/selection_panel/x_bg.dds - will have x_bg_{name}.dds for factions, ork uses w_bg.dds

//e1 is the class, a2 is the target, a3 is memory to write to
int CreateWidget(int e1, const char* e2, int e3) {
    int t = 0;
    __asm {
        mov esi, e3;
        mov edx, e2;
        mov eax, e1;
        call sfw;
        mov t, eax;
    }
    return t;
}
DWORD addToDic = 0;

int* callAddToDic(int m1, int m2) {
    int* t = nullptr;
    __asm {
        push m2;
        mov esi, m1;
        call addToDic;
        mov t, eax;
    }
    return t;
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

DWORD* a1 = nullptr;
DWORD SPVFT = 0;

DWORD selectuielement = 0;


char* test1 = nullptr;
DWORD test_jmpback = 0;
void __declspec(naked) TestMidDrawShell() {
    __asm {
        //lea ecx, [esi+0x48];
        mov ecx, dword ptr[test1];
        //mov ecx, dword ptr[esi + 0x48];
        mov eax, 0x1;
        //mov edx, dword ptr[ecx];
        //mov ecx, dword ptr [ecx];
        //call revwaaagh;
        call selectuielement;
        jmp [test_jmpback];
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
        sub_AD3960 = base + 0x6D3960;
        sub_AD3990 = base + 0x6D3990;
        reveal_waaagh = reinterpret_cast<RevealWaaaghUI>(base + 0x285660);
        revwaaagh = (base + 0x285660);
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
        DrawUIElement = base + 0x6D39C0;
        jmpback_drawui = base + 0x76D1A5;
        jmpback_drawui_t = base + 0x76D1C1;
        //JmpPatch(reinterpret_cast<BYTE*>(base + 0x76D04D), (DWORD)MidDrawUI, 6);
        test_jmpback = (base + 0x76D18F); //00B6D0C9
        selectuielement = (base + 0x285450);
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x76D0E0), (DWORD)TestMidDrawShell, 7);
        NopPatch(reinterpret_cast<BYTE*>(base + 0x76D0BE), 4);
        NopPatch(reinterpret_cast<BYTE*>(base + 0x76D0C5), 4);
        call_681B50 = reinterpret_cast<sub_681B50>(base + 0x281B50);
        test1 = new char[0x3C8];
        //test1 widget memory not correct, lacking memory address at very end, two ones in two bytes before that, and there should not be any uninit chars in mem block
        if (test1) {
            addToDic = base + 0x282760;
            a1 = new DWORD[0x5C];
            SPVFT = (base + 0xD15490);
            call_681B50((int)a1, (int)"ui\\movies\\hud\\selection_panel");
            *a1 = (DWORD)&SPVFT;
            a1[12] = 0;
            a1[13] = 0;
            a1[14] = 0;
            a1[15] = 1176256512;
            a1[16] = 0;
            a1[17] = 0;
            CreateWidget((int)a1, "/waaagh_meter_shell/meter_mc/sm", (int)test1);
            callAddToDic((int)test1, (int)a1);
           
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

