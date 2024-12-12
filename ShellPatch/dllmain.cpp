// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"

DWORD base;
HMODULE util;
HMODULE plat;
ShellMap sh_map;

std::vector<std::string> shells;

typedef void(__cdecl* Timestampedf)(const char*, ...);
typedef void(__cdecl* Fatalf)(const char*, ...);

Timestampedf Timestampedtracef;
Fatalf Fatal_f;
HMODULE debug;

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

DWORD DrawUIElement = 0;
DWORD SelectUIElement = 0;


//shit that actually matters lol    




//shell generation hooks

const char* shell_char;
std::string race_string = "";
void getShellName() {
    __asm {
        mov ecx, dword ptr ds : 0x1335720;
        mov edx, [ecx + 0x8];
        mov eax, [edx + 0x4];
        mov eax, dword ptr[eax + 0xc8];
        add eax, 0x70;
        mov shell_name, eax;
    }
    race_string = sh_map.lookupShell(shell_name);
    shell_char = race_string.c_str();
}

HMODULE mscvr80;

DWORD loadshells_jmpback = 0;
DWORD operator_new = 0;
DWORD sf_widget_c = 0;
DWORD add_to_dic = 0;


int index_i = 0;
const char* cur_shell_load = nullptr;
char shell_load_index = 110;
//maybe use offset of value in double pointer
void __declspec(naked) MidLoadShells() {
    /*__asm {
        call add_to_dic;
    }*/

    for (index_i = 0; index_i < sh_map.totalShells(); index_i++, shell_load_index++) {
        cur_shell_load = sh_map.getShell(index_i);
            __asm {
                push 0x3C8;
                call operator_new;
                mov esi, eax;
                add esp, 4;
                //mov dword ptr[esp + 0x14], esi;
                mov dl, shell_load_index;
                mov BYTE PTR[esp + 0x2a4], dl;
                mov edx, cur_shell_load;
                mov eax, ebp;
                call sf_widget_c;
                push ebp;
                mov esi, eax;
                mov BYTE PTR[esp + 0x2a8], 0x0
                call add_to_dic;
            }
    }
     __asm{
        jmp[loadshells_jmpback];
    }
}


//drawing hooks
bool base_shell_yah = false;
DWORD test_jmpback = 0;
DWORD* render_ptr = nullptr;
DWORD render_offset = 0;
void __declspec(naked) TestMidDrawShell() {
    if (base_shell_yah) {
        __asm {
            mov ecx, esi;
            add ecx, render_offset;
            mov ecx, dword ptr[ecx];
        }
    }
    else {
        __asm {
            mov ecx, dword ptr[render_ptr];
        }
    }
    __asm {
        mov al, 0x1;
        call SelectUIElement;
    }
    __asm{ 
        jmp[test_jmpback];
    }
}


DWORD constant_shell = 0;
DWORD change_jmpback = 0;
void __declspec(naked) ChangeMidShell() {
    __asm {
        push shell_char;
        jmp[change_jmpback];
    }
}

DWORD dic_in = 0;
DWORD dic_key = 0;

char* selection_panel_pointer = 0;
void getRenderPointer() {
    //sh_map.setRacePointer("race_marine", false, tt);
    sh_map.updateRacePointers();
    render_ptr = sh_map.getShellTarget(0);
    render_ptr = (DWORD*)sh_map.lookupShellPointer(race_string);
    base_shell_yah = sh_map.lookupBaseShell(race_string);
    if (base_shell_yah) {
        render_offset = sh_map.getBaseShellOffset(race_string);
    }
}

