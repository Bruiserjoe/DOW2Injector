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
DWORD map_lists_root = 0;

DWORD replace_map_hook = 0;
DWORD map_alloc_hook = 0;
DWORD map_load_hook = 0;
DWORD map_destruct_hook = 0;
DWORD set_gamemode_hook = 0;

//returns a pointer of some kind
typedef DWORD32* (__thiscall* setGamemode)(DWORD32 index, DWORD32* address, char* a_struct);
#ifdef DOW2_SIGSCAN
setGamemode setgame_target = nullptr;
#else
setGamemode setgame_target = reinterpret_cast<setGamemode>(0x004882c6); //function before hook
#endif

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

#ifdef DOW2_SIGSCAN
static bool GetImageRange(HMODULE module, BYTE*& start, size_t& size)
{
    if (!module) {
        return false;
    }

    BYTE* image = reinterpret_cast<BYTE*>(module);
    IMAGE_DOS_HEADER* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(image);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }

    IMAGE_NT_HEADERS32* nt =
        reinterpret_cast<IMAGE_NT_HEADERS32*>(image + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }

    start = image;
    size = nt->OptionalHeader.SizeOfImage;
    return size != 0;
}

static bool FindUnique(
    BYTE* image,
    size_t image_size,
    const char* pattern,
    const char* mask,
    DWORD& address)
{
    SigScanResult first = SigScan(image, image_size, pattern, mask);
    if (!first.found) {
        return false;
    }

    size_t next_offset =
        static_cast<size_t>(first.address - reinterpret_cast<DWORD>(image)) + 1;
    if (next_offset < image_size) {
        SigScanResult second =
            SigScan(image + next_offset, image_size - next_offset, pattern, mask);
        if (second.found) {
            return false;
        }
    }

    address = first.address;
    return true;
}

static DWORD DecodeRelativeTarget(DWORD instruction)
{
    return instruction + 5
        + *reinterpret_cast<const LONG*>(instruction + 1);
}

static bool MatchesMaskedBytes(
    DWORD address,
    const BYTE* expected,
    const char* mask)
{
    size_t length = strlen(mask);
    const BYTE* actual = reinterpret_cast<const BYTE*>(address);
    for (size_t i = 0; i < length; i++) {
        if (mask[i] == 'x' && actual[i] != expected[i]) {
            return false;
        }
    }
    return true;
}
#endif

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
        mov eax, dword ptr[map_lists_root];
        mov eax, dword ptr[eax];
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
    map_size = 0;
    gamemodepatch_maps_count = 0x4C;
    numberofmap_lists = 0;
    numberofmaps_total = 6;

    for (size_t i = 0; i < 100; i++) {
        map_strings[i].clear();
        map_lists[i] = {};
        unique_map_lists[i] = {};
    }

    std::ifstream file;
    file.open(path);
    if (!file) {
        return;
    }

    std::string line;
    size_t count = 0;
    size_t unique_count = 0;
    DWORD32 cur_offset = 76;
    while (count < 100 && getline(file, line)) {
        std::string list = getList(line);
        if (list.compare("default") != 0) {
            map_strings[count] = list;
            DWORD32 map_offset = cur_offset;
            bool found = false;

            for (size_t i = 0; i < unique_count; i++) {
                if (strcmp(unique_map_lists[i].path, list.c_str()) == 0) {
                    map_offset = unique_map_lists[i].map_offset;
                    found = true;
                    break;
                }
            }

            if (!found) {
                if (unique_count >= 100) {
                    break;
                }

                unique_map_lists[unique_count] = {
                    getIndex(line),
                    map_strings[count].c_str(),
                    cur_offset
                };
                map_offset = cur_offset;
                unique_count++;
                cur_offset += 12;
            }

            map_lists[count] = {
                getIndex(line),
                map_strings[count].c_str(),
                map_offset
            };
            count++;
        }
    }

    map_size = count;
    numberofmap_lists = static_cast<DWORD32>(unique_count);
    numberofmaps_total = 6 + static_cast<DWORD32>(unique_count);
    gamemodepatch_maps_count = 0x4C + (unique_count * 12);
    // think the four bytes right after the first 72 are used for smth so fucking smth, so these are going unused
    /*BYTE ss[3] = {(BYTE)'\x83', (BYTE)'\xFB', (BYTE)'\x06'};
    ss[2] = numberofmap_lists;
    // MemPatch(reinterpret_cast<BYTE*>(base + 0x7A392E), ss, 3);
    BYTE ss2[2] = { (BYTE)'\x6A', (BYTE)'\x06' };
    ss2[1] = numberofmap_lists;
    // MemPatch(reinterpret_cast<BYTE*>(base + 0x7A36C4), ss2, 2);*/
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
    std::memset(map, 0, sizeof(map));
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
DWORD32 MapListFinalize = 0;
DWORD32 MapListFreeSimple = 0;
DWORD32 MapListDestructor = 0;

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
        mov dword ptr[edi], 0;
        mov dword ptr[edi + 0x4], 0;
        mov dword ptr[edi + 0x8], 0;
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
        push ebx;
        push esi;
        push edi;
        mov eax, edi;
        mov esi, [edi + 0x4];
        mov edi, [edi];
        push 0;
        call MapListFinalize;
        pop edi;
        pop esi;
        pop ebx;
        add ebx, 0x1;
        add edi, 0xC;
        jmp load_map_loop;
    exit_loop:
        xor ebx, ebx;
        jmp[jmpback_detourMapLoad];
    }
}

