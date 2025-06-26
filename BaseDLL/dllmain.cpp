// dllmain.cpp : Defines the entry point for the DLL application.
#include "dllmain.h"

//0x004132DA is causing crash on skirmish load

//think the shit crashes on clicking spacehulkannihialteteamffa because address is fucked??
//legit no clue!
//why is it that index in particular bro??
//tffa modes are the issue for some reason
//the player index is being corrupted!
//don't know why tho...
//00419FCE
//think I'm corrupting stack maybe?? but weird it
//fix crash on replaceaddr new map lists by setting offset properly bruh
//current loading crash is from mapload
//figure out why!
//also there is a weird crash from shellpatch!

typedef void(__cdecl* Timestampedf)(const char*, ...);
typedef void(__cdecl* Fatalf)(const char*, ...);

Timestampedf Timestampedtracef;
Fatalf Fatal_f;

int lobby_slots = 0;
DWORD base;
HMODULE plat;
HMODULE debug;
DWORD slots_addr;
size_t cur_index = 0;

//returns a pointer of some kind
typedef DWORD32* (__thiscall* setGamemode)(DWORD32 index, DWORD32* address, char* a_struct);
setGamemode setgame_target = reinterpret_cast<setGamemode>(0x004882c6); //function before hook

Mode map[100];
extern "C" Mode* g_map = nullptr;
//Exception thrown at 0x00488164 in DOW2.exe: 0xC0000005: Access violation reading location 0x3E70F050.
//caused by wrong handling of parameters

//http://jbremer.org/x86-api-hooking-demystified/
//https://www.x86matthew.com/view_post?id=stealth_hook

//https://www.unknowncheats.me/forum/c-and-c-/154364-detourfunction-__thiscall.html
//https://www.unknowncheats.me/forum/programming-for-beginners/424330-hook-functions.html
//https://guidedhacking.com/threads/introduction-to-calling-conventions-for-beginners.20041/
// https://www.tripwire.com/state-of-security/ghidra-101-creating-structures-in-ghidra



DWORD32 jmpback_setGamemodeHook = 0;

void __declspec(naked) setGamemodeHook() {
    __asm {
        mov [cur_index], ecx;
        cmp ecx, 100;
        jnb zero_gamemode;
        mov edi, dword ptr ds:[g_map]; //thx chatgpt
        imul edx, ecx, 0x2;
        add edi, edx;
        xor edx, edx;
        mov dl, byte ptr[edi];
        mov dh, byte ptr[edi + 0x1];
        mov byte ptr[eax + 0x5B], dl;
        mov byte ptr[eax + 0x5C], dh;
        jmp skip_gamemode;
    zero_gamemode:
        mov byte ptr[eax + 0x5B], 0x0;
        mov byte ptr[eax + 0x5C], 0x0;
    skip_gamemode:
        mov edx, dword ptr[esp + 0x4];
        mov[eax + 0x50], edx;
        mov[eax + 0x54], ecx;
        xor edi, edi;
        jmp[jmpback_setGamemodeHook];
    }
}


//hook the function which edits the displayed gamemode value and save that string for use


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
    const char* path; //file path
    DWORD32 map_offset;
};

size_t map_size = 0;
MapAddr* map_lists_ptr = nullptr;
MapAddr map_lists[100];
DWORD32 numberofmap_lists = 6;
DWORD32 numberofmaps_total = 6;
MapAddr* un_map_lists_ptr = nullptr;
MapAddr unique_map_lists[100];
std::string map_strings[100];

DWORD jmpbackaddr_mapoffset;

// map loading

void __declspec(naked) ReplaceMapAddr() {
    // get the offset from start
    __asm {
        mov eax, dword ptr ds : [map_lists_ptr] ;
        mov edx, cur_index;
        mov ecx, 0;
    find_map_list:
        cmp ecx, [map_size];
        jz unfound_map_list;
        cmp [eax], edx;
        jz found_map_list;
        add eax, 0xC;
        add ecx, 0x1;
        jmp find_map_list;
    found_map_list:
        mov edx, [eax + 0x8];
        jmp end_loops;
    unfound_map_list:
        cmp byte ptr[ebx + 0x5B], 0x0;
        jnz set_ffa;
        cmp byte ptr[ebx + 0x5C], 0x0;
        jnz set_ffa;
        mov edx, 0xC;
        jmp end_loops;
    set_ffa:
        mov edx, 0x3C;
    end_loops:
        // final step
        mov eax, ds : 0x13357a0;
        mov eax, dword ptr [eax];
        add eax, edx;
        jmp[jmpbackaddr_mapoffset];
    }
}

