// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"

DWORD base;
HMODULE util;
ShellMap sh_map;

DWORD image_jmpback;
const char* testipath = "selection_panel/x_bg_eld.dds";
void __declspec(naked) EditImagePath() {
    __asm {
        //copying asm we copied over
        mov dword ptr[esp + 0x28], eax;
        push 0x1;
        //actually what we want to do
        mov edx, dword ptr[esi + 0x10];
        mov eax, testipath;
        mov dword ptr[edx + 0x10], eax;
        mov eax, dword ptr [esp + 0x28];
        jmp[image_jmpback];
    }
}


typedef int(__fastcall* CreateSFWidget)(int e1, const char* a2, int a3);
CreateSFWidget sf_widg = nullptr;
DWORD sfw = 0;


//the shells are stored in 000_data.sga
//ui/textures/space_marines/hud/selection_panel/x_bg.dds - will have x_bg_{name}.dds for factions, ork uses w_bg.dds


DWORD addToDic = 0;




//all of the random callbacks in the function all call to the same function

const char* shell_name;

//functions called for setting up texture being drawn too?
DWORD sub_AD3960 = 0;
DWORD sub_AD3990 = 0;

//function which all races call besides orks
typedef void(__thiscall* RevealWaaaghUI)(uint8_t* c);
RevealWaaaghUI reveal_waaagh = nullptr;
DWORD revwaaagh = 0;


//dictionary instance function
typedef int* (__stdcall* DInstance)(void);
DInstance DicInstance = nullptr;

//dictionary key function
typedef int* (__thiscall* DGetKey)(int* esi, const char* p1);
DGetKey DicGetKey = nullptr;
DWORD DrawUIElement = 0;

//shit that actually matters lol



//shell generation hooks
DWORD select_a1 = 0;
DWORD newop = 0x0;
std::vector<std::string> shells = { "/waaagh_meter_shell/meter_mc/gn" };
extern "C" void loadNewShells() {
    //int count = 0x6E;
    for (auto& i : shells) {
        const char* t = i.c_str();
        __asm {
            mov edx, ebp;
            mov eax, select_a1;
            push 0x3C8;
            call newop; //apparently new operator in the games memory
            //jz to xor
            //jz short locationxor; guess we ain't being safe here 8v8
            //sfw section
            add byte ptr[esp + 0x2A4], 0x1;
            mov esi, eax;
            mov edx, t;
            mov eax, select_a1;
            call sfw;
            //dic section
            push select_a1;
            mov esi, eax;
            mov byte ptr[esp + 0x2a8], 0x0;
            call addToDic;
        }
    }
}

typedef DWORD* (__stdcall* GenerateSelectionPanel)(DWORD* a1);
GenerateSelectionPanel gen_selection_target = nullptr;

DWORD* __stdcall GenerateSelectionPanelDetour(DWORD* a1) {

    DWORD* t = gen_selection_target(a1);
    //loadNewShells({ "/waaagh_meter_shell/meter_mc/gn" }, a1);
    return t;
}



const char* gn_name = "/waaagh_meter_shell/meter_mc/gn";
DWORD shellgen_jmpback = 0;
void __declspec(naked) MidShellGenerate() {
    __asm {
        call addToDic;
        mov select_a1, ebp;
        /*push 0x3C8;
        call newop; //apparently new operator in the games memory
        mov esi, eax;
        add esp, 0x4;
        mov dword ptr[esp + 0x14], esi;
        mov byte ptr[esp + 0x2A4], 0x6E;
        mov edx, gn_name;
        mov eax, ebp;
        call sfw; //calls createsfw widget
        push ebp;
        mov esi, eax;
        mov byte ptr[esp + 0x2A8], 0x0;
        call addToDic;*/
        call loadNewShells;
        //end
        jmp[shellgen_jmpback];
    }
}