DWORD shellget_jmpback = 0;
DWORD* temp_shell_target = 0;
//cur_shell_load is same offset so it draws the same fuckign thing since it uses the offset omfg, I think this is right fufcucjusiih
void __declspec(naked) MidShellGet() {
    for (index_i = 0; index_i < sh_map.shellNum(); index_i++) {
        cur_shell_load = sh_map.getShell(index_i);
        temp_shell_target = sh_map.getShellTarget(index_i);
        __asm {
            push cur_shell_load;
            lea edx, [esp + 0x14];
            push edx;
            call edi; //dic instance
            mov ecx, eax;
            call ebx; //dic getkey
            lea ecx, [temp_shell_target];
            push ecx;
            mov ecx, [eax];
            push ecx;
            call DrawUIElement; //drawuielement
            //mov ecx, [temp_shell_target];
            //xor al, al;
            //call SelectUIElement;
        }
        sh_map.updateShellTarget(index_i, temp_shell_target);
    }
    __asm {
        push 0x1115304
        jmp[shellget_jmpback];
    }
}

DWORD midshell_jmpback = 0;
void __declspec(naked) MidShellSelect() {
    for (index_i = 0; index_i < sh_map.shellNum(); index_i++) {
        temp_shell_target = sh_map.getShellTarget(index_i);
        __asm {
            mov ecx, [temp_shell_target];
            xor al, al;
            call SelectUIElement;
        }
        sh_map.updateShellTarget(index_i, temp_shell_target);
    }
    getRenderPointer();
    __asm {
        mov ecx, [esi + 0x50];
        xor al, al;
        call SelectUIElement;
        jmp[midshell_jmpback];
    }
}

//rewrite generatewaaaghmeter shell functions


DWORD v32 = 0;
DWORD v17 = 0;
DWORD v18 = 0;




typedef char(__thiscall *GenerateWaaaghMeterShell)(char* ecx);
GenerateWaaaghMeterShell gen_waaagh_target = nullptr;

const char* waaagh_meter_mc = nullptr;
const char* waaagh_text = nullptr;
const char* waaagh_shell = nullptr;
const char* global_abilities = nullptr;

char __fastcall GenerateWaaaghMeterShellDetour(char* ecx1) {
    __asm {
       /* lea edx, [esp + 0x14];
        mov v32, edx;
        mov ecx, dword ptr ds : 0x1335720;
        mov edx, [ecx + 0x8];
        mov eax, [edx + 0x4];
        mov v17, eax;
        mov eax, dword ptr[eax + 0xc8];
        mov v18, eax;
        mov dword ptr [esp + 0x10], eax*/
        mov eax, DWORD PTR ds : 0xf89100;
        mov dic_in, eax;
        mov eax, DWORD PTR ds : 0xf89330;
        mov dic_key, eax;
    }
    getShellName();
    std::string sh(shell_name);
    race_string = sh;
    selection_panel_pointer = ecx1;
    #ifdef _DEBUG
        Timestampedtracef("Shell Patch: patching the shell jmps in!");
    #endif
    if (sh.compare("race_ork") == 0) {
        BYTE* src = (BYTE*)"\x68\x04\x53\x11\x01";
        MemPatch(reinterpret_cast<BYTE*>(base + 0x76CF6F), src, 5); //MidShellGet
        src = (BYTE*)"\x8b\x4e\x50\x32\xc0\xe8\x23\x84\xb1\xff";
        MemPatch(reinterpret_cast<BYTE*>(base + 0x76D023), src, 10); //MidShellSelect
        src = (BYTE*)"\x68\xFF\x00\x00\x00";
        MemPatch(reinterpret_cast<BYTE*>(base + 0x76D0A7), src, 5); //TestMidDrawShell
    }
    else {
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x76D023), (DWORD)MidShellSelect, 10);
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x76CF6F), (DWORD)MidShellGet, 5);
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x76D0A7), (DWORD)TestMidDrawShell, 5);
    }
    char t = gen_waaagh_target(ecx1);
    return t;
}

typedef bool(__stdcall* PlatGetOption)(const char* option, char* str, unsigned int size);
PlatGetOption plat_getoption = nullptr;