// two new detours which increase size of memory allocated and then load the map lists into the newly created space

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
std::string gamemodepatch_filepath;
size_t gamemodepatch_maps_count = 0;



size_t getIndex(std::string line) {
    size_t i;
    std::string parse;
    for (i = 0; i < line.size() && line[i] != ':'; i++) {
        parse.push_back(line[i]);
    }
    i = std::stoi(parse);
    return i;
}

//workspace for infinite player slots

typedef bool(__stdcall* PlatGetOption)(const char* option, char* str, unsigned int size);
PlatGetOption plat_getoption = nullptr;
void readListConfig(std::string path) {
    std::ifstream file;
    file.open(path);
    if (!file) {
        gamemodepatch_maps_count = 0x4C;
        return;
    }
    std::string line;
    size_t count = 0;
    DWORD32 cur_offset = 76;
    while (getline(file, line)) {
        std::string list = getList(line);
        if (list.compare("default") != 0) {
            map_strings[count] = list;
            map_lists[count] = { getIndex(line), map_strings[count].c_str(), cur_offset };
            bool not_unique = false;
            for (size_t i = 0; i < gamemodepatch_maps_count; i++) {
                if (strcmp(unique_map_lists[i].path, list.c_str()) == 0) {
                    not_unique = true;
                    break;
                }
            }
            if (!not_unique) {
                map_lists[count] = { getIndex(line), map_strings[count].c_str(), cur_offset };
                unique_map_lists[gamemodepatch_maps_count] = {getIndex(line), map_strings[count].c_str(), cur_offset };
                gamemodepatch_maps_count++;
                numberofmap_lists++;
                numberofmaps_total++;
                cur_offset += 12;
            }
            count++;
            // Timestampedtracef("GAMEMODE PATCH: Successfully created new map list!");
        }
    }
    map_size = count;
    gamemodepatch_maps_count = 0x48 + (gamemodepatch_maps_count * 12) + 4;
    // think the four bytes right after the first 72 are used for smth so fucking smth, so these are going unused
    /*BYTE ss[3] = {(BYTE)'\x83', (BYTE)'\xFB', (BYTE)'\x06'};
    ss[2] = numberofmap_lists;
    // MemPatch(reinterpret_cast<BYTE*>(base + 0x7A392E), ss, 3);
    BYTE ss2[2] = { (BYTE)'\x6A', (BYTE)'\x06' };
    ss2[1] = numberofmap_lists;
    // MemPatch(reinterpret_cast<BYTE*>(base + 0x7A36C4), ss2, 2);*/
    numberofmap_lists = numberofmap_lists - 6;
    file.close();
}
char mod1[0x200];
std::string modu;
extern "C" void readMapList() {
    //getting the module name
    plat_getoption("modname", mod1, 0x200);
    modu = std::string(mod1);
    modu = modu + ".gamemodes";
    readListConfig(modu);
    GamemodeMap::readConfig(modu, map);
}

DWORD32 jmpback_detourMapAlloc = 0;
// detour to allocate more memory to maps list :)))
void __declspec(naked) detourMapAlloc() {
    readMapList();
    __asm {
        mov ebp, DWORD PTR[esp + 0x20];
        push esi;
        push edi;
        push gamemodepatch_maps_count;
        jmp[jmpback_detourMapAlloc];
    }
}

DWORD32 jmpback_detourMapVecAlloc = 0;
void __declspec(naked) detourMapVecAlloc() {
    __asm {
        push numberofmaps_total;
        push 0x0C;
        push esi;
        jmp[jmpback_detourMapVecAlloc];
    }
}

DWORD32 gamemodepatch_map_start = 0;

typedef void(__stdcall* LoadMaps)(char* path, void* param2);
LoadMaps LoadMapFolder = nullptr;
DWORD32 LoadMapFolderSimple = 0;
DWORD32 MapListCopy = 0;
typedef void(__stdcall* MapListFreeSmth)(int a1);
MapListFreeSmth MapListFree = nullptr;
DWORD32 MapListVerify = 0;
DWORD32 MapListFix = 0;
DWORD32 MapListFreeSimple = 0;

