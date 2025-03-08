// dllmain.cpp : Defines the entry point for the DLL application.
#include "dllmain.h"

//0x004132DA is causing crash on skirmish load

typedef void(__cdecl* Timestampedf)(const char*, ...);
typedef void(__cdecl* Fatalf)(const char*, ...);

Timestampedf Timestampedtracef;
Fatalf Fatal_f;

int lobby_slots = 0;
DWORD base;
HMODULE plat;
HMODULE debug;
DWORD slots_addr;
char g_ffa = 0;
char g_tffa = 0;

size_t cur_index = 0;

//returns a pointer of some kind
typedef DWORD32* (__thiscall* setGamemode)(DWORD32 index, DWORD32* address, char* a_struct);
setGamemode setgame_target = reinterpret_cast<setGamemode>(0x004882c6); //function before hook

GamemodeMap map;

//Exception thrown at 0x00488164 in DOW2.exe: 0xC0000005: Access violation reading location 0x3E70F050.
//caused by wrong handling of parameters

//http://jbremer.org/x86-api-hooking-demystified/
//https://www.x86matthew.com/view_post?id=stealth_hook

//https://www.unknowncheats.me/forum/c-and-c-/154364-detourfunction-__thiscall.html
//https://www.unknowncheats.me/forum/programming-for-beginners/424330-hook-functions.html
//https://guidedhacking.com/threads/introduction-to-calling-conventions-for-beginners.20041/
// https://www.tripwire.com/state-of-security/ghidra-101-creating-structures-in-ghidra

Mode cur_mode;
DWORD32 jmpback_setGamemodeHook = 0;
void __declspec(naked) setGamemodeHook() {
    __asm {
        mov cur_index, ecx;
    }
    cur_mode = map.getMode(cur_index);
    g_ffa = cur_mode.ffa;
    g_tffa = cur_mode.t_ffa;
    __asm {
        mov dl, [g_ffa];
        mov byte ptr[ebx + 0x5b], dl;
        mov dl, [g_tffa];
        mov byte ptr[ebx + 0x5c], dl;
        mov dword ptr[ebx + 0x50], edx;
        mov dword ptr[ebx + 0x54], ecx;
    }
    __asm {
        jmp[jmpback_setGamemodeHook];
    }
}


//hook the function which edits the displayed gamemode value and save that string for use

typedef void(__stdcall* GamemodeChange)();
GamemodeChange gc = reinterpret_cast<GamemodeChange>(0x00486f6a);

//recreate locstring so we can get the locstring value from ebp like in possiblegamemodestring



//PossibleMapListGFXGlobal + 0x30 is last stand maps
//PossibleMapListGFXGlobal + 0xc is regular pvp maps
//PossibleMapListGFXGlobal + 0x3c is ffa maps
//PossibleMapListGFXGlobal + 0x18 is benchmark maps?
//PossibleMapListGFXGlobal + 0x0 is campaign maps

//function to be mempatched into MultiplayerUpdateMapList
//https://www.unknowncheats.me/forum/c-and-c-/67884-mid-function-hook-deal.html
//https://www.youtube.com/watch?v=jTl3MFVKSUM


struct MapAddr {
    size_t g_index; //game index
    std::string path; //file path
};

DWORD offset_1;
DWORD32 campaign_maps;
std::vector<MapAddr> map_lists;

size_t generateMapList(std::string file_path, size_t g_index) {


    void* dat = std::malloc(100); //no idea how big this has to be
    DWORD32 t = (DWORD32)dat; //can't use the default map list since it seems to already have it packed tight
    char* d = (char*)t;
    *(DWORD32*)(t + 0x48) = 0;
    //this works now?
    ldmaps_org((char*)file_path.c_str(), (void*)t); //figure out why we get ntl error here, probably arguments


    DWORD32* tp = mpdrp_org((d + 4), (int)(d + 4)); //begin pointer
    //loop to set drop down buttons?
    for (DWORD32* i = tp; i != (DWORD32*)(d + 0x4); i += 0x1eb) {
        //do the map cleanupread function
        drpadd_org((int)i);
    }
    //(DWORD32*)(t + 4) = tp;
    map_lists.push_back({ g_index, file_path });
    Timestampedtracef("GAMEMODE PATCH: Successfully created new map list!");
    return map_lists.size() - 1;
}


