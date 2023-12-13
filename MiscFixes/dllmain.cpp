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


DWORD sub_AD3960 = 0;
//a1 is the char array, esi0 is the class, a2 gets pushed onto the stack, 
void callAD3960(int a1, int esi0, int a2) {
    __asm {
        lea eax, [a2];
        push eax;
        mov eax, a1;
        mov esi, esi0;
        call sub_AD3960;
    }
}
DWORD sub_AD3990 = 0;
//a1 is the char array, esi0 is the class, a2 gets pushed onto the stack, 
void callAD3990(int a1, int esi0, int a2) {
    __asm {
        lea edx, [a2]
        push edx;
        mov eax, a1;
        mov esi, esi0;
        call sub_AD3990;
    }
}

//function which all races call besides orks
typedef void(__stdcall *sub_686180)(int a1, int a2, int a3);
sub_686180 race_call = nullptr;

//dictionary instance function
typedef int*(__stdcall *DInstance)(void);
DInstance DicInstance = nullptr;

//dictionary key function
typedef int* (__thiscall *DGetKey)(int* esi, const char* p1);
DGetKey DicGetKey = nullptr;
DWORD DrawUIElement = 0;


char* v1; //class value for waaagh meter
int v32; //eax before we change it to the shell we want

const char* waaagh_shell = "/waaagh_meter_shell";

DWORD offset_text = (0);
DWORD offset_waagh = (0);
extern "C" void drawUI() {
    std::string s(shell);
    if (s.compare("race_ork") == 0) {
        //call the two functions, need to figure out how to call a __userpurge function, https://stackoverflow.com/questions/4099026/how-to-hook-usercall-userpurge-spoils-functions
        //callAD3960((int)"/waaagh_meter_shell/waaagh_mc", (int)v1, (int)(v1 + 60));
        //callAD3990((int)"/waaagh_meter_shell/meter_mc/waaagh_text", (int)v1, (int)(v1 + 56));
        //testing
        __asm {
            //testing
            lea eax, [esi + 0x3C];
            push eax;
            mov eax, offset_waagh;
            call sub_AD3960;
            //text*/
            lea eax, [esi + 0x38];
            push eax;
            mov eax, offset_text;
            call sub_AD3990;
        }
    }
    else {
        __asm {
            //testing
            lea eax, [esi + 0x3C];
            push eax;
            mov eax, offset_waagh;
            call sub_AD3960;
            //text*/
            lea eax, [esi + 0x38];
            push eax;
            mov eax, offset_text;
            call sub_AD3990;
        }
        //call the the function that the other factions use, and set that one variable
        //callAD3960((int)"/waaagh_meter_shell/waaagh_mc", (int)v1, (int)(v1 + 60));
        //callAD3990((int)"/waaagh_meter_shell/meter_mc/waaagh_text", (int)v1, (int)(v1 + 56));
        if (s.compare("race_marine") == 0) {
            int* v19 = (int*)*((DWORD*)v1 + 16);
            if (v19 != 0 && *((BYTE*)v19 + 521) != 1) {
                int v20 = *v19;
                *((BYTE*)v19 + 521) = 1;
                uint8_t* t = (uint8_t*)v19;
                race_call((int)t, (int)(t + 8), t[521]);
            }
        }
        
    }
    //*((float*)v1 + 9) = *(float*)(v32 + 2492);
    //*((float*)v1 + 11) = *(float*)(v32 + 2508);
    __asm {
        mov     eax, [v32];
        fld     dword ptr[eax + 0x9BC];
        fstp    dword ptr[esi + 0x24];
        fld     dword ptr[eax + 0x9CC];
        fstp    dword ptr[esi + 0x2C];
    }
    //drawing waaagh meter shell
    __asm {
        //dic instance
        push waaagh_shell;
        lea ecx, [v32];
        push ecx;
        call DicInstance;
        //dic key
        mov ecx, eax;
        call DicGetKey;
        //DrawUIElement
        mov eax, [eax];
        lea edx, [esi + 0x58];
        push edx;
        push eax;
        call DrawUIElement;
    }
    std::ofstream file;
    file.open("shell.txt");
    file << s;
    file.close();
}
DWORD jmpback_drawui;
DWORD jmpback_drawui_t;
void __declspec(naked) MidDrawUI() {
    __asm {
        mov [v32], eax; //saving eax value for use later
        add eax, 0x70; //aligning eax to the race name
        mov [shell], eax; //putting into shell var
        mov [v1], esi; //putting esi into v1 for use later

        //calling function
        call drawUI; //calling the actual function
        jmp [jmpback_drawui_t];
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
        race_call = reinterpret_cast<sub_686180>(base + 0x286180);
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
        offset_text = (base + 0xD1522C);
        offset_waagh = (base + 0xD1520C);
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x76D04D), (DWORD)MidDrawUI, 6);
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

