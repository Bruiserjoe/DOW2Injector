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
DWORD SelectUIElement = 0;


//shit that actually matters lol    
const char* cur_name = "/waaagh_meter_shell/meter_mc/gn";




//shell generation hooks




const char* gn_name = "/waaagh_meter_shell/meter_mc/gn";

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

HMODULE mscvr80;

DWORD loadshells_jmpback = 0;
DWORD operator_new = 0;
DWORD sf_widget_c = 0;
DWORD add_to_dic = 0;

int index_i = 0;
const char* cur_shell_load = nullptr;
void __declspec(naked) MidLoadShells() {
    
    for (index_i = 0; index_i < sh_map.shellNum(); index_i++) {
        cur_shell_load = sh_map.getShell(index_i);
        __asm {
            push 0x3C8;
            call operator_new;
            mov esi, eax;
            add esp, 4;
            mov edx, offset cur_shell_load;
            mov eax, ebp;
            call sf_widget_c;
            push ebp;
            mov esi, eax;
            call add_to_dic;
        }
    }
     __asm{
        push 0x3C8;
        call operator_new;
        jmp[loadshells_jmpback];
    }
}


//drawing hooks
std::string race_string = "";
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
            //lea ecx, [esi+0x48];
            //mov ecx, dword ptr[test1];
            //mov ecx, dword ptr[esi + 0x54];
            //mov ecx, dword ptr[esi + 0x44];
            mov ecx, dword ptr[render_ptr];
        }
    }
        //mov ecx, dword ptr[tt];
        //mov ecx, dword ptr[esi + 0x4C];
        //mov al, 0x1;
        //mov edx, dword ptr[ecx];
        //mov ecx, dword ptr [ecx];
        //call revwaaagh;
        //call selectuielement_location;
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
void __declspec(naked) MidShellGet() {
    for (index_i = 0; index_i < sh_map.shellNum(); index_i++) {
        cur_shell_load = sh_map.getShell(index_i);
        temp_shell_target = sh_map.getShellTarget(index_i);
        __asm {
            push offset cur_shell_load;
            lea edx, [esp + 0x14];
            push edx;
            call edi; //dic instance
            mov ecx, eax;
            call ebx; //dic getkey
            mov eax, [eax];
            lea ecx, [temp_shell_target];
            push ecx;
            push eax;
            call DrawUIElement; //drawuielement
            mov ecx, [temp_shell_target];
            xor al, al;
            call SelectUIElement;
        }
        sh_map.updateShellTarget(index_i, temp_shell_target);
    }
    getRenderPointer();
    __asm {
        push 0x1115304
        jmp[shellget_jmpback];
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
        lea edx, [esp + 0x14];
        mov v32, edx;
        mov ecx, dword ptr ds : 0x1335720;
        mov edx, [ecx + 0x8];
        mov eax, [edx + 0x4];
        mov v17, eax;
        mov eax, dword ptr[eax + 0xc8];
        mov v18, eax;
        mov dword ptr [esp + 0x10], eax
        mov edi, DWORD PTR ds : 0xf89100;
        mov dic_in, edi;
        mov ebx, DWORD PTR ds : 0xf89330;
        mov dic_key, ebx;
    }

    /*for (int i = 0; i < sh_map.shellNum(); i++) {
        cur_shell_load = sh_map.getShell(i);
        temp_shell_target = sh_map.getShellTarget(i);
        DWORD t = 0;
        __asm {
            lea eax, [esp + 0x14];
            push offset cur_shell_load;
            push eax;
            call dic_in;
            mov t, eax;
        }
        __asm {
            mov ecx, t;
            call dic_key;
            mov edx, [eax];
            mov t, edx;
        }
        __asm {
            mov esi, ecx1;
            lea edx, [temp_shell_target];
            push edx;
            push t;
            call DrawUIElement;
        }
        __asm {
            xor al, al;
            mov ecx, [temp_shell_target];
            call SelectUIElement;
        }
        sh_map.updateShellTarget(i, temp_shell_target);
    }*/
    getShellName();
    std::string sh(shell_name);
    race_string = sh;
    //getRenderPointer();

    /*__asm {
        mov esi, ecx1;
        lea eax, [esi + 0x3C];
        push eax;
        mov eax, offset waaagh_meter_mc;
        call sub_AD3960;
    }
    __asm {
        lea ecx, [esi + 0x38];
        push ecx;
        mov eax, offset waaagh_text;
        call sub_AD3990;
    }

    //determine shell to be drawn
    temp_shell_target = sh_map.getShellTarget(1);
    __asm {
        mov al, 1;
        mov ecx, [temp_shell_target];
        call SelectUIElement;
    }
    //random float values that need to be stored
    __asm {
        mov esi, ecx1;
        mov eax, dword ptr [v18];
        fld DWORD PTR[eax + 0x9bc];
        fstp DWORD PTR[esi + 0x24];
        fld DWORD PTR[eax + 0x9cc];
        fstp DWORD PTR[esi + 0x2c];
    }

    //waaagh_meter_shell drawui
    __asm {
        push offset waaagh_shell;
        lea ecx, [esp + 0x14];
        push ecx;
        call dic_in;
        mov ecx, eax;
        call dic_key;
        mov eax, [eax];
        lea edx, [esi + 0x54];
        push edx;
        push eax;
        call DrawUIElement;
    }
    //global abilities drawui
    __asm {
        push offset global_abilities;
        lea ecx, [esp + 0x14];
        push ecx;
        call dic_in;
        mov ecx, eax;
        call dic_key;
        mov eax, [eax];
        lea edx, [esi + 0x58];
        push edx;
        push eax;
        call DrawUIElement;
        mov v17, eax;
    }
    */
    selection_panel_pointer = ecx1;
    if (sh.compare("race_ork") == 0) {
        BYTE* src = (BYTE*)"\x68\x78\x52\x11\x01";
        MemPatch(reinterpret_cast<BYTE*>(base + 0x76CF4B), src, 5); //ChangeMidShell
        src = (BYTE*)"\x68\xFF\x00\x00\x00";
        MemPatch(reinterpret_cast<BYTE*>(base + 0x76D0A7), src, 5); //TestMidDrawShell
    }
    else {
        //JmpPatch(reinterpret_cast<BYTE*>(base + 0x76CF4B), (DWORD)ChangeMidShell, 5);
        //JmpPatch(reinterpret_cast<BYTE*>(base + 0x76D0A7), (DWORD)TestMidDrawShell, 5);
    }
    __asm {
        mov edi, dic_in;
        mov ebx, dic_key;
        mov esi, ecx1;
    }
    char t = gen_waaagh_target(ecx1);
    return t;
}