//drawing hooks
DWORD test_jmpback = 0;
void __declspec(naked) TestMidDrawShell() {
    __asm {
        //lea ecx, [esi+0x48];
        //mov ecx, dword ptr[test1];
        //mov ecx, dword ptr[esi + 0x54];
        mov ecx, dword ptr[esi + 0x40];
        //mov ecx, dword ptr[esi + 0x4C];
        //mov al, 0x1;
        //mov edx, dword ptr[ecx];
        //mov ecx, dword ptr [ecx];
        //call revwaaagh;
        //call selectuielement_location;
        jmp[test_jmpback];
    }
}
std::string shell_string;
const char* shell_char;
void getShellName() {
    __asm {
        mov ecx, dword ptr ds : 0x1335720;
        mov edx, [ecx + 0x8];
        mov eax, [edx + 0x4];
        mov eax, dword ptr[eax + 0xc8];
        add eax, 0x70;
        mov shell_name, eax;
    }
    shell_string = sh_map.lookupShell(shell_name);
    shell_char = shell_string.c_str();
}
DWORD constant_shell = 0;
DWORD change_jmpback = 0;
void __declspec(naked) ChangeMidShell() {
    __asm {
        push shell_char;
        jmp[change_jmpback];
    }
}



typedef char(__thiscall *GenerateWaaaghMeterShell)(char* ecx);
GenerateWaaaghMeterShell gen_waaagh_target = nullptr;

char __fastcall GenerateWaaaghMeterShellDetour(char* ecx) {
    getShellName();
    std::string sh(shell_name);
    if (sh.compare("race_ork") == 0) {
        BYTE* src = (BYTE*)"\x68\x78\x52\x11\x01";
        MemPatch(reinterpret_cast<BYTE*>(base + 0x76CF4B), src, 5); //ChangeMidShell
        src = (BYTE*)"\x68\xFF\x00\x00\x00";
        MemPatch(reinterpret_cast<BYTE*>(base + 0x76D0A7), src, 5); //TestMidDrawShell
    }
    else {
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x76CF4B), (DWORD)ChangeMidShell, 5);
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x76D0A7), (DWORD)TestMidDrawShell, 5);
    }
    char t = gen_waaagh_target(ecx);
    return t;
}

//figure out where image data is actually loaded and change out the path for sm to the one we want
//  -dynamic loading of however many shells you got listed in config file, use detours and just call a function to load them all after calling original
//          //try doing a mid function hook instead of using detours
//  -test with more than one new shell
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        base = (DWORD)GetModuleHandleA("DOW2.exe");
        gen_waaagh_target = reinterpret_cast<GenerateWaaaghMeterShell>(base + 0x76CF40);
        gen_selection_target = reinterpret_cast<GenerateSelectionPanel>(base + 0x739850);
        sub_AD3960 = base + 0x6D3960;
        sub_AD3990 = base + 0x6D3990;
        reveal_waaagh = reinterpret_cast<RevealWaaaghUI>(base + 0x285660);
        revwaaagh = (base + 0x285660);
        sf_widg = reinterpret_cast<CreateSFWidget>(base + 0x285050);
        sfw = (base + 0x285050);
        //testing for the waaagh meter patch
        DrawUIElement = base + 0x6D39C0;




        //used
       
        

        //new patch stuff
        //used
        shellgen_jmpback = base + 0x73C5E8;
        newop = base + 0x9AA9B6;
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x73C5E3), (DWORD)MidShellGenerate, 5); 
        addToDic = base + 0x282760;
        //new version
        change_jmpback = base + 0x76CF50;
        constant_shell = base + 0xF35720;
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x76CF4B), (DWORD)ChangeMidShell, 5);
        test_jmpback = (base + 0x76D188);
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x76D0A7), (DWORD)TestMidDrawShell, 5);


        util = GetModuleHandleA("Util.dll");
        if (util) {
            DicInstance = reinterpret_cast<DInstance>(GetProcAddress(util, MAKEINTRESOURCEA(533)));
            DicGetKey = reinterpret_cast<DGetKey>(GetProcAddress(util, MAKEINTRESOURCEA(385)));
        }
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach((void**)&gen_waaagh_target, GenerateWaaaghMeterShellDetour);
        //DetourAttach((void**)&gen_selection_target, GenerateSelectionPanelDetour);
        DetourTransactionCommit();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

