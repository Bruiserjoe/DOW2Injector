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

//look into resourcedecorator reveal function
//400000
//v5+23 is +num/icon decorator above node?
//v5+24 is resource decorator number
//v5+25 is resource point damage
//v5+27 is node damage


/*
00B22FAA //completed, for power node. smaller bar for nodes
00B22D8F //called when unit selected or resource selected

00682013 jumps back to
*/


/*
00B92662
00680CAC
0040E63C
0041D961
*/


//squad group, group of squads
//entity group, just a group of entities

DWORD jmpback_wrapper = 0;
DWORD edx_value = 0;
DWORD target_function = 0;
void __declspec(naked) MidWrapper() {
    __asm {
        mov eax, dword ptr[edi];
        mov edx, dword ptr[eax + 0x10];
        mov edx_value, edx;
    }
    if (edx_value == target_function) {
        edx_value = 0;
    }
    __asm {
        jmp[jmpback_wrapper];
    }
}


DWORD sub_682070 = 0;

//hooking reveal function to inspect the parameters
//figure out how to read the st0, st1, etc registers
DWORD jmpback_reveal = 0;
float s_regs[7];
long double test3 = 1.0;
float test1 = 0.0f;
float test2 = 0.0f;
uint32_t tttt = 0;
//st0 i think controls how full the decorator is, they are floats i think
//https://www.website.masmforum.com/tutorials/fptute/fpuchap4.htm#fstp
//https://www.website.masmforum.com/tutorials/fptute/fpuchap1.htm
//figure out how to fucking loading st register values properly into my VARIABLES
void __declspec(naked) MidReveal() {
    //get all of the st values out of the registers
    //doing my own fld and fst gives me the proper values. So why does doing it to their values fail?
   __asm {
       finit;
       // fld dword ptr [test2];
       fst test1;
       // fstp [test3];
    }
   test1 += 0.1f;
   test2 += 0.1f;
   test3 += 0.1;
    __asm {
        call sub_682070;
    }
    __asm{
        jmp[jmpback_reveal];
    }
}


//hook the wrapper and see what it passes down
//400000
//probably have to add the healthbar myself :(
//determine how the xyz of the healthbars are getting set
//see what changes in in sf when actual healthbar rendered

//listening post functions
//B5AFE0
//8701B0
//870610

//other two functions in revealresourcedecorator
//00DCC200
//009AB7D0

//know what function projects the values into the world space
//need to know what values and functions dictate the screen space coordinates

//figure out where the float values are pushed onto stack
//also try testing some stuff with the discovered function

//follow decorator_wrapper up the call stack and figure out the constructor is doing and what gives it, its parameters

/*
00B23153
00682013
00B92662
00680CAC - this function is where I need to reverse most likely
0040E63C
*/

//crashes on noping out nodehealth damage and not on nodehealth
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    BYTE* src = (BYTE*)"\xEB";
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        base = (DWORD)GetModuleHandleA("DOW2.exe");
        //removing stats dropdown patch
        jmpback_midstats = (base + 0x64D64);
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x64CB8), (DWORD)MidStatsPopGenerate, 7); 
        //this jmppatch just jmps around 
        //the entire code which creates the ui element

        // scaling patch
        // https://github.com/RipleyTom/rustpatch
        MemPatch(reinterpret_cast<BYTE*>(base + 0x77C1F0), src, 1);

        //testing for decorator
        jmpback_wrapper = base + 0x28200D;
        target_function = base + 0x722AF0;
        // JmpPatch(reinterpret_cast<BYTE*>(base + 0x282008), (DWORD)MidWrapper, 5);

        sub_682070 = base + 0x282070;
        jmpback_reveal = base + 0x72312F;
        // JmpPatch(reinterpret_cast<BYTE*>(base + 0x72312A), (DWORD)MidReveal, 5);

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