//figure out how to properly allocate data in exe, apparently need to add 4 to esp, that fixes crash wtf, ig because of allocate?
//Get select ui element part in
//  -two shells are drawing because we never load gn, figure out why it is still drawing two tf inspect the selectuielemnt execution for new shell
//remove regular shell loadi
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        base = (DWORD)GetModuleHandleA("DOW2.exe");

        waaagh_meter_mc = (const char*)base + 0xD1520C;
        waaagh_text = (const char*)base + 0xD15324;
        waaagh_shell = (const char*)base + 0xD151F8;
        global_abilities = (const char*)base + 0xD14C08;

        gen_waaagh_target = reinterpret_cast<GenerateWaaaghMeterShell>(base + 0x76CF40);
        sub_AD3960 = base + 0x6D3960;
        sub_AD3990 = base + 0x6D3990;
        reveal_waaagh = reinterpret_cast<RevealWaaaghUI>(base + 0x285660);
        revwaaagh = (base + 0x285660);
        sf_widg = reinterpret_cast<CreateSFWidget>(base + 0x285050);
        sfw = (base + 0x285050);
        //testing for the waaagh meter patch
        DrawUIElement = base + 0x6D39C0;
        SelectUIElement = base + 0x285450;
        
        sh_map.addShell("/waaagh_meter_shell/meter_mc/gn");
        //sh_map.addShell((const char*) base + 0xD15278); //sm
        //sh_map.addShell((const char*)base + 0xD15304);//ig
        //sh_map.addShell((const char*) base + 0xD15298); //eld
        //sh_map.addShell((const char*) base + 0xD152BC); //tyr
        //sh_map.addShell((const char*) base + 0xD152E0); //csm
        //used
        

        //new patch stuff
        //used
        addToDic = base + 0x282760;
        
        loadshells_jmpback = base + 0x73C212;
        operator_new = base + 0xB879D0;
        sf_widget_c = base + 0x285050;
        add_to_dic = base + 0x282760;
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x73C208), (DWORD)MidLoadShells, 10);
        
        //new version
        change_jmpback = base + 0x76CF50;
        constant_shell = base + 0xF35720;
        //JmpPatch(reinterpret_cast<BYTE*>(base + 0x76CF4B), (DWORD)ChangeMidShell, 5);
        
        shellget_jmpback = base + 0x76CF74;
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x76CF6F), (DWORD)MidShellGet, 5);
        

        test_jmpback = (base + 0x76D188);
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x76D0A7), (DWORD)TestMidDrawShell, 5);

        mscvr80 = GetModuleHandleA("MSVCR80");
        if (mscvr80) {
            operator_new = reinterpret_cast<DWORD>(GetProcAddress(mscvr80, MAKEINTRESOURCEA(15))); //new in the dll loaded by dow2
        }

        util = GetModuleHandleA("Util.dll");
        if (util) {
            DicInstance = reinterpret_cast<DInstance>(GetProcAddress(util, MAKEINTRESOURCEA(533)));
            DicGetKey = reinterpret_cast<DGetKey>(GetProcAddress(util, MAKEINTRESOURCEA(385)));
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