DWORD32 jmpback_detourMapLoad = 0;
size_t mapOffsetStart = 0x48;
// detour to load new map lists into our expanded memory
void __declspec(naked) detourMapLoad() {
    __asm {
        mov [esi + 0x40], ebx;
        mov edi, DWORD PTR[ebp + 0x0];
        //mov gamemodepatch_map_start, eax;
        add edi, 0x4C; //the current address
        xor ebx, ebx; // the index into map lists
    load_map_loop:
        cmp ebx, [numberofmap_lists];
        jge exit_loop;
        imul ecx, ebx, 0xC; // get offset into uniq list
        mov eax, dword ptr ds : [un_map_lists_ptr] ; // map list ptr
        add ecx, eax;
        // get char str ptr
        mov ecx, [ecx + 0x4];
        push edi;
        push ecx;
        call LoadMapFolderSimple;
        mov ecx, [edi + 0x4];
        push ecx;
        mov eax, ecx;
        call MapListCopy;
        mov ecx, eax;
        mov eax, [edi + 0x4];
        cmp ecx, eax;
        jz skip_free_loop;
        // now second loop!
        mov edx, ecx;
    free_loop:
        push edx;
        call MapListFree;
        add edx, 0x7AC;
        cmp edx, eax;
        jnz free_loop;
    skip_free_loop:
        mov [edi + 0x4], ecx;
        add ebx, 0x1;
        add edi, 0xC;
        jmp load_map_loop;
    exit_loop:
        xor ebx, ebx;
        jmp[jmpback_detourMapLoad];
    }
}

DWORD32 jmpback_detourMapLoadEndLoop = 0;
void __declspec(naked) detourMapLoadEndLoop() {
    __asm {
        add ebx, 1;
        cmp ebx, numberofmaps_total;
        jmp[jmpback_detourMapLoadEndLoop];
    }
}

//use modname to select cfg file to use in main dow2 folder, look at initmodule
BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
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
        // Timestampedtracef("GAMEMODE PATCH: Injection start!");
        setgame_target = reinterpret_cast<setGamemode>(base + 0x882c6);
        //mod_org = reinterpret_cast<ModAssignPlayers>(base + 0x39e880);


        jmpbackaddr_mapoffset = (base + 0x86E58);
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x86E36), (DWORD)ReplaceMapAddr, 0x1F);

        // map detours, for gamemode patch
        jmpback_detourMapAlloc = base + 0x7A36A2;
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x7A369A), (DWORD)detourMapAlloc, 8);
        // detour vector constructor to make sure the number of 
        jmpback_detourMapVecAlloc = base + 0x7A36C9;
        // JmpPatch(reinterpret_cast<BYTE*>(base + 0x7A36C4), (DWORD)detourMapVecAlloc, 5); unstable
        jmpback_detourMapLoad = base + 0x7A38EA;
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x7A38E5), (DWORD)detourMapLoad, 5);

        // detouring the end of mapLoad cause it has a loop for six map lists when we should be doing all the new ones too
        jmpback_detourMapLoadEndLoop = base + 0x7A3931;
        // JmpPatch(reinterpret_cast<BYTE*>(base + 0x7A392B), (DWORD)detourMapLoadEndLoop, 6); unstable
         
        //original functions for map stuff
        LoadMapFolder = reinterpret_cast<LoadMaps>(base + 0x7a42d0);
        LoadMapFolderSimple = base + 0x7a42d0;
        MapListFree = reinterpret_cast<MapListFreeSmth>(base + 0x7A2A20);
        MapListFreeSimple = (base + 0x7A2A20);
        MapListCopy = base + 0x7C351;
        MapListVerify = base + 0x7A5E20;
        MapListFix = base + 0x7A5B40;
        

        g_map = map;
        un_map_lists_ptr = unique_map_lists;
        map_lists_ptr = map_lists;
        jmpback_setGamemodeHook = base + 0x882FA;
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x882C6), (DWORD)setGamemodeHook, 5);

        // Timestampedtracef("GAMEMODE PATCH: Injection finish!");
        break;
    case DLL_PROCESS_DETACH:
        // Timestampedtracef("GAMEMODE PATCH: DETACHED!!!");
        break;
}


return TRUE;
}