//figure out how to properly allocate data in exe, apparently need to add 4 to esp, that fixes crash wtf, ig because of allocate?
//Get select ui element part in
//  -two shells are drawing because we never load gn, figure out why it is still drawing two tf inspect the selectuielemnt execution for new shell
//noping out the initial loading of the shells is the only thing that creates the same problem for the regular shells, noping it out in generatewaaaghshell did nothing
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    std::string modu;
    char mod1[0x200];
    bool ret = false;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        base = (DWORD)GetModuleHandleA("DOW2.exe");

        plat = GetModuleHandleA("Platform.dll");
        if (plat) {
            plat_getoption = reinterpret_cast<PlatGetOption>(GetProcAddress(plat, MAKEINTRESOURCEA(78)));
        }
        debug = GetModuleHandleA("Debug.dll");
        if (debug) {
            Timestampedtracef = reinterpret_cast<Timestampedf>(GetProcAddress(debug, MAKEINTRESOURCEA(50)));
            Fatal_f = reinterpret_cast<Fatalf>(GetProcAddress(debug, MAKEINTRESOURCEA(31)));
        }
        //getting the module name
        ret = plat_getoption("modname", mod1, 0x200);
        modu = std::string(mod1);
        modu = modu + ".shells";

        sh_map.loadFile(modu);
        Timestampedtracef("Shell Patch: Success loading config!");
        waaagh_meter_mc = (const char*)base + 0xD1520C;
        waaagh_text = (const char*)base + 0xD15324;
        waaagh_shell = (const char*)base + 0xD151F8;
        global_abilities = (const char*)base + 0xD14C08;

        gen_waaagh_target = reinterpret_cast<GenerateWaaaghMeterShell>(base + 0x76CF40);
        
        sub_AD3960 = base + 0x6D3960;
        sub_AD3990 = base + 0x6D3990;
        sf_widg = reinterpret_cast<CreateSFWidget>(base + 0x285050);
        sfw = (base + 0x285050);
        //testing for the waaagh meter patch
        DrawUIElement = base + 0x6D39C0;
        SelectUIElement = base + 0x285450;
        if (base == 0) {
            Fatal_f("Shell Patch: Base is zero for some reason! WHAT THE FUCK!");
            return FALSE;
        }
        //sh_map.addShell("/waaagh_meter_shell/meter_mc/gn");
        //sh_map.addShell("/waaagh_meter_shell/meter_mc/nec");
        sh_map.addShell((const char*) base + 0xD15278); //sm
        sh_map.addShell((const char*)base + 0xD15304);//ig
        sh_map.addShell((const char*) base + 0xD15298); //eld
        sh_map.addShell((const char*) base + 0xD152BC); //tyr
        sh_map.addShell((const char*) base + 0xD152E0); //csm
        //used
        

        //new patch stuff
        //used
        addToDic = base + 0x282760;
        
        loadshells_jmpback = base + 0x73C2C5;
        operator_new = base + 0xB879D0;
        sf_widget_c = base + 0x285050;
        add_to_dic = base + 0x282760;
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x73C18A), (DWORD)MidLoadShells, 5);
        
        //new version
        change_jmpback = base + 0x76CF50;
        constant_shell = base + 0xF35720;
        //JmpPatch(reinterpret_cast<BYTE*>(base + 0x76CF4B), (DWORD)ChangeMidShell, 5);
        
        midshell_jmpback = base + 0x76D02D;
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x76D023), (DWORD)MidShellSelect, 10);

        shellget_jmpback = base + 0x76CF74;
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x76CF6F), (DWORD)MidShellGet, 5);
        

        test_jmpback = (base + 0x76D18F);
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x76D0A7), (DWORD)TestMidDrawShell, 5);

        mscvr80 = GetModuleHandleA("MSVCR80");
        if (mscvr80) {
            operator_new = reinterpret_cast<DWORD>(GetProcAddress(mscvr80, MAKEINTRESOURCEA(15))); //new in the dll loaded by dow2
        }

        util = GetModuleHandleA("Util.dll");
        if (util) {
            dic_in = reinterpret_cast<DWORD>(GetProcAddress(util, MAKEINTRESOURCEA(533)));
            dic_key = reinterpret_cast<DWORD>(GetProcAddress(util, MAKEINTRESOURCEA(385)));;
        }
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach((void**)&gen_waaagh_target, GenerateWaaaghMeterShellDetour);
        DetourTransactionCommit();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