typedef void(__thiscall* DestroyMapList)(void* map_list);

void __stdcall DestroyCustomMapLists(void* container) {
    if (!container || !MapListDestructor) {
        return;
    }

    DestroyMapList destroy_map_list =
        reinterpret_cast<DestroyMapList>(MapListDestructor);
    BYTE* custom_map_list =
        reinterpret_cast<BYTE*>(container) +
        0x4C +
        (numberofmap_lists * 0x0C);

    for (DWORD32 i = numberofmap_lists; i > 0; i--) {
        custom_map_list -= 0x0C;
        destroy_map_list(custom_map_list);
    }
}

DWORD32 jmpback_detourMapDestruct = 0;
void __declspec(naked) detourMapDestruct() {
    __asm {
        pushfd;
        pushad;
        push edi;
        call DestroyCustomMapLists;
        popad;
        popfd;

        push dword ptr[MapListDestructor];
        jmp[jmpback_detourMapDestruct];
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

#ifdef DOW2_SIGSCAN
static bool ResolveAddresses(HMODULE module)
{
    BYTE* image = nullptr;
    size_t image_size = 0;
    DWORD globals_anchor = 0;
    DWORD replace_anchor = 0;
    DWORD allocation_anchor = 0;
    DWORD processing_anchor = 0;
    DWORD destruction_anchor = 0;

    static const char globals_pattern[] =
        "\xA1\x00\x00\x00\x00\x8B\x15\x00\x00\x00\x00\x05\x7C\x01"
        "\x00\x00\x50\xE8\x00\x00\x00\x00\xC3";
    static const char globals_mask[] = "x????xx????xxxxxxx????x";
    static const char replace_pattern[] =
        "\x83\x4B\x60\xFF\x80\x7B\x5B\x00\x75\x00\x80\x7B\x5C\x00"
        "\x75\x00\xA1\x00\x00\x00\x00\x8B\x00\x83\xC0\x0C\xEB\x00"
        "\xA1\x00\x00\x00\x00\x8B\x00\x83\xC0\x3C\x50\xE8\x00\x00"
        "\x00\x00";
    static const char replace_mask[] =
        "xxxxxxxxx?xxxxx?x????xxxxxx?x????xxxxxxx????";
    static const char allocation_pattern[] =
        "\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50"
        "\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x08\x53\x55\x8B\x6C"
        "\x24\x20\x56\x57\x6A\x4C\xE8\x00\x00\x00\x00";
    static const char allocation_mask[] =
        "xxx????xxxxxxxxxxxxxxxxxxxxxxxxxxxx????";
    static const char processing_pattern[] =
        "\x8B\x75\x00\x8B\x4E\x40\x51\x8B\xC1\xE8\x00\x00\x00\x00"
        "\x8B\xD8\x8B\x46\x40\x3B\xD8\x89\x44\x24\x10\x8B\xFB\x74"
        "\x00\x57\xE8\x00\x00\x00\x00\x81\xC7\xAC\x07\x00\x00\x3B"
        "\x7C\x24\x10\x75\x00\x89\x5E\x40\x33\xDB\x8D\x9B\x00\x00"
        "\x00\x00\x83\xFB\x04";
    static const char processing_mask[] =
        "xxxxxxxxxx????xxxxxxxxxxxxxx?xx????xxxxxxxxxxx?xxxxxxxxxxxxxx";
    static const char destruction_pattern[] =
        "\x8B\x77\x48\x85\xF6\x74\x00\x56\xE8\x00\x00\x00\x00\x56"
        "\xE8\x00\x00\x00\x00\x83\xC4\x04\x68\x00\x00\x00\x00\x6A"
        "\x06\x6A\x0C\x57\xC7\x44\x24\x20\xFF\xFF\xFF\xFF\xE8\x00"
        "\x00\x00\x00";
    static const char destruction_mask[] =
        "xxxxxx?xx????xx????xxxx????xxxxxxxxxxxxxx????";
    static const char gamemode_pattern[] =
        "\x83\xF9\x01\x74\x00\x83\xF9\x02\x74\x00\xC6\x40\x5B\x00"
        "\xC6\x40\x5C\x00\xEB\x00\x83\xF9\x01\xC6\x40\x5B\x00\xC6"
        "\x40\x5C\x00\x75\x00\x88\x48\x5B\xEB\x00\xC6\x40\x5C\x01"
        "\x8B\x54\x24\x04\x89\x50\x50\x89\x48\x54\xC2\x04\x00";
    static const char gamemode_mask[] =
        "xxxx?xxxx?xxxxxxxxx?xxxxxxxxxxxx?xxxx?xxxxxxxxxxxxxxxxx";

    static const BYTE replace_original[] = {
        0x80, 0x7B, 0x5B, 0x00, 0x75, 0x12,
        0x80, 0x7B, 0x5C, 0x00, 0x75, 0x0C,
        0xA1, 0xA0, 0x57, 0x33, 0x01, 0x8B, 0x00,
        0x83, 0xC0, 0x0C, 0xEB, 0x0A,
        0xA1, 0xA0, 0x57, 0x33, 0x01, 0x8B, 0x00
    };
    static const BYTE allocation_original[] =
        { 0x8B, 0x6C, 0x24, 0x20, 0x56, 0x57, 0x6A, 0x4C };
    static const BYTE load_original[] =
        { 0x89, 0x5E, 0x40, 0x33, 0xDB };
    static const BYTE destruction_original[] =
        { 0x68, 0xA4, 0x3A, 0x44, 0x00 };
    static const BYTE gamemode_original[] =
        { 0x83, 0xF9, 0x01, 0x74, 0x0F };

    if (!GetImageRange(module, image, image_size)
        || !FindUnique(
            image,
            image_size,
            globals_pattern,
            globals_mask,
            globals_anchor)
        || !FindUnique(
            image,
            image_size,
            replace_pattern,
            replace_mask,
            replace_anchor)
        || !FindUnique(
            image,
            image_size,
            allocation_pattern,
            allocation_mask,
            allocation_anchor)
        || !FindUnique(
            image,
            image_size,
            processing_pattern,
            processing_mask,
            processing_anchor)
        || !FindUnique(
            image,
            image_size,
            destruction_pattern,
            destruction_mask,
            destruction_anchor)
        || !FindUnique(
            image,
            image_size,
            gamemode_pattern,
            gamemode_mask,
            set_gamemode_hook)) {
        return false;
    }

    replace_map_hook = replace_anchor + 0x04;
    map_alloc_hook = allocation_anchor + 0x1A;
    map_load_hook = processing_anchor + 0x2F;
    map_destruct_hook = destruction_anchor + 0x16;
    map_lists_root = *reinterpret_cast<const DWORD*>(globals_anchor + 7);
    DWORD replace_root_1 =
        *reinterpret_cast<const DWORD*>(replace_anchor + 0x11);
    DWORD replace_root_2 =
        *reinterpret_cast<const DWORD*>(replace_anchor + 0x1D);

    if (!map_lists_root
        || map_lists_root != replace_root_1
        || map_lists_root != replace_root_2
        || !MatchesMaskedBytes(
            replace_map_hook,
            replace_original,
            "xxxxxxxxxxxxx????xxxxxxxx????xx")
        || memcmp(
            reinterpret_cast<const void*>(map_alloc_hook),
            allocation_original,
            sizeof(allocation_original)) != 0
        || memcmp(
            reinterpret_cast<const void*>(map_load_hook),
            load_original,
            sizeof(load_original)) != 0
        || !MatchesMaskedBytes(
            map_destruct_hook,
            destruction_original,
            "x????")
        || memcmp(
            reinterpret_cast<const void*>(set_gamemode_hook),
            gamemode_original,
            sizeof(gamemode_original)) != 0
        || *reinterpret_cast<const BYTE*>(allocation_anchor + 0x6B) != 0xE8
        || *reinterpret_cast<const BYTE*>(allocation_anchor + 0x7C) != 0xE8
        || *reinterpret_cast<const BYTE*>(processing_anchor + 0x09) != 0xE8
        || *reinterpret_cast<const BYTE*>(processing_anchor + 0x1E) != 0xE8
        || *reinterpret_cast<const BYTE*>(processing_anchor + 0x70) != 0xE8
        || *reinterpret_cast<const BYTE*>(map_destruct_hook) != 0x68) {
        return false;
    }

    jmpbackaddr_mapoffset = replace_anchor + 0x26;
    jmpback_detourMapAlloc = allocation_anchor + 0x22;
    jmpback_detourMapLoad = processing_anchor + 0x34;
    jmpback_detourMapDestruct = destruction_anchor + 0x1B;
    jmpback_setGamemodeHook = set_gamemode_hook + 0x34;

    DWORD load_map_folder =
        DecodeRelativeTarget(allocation_anchor + 0x6B);
    if (load_map_folder
        != DecodeRelativeTarget(allocation_anchor + 0x7C)) {
        return false;
    }

    LoadMapFolder = reinterpret_cast<LoadMaps>(load_map_folder);
    LoadMapFolderSimple = load_map_folder;
    MapListCopy = DecodeRelativeTarget(processing_anchor + 0x09);
    MapListFree = reinterpret_cast<MapListFreeSmth>(
        DecodeRelativeTarget(processing_anchor + 0x1E));
    MapListFreeSimple = reinterpret_cast<DWORD>(MapListFree);
    MapListFinalize = DecodeRelativeTarget(processing_anchor + 0x70);
    MapListDestructor =
        *reinterpret_cast<const DWORD*>(map_destruct_hook + 1);
    setgame_target = reinterpret_cast<setGamemode>(set_gamemode_hook);
    return true;
}
#endif

static bool FailInitialization(const char* message)
{
    if (Timestampedtracef) {
        Timestampedtracef("%s", message);
    }
    else {
        OutputDebugStringA(message);
        OutputDebugStringA("\n");
    }
    return false;
}

//use modname to select cfg file to use in main dow2 folder, look at initmodule
BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
        {
        HMODULE dow2 = GetModuleHandleA("DOW2.exe");
        base = reinterpret_cast<DWORD>(dow2);
        if (!base) {
            return FailInitialization(
                "GAMEMODE PATCH: DOW2.exe was not loaded.");
        }

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
#ifdef DOW2_SIGSCAN
        if (!ResolveAddresses(dow2)) {
            return FailInitialization(
                "GAMEMODE PATCH: Signature resolution failed.");
        }
#else
        setgame_target = reinterpret_cast<setGamemode>(base + 0x882c6);
        //mod_org = reinterpret_cast<ModAssignPlayers>(base + 0x39e880);

        replace_map_hook = base + 0x86E36;
        jmpbackaddr_mapoffset = (base + 0x86E58);

        // map detours, for gamemode patch
        map_alloc_hook = base + 0x7A369A;
        jmpback_detourMapAlloc = base + 0x7A36A2;
        // detour vector constructor to make sure the number of 
        jmpback_detourMapVecAlloc = base + 0x7A36C9;
        // JmpPatch(reinterpret_cast<BYTE*>(base + 0x7A36C4), (DWORD)detourMapVecAlloc, 5); unstable
        map_load_hook = base + 0x7A38E5;
        jmpback_detourMapLoad = base + 0x7A38EA;

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
        MapListFinalize = base + 0x7A5C10;
        MapListDestructor = base + 0x43AA4;

        map_destruct_hook = base + 0x7A56A9;
        jmpback_detourMapDestruct = base + 0x7A56AE;

        set_gamemode_hook = base + 0x882C6;
        map_lists_root = base + 0xF357A0;
        g_map = map;
        un_map_lists_ptr = unique_map_lists;
        map_lists_ptr = map_lists;
        jmpback_setGamemodeHook = base + 0x882FA;
#endif

        g_map = map;
        un_map_lists_ptr = unique_map_lists;
        map_lists_ptr = map_lists;

        if (!JmpPatch(
                reinterpret_cast<BYTE*>(replace_map_hook),
                (DWORD)ReplaceMapAddr,
                0x1F)
            || !JmpPatch(
                reinterpret_cast<BYTE*>(map_alloc_hook),
                (DWORD)detourMapAlloc,
                8)
            || !JmpPatch(
                reinterpret_cast<BYTE*>(map_load_hook),
                (DWORD)detourMapLoad,
                5)
            || !JmpPatch(
                reinterpret_cast<BYTE*>(map_destruct_hook),
                (DWORD)detourMapDestruct,
                5)
            || !JmpPatch(
                reinterpret_cast<BYTE*>(set_gamemode_hook),
                (DWORD)setGamemodeHook,
                5)) {
            return FailInitialization(
                "GAMEMODE PATCH: Failed to apply a patch.");
        }

        // Timestampedtracef("GAMEMODE PATCH: Injection finish!");
        break;
        }
    case DLL_PROCESS_DETACH:
        // Timestampedtracef("GAMEMODE PATCH: DETACHED!!!");
        break;
}


return TRUE;
}