extern "C" DWORD32 getMapList() {
    size_t l = 0;
    for (auto& i : map_lists) {
        if (i.g_index == cur_index) {
            DWORD32 t = 0;
            __asm {
                mov edx, dword ptr[campaign_maps];
                add edx, 0x4C;
                add edx, 4;
                mov t, edx;
            }
            t += (l * 12);
            return t;
        }
        l++;
    }
    DWORD32 t = 0;
    if (g_ffa == 0 && g_tffa == 0) {
        char* data;
        __asm {
            push edx;
            mov edx, dword ptr[campaign_maps];
            add edx, 0xc;
            mov data, edx;
            mov t, edx;
            pop edx;
        }
    }
    else {
        __asm {
            push edx;
            mov edx, dword ptr[campaign_maps];
            add edx, 0x3c;
            mov t, edx;
            pop edx;
        }
    }

    return t;
}

DWORD jmpbackaddr;
void __declspec(naked) ReplaceAddr() {

__asm {

    push edx;
    //push esi;
    //mov edx, dword ptr [offset_1];
    //mov edx, dword ptr[edx];
    //mov edx, dword ptr[edx];
    //mov edx, dword ptr [campaign_maps]
    // mov edx, pvp_maps;

    //mov eax, edx;
    call getMapList;
    //add eax, 0xc;
    //mov esi, [ffa_maps];
    //pop esi;
    pop edx;
    jmp[jmpbackaddr];
}
}


//rewrite lobbyslottypeset to mess with team setup - 00447b9a
std::string getList(std::string line) {
    size_t pos = line.find("list:");
    pos += 5;
    std::string str;
    for (pos; pos < line.size() && line[pos] != ';'; pos++) {
        if (line[pos] != ' ' && line[pos] != '\t') {
            str.push_back(line[pos]);
        }
    }
    if (str.compare("default") != 0) {
        str = "Data:Maps/" + str + "/";
    }
    return str;
}

size_t getIndex(std::string line) {
    size_t i;
    std::string parse;
    for (i = 0; i < line.size() && line[i] != ':'; i++) {
        parse.push_back(line[i]);
    }
    i = std::stoi(parse);
    return i;
}

void readListConfig(std::string path) {
    std::ifstream file;
    file.open(path);
    std::string line;
    size_t count = 0;
    while (getline(file, line)) {
        std::string list = getList(line);
        if (list.compare("default") != 0) {
            map_lists.push_back({ getIndex(line), list });
            Timestampedtracef("GAMEMODE PATCH: Successfully created new map list!");
        }
    }
    file.close();
}
//workspace for infinite player slots

typedef bool(__stdcall *PlatGetOption)(const char* option, char* str, unsigned int size);
PlatGetOption plat_getoption = nullptr;

//use modname to select cfg file to use in main dow2 folder, look at initmodule
BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    std::ofstream f;
    std::string modu;
    char mod1[0x200];
    bool ret = false;
    switch (dwReason) {
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
        Timestampedtracef("GAMEMODE PATCH: Injection start!");
        //getting the module name
        ret = plat_getoption("modname", mod1, 0x200);
        modu = std::string(mod1);
        modu = modu + ".gamemodes";
        setgame_target = reinterpret_cast<setGamemode>(base + 0x882c6);
        //mod_org = reinterpret_cast<ModAssignPlayers>(base + 0x39e880);
        offset_1 = base + 0xf357a0;
        campaign_maps = *((DWORD32*)*((DWORD32*)offset_1)) + 0x0;



        jmpbackaddr = (base + 0x86e58);
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x86e47), (DWORD)ReplaceAddr, 5);
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x86e53), (DWORD)ReplaceAddr, 5);



        //original functions for map stuff
        ldmaps_org = reinterpret_cast<LoadMaps>(base + 0x7a42d0);
        mpdrp_org = reinterpret_cast<MapDropdown>(base + 0x7c351);
        drpadd_org = reinterpret_cast<DropDownAdd>(base + 0x7a2a20);

        readListConfig(modu);
        map = GamemodeMap();
        map.readConfig(modu);
        jmpback_setGamemodeHook = base + 0x882FA;
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x882C6), (DWORD)setGamemodeHook, 5);

        Timestampedtracef("GAMEMODE PATCH: Injection finish!");
        break;
    case DLL_PROCESS_DETACH:
        break;
}


return TRUE;
}