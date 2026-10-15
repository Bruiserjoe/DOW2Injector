// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"

DWORD base;
HMODULE util;
HMODULE plat;

std::vector<std::string> shells;

typedef void(__cdecl* Timestampedf)(const char*, ...);
typedef void(__cdecl* Fatalf)(const char*, ...);

Timestampedf Timestampedtracef;
Fatalf Fatal_f;
HMODULE debug;

typedef int(__fastcall* CreateSFWidget)(int e1, const char* a2, int a3);
CreateSFWidget sf_widg = nullptr;
DWORD sfw = 0;

// converting get
// -also need renderpointer to be updated in selectui, selectui section not being called, probably because of how we init the memory
// //   - selectui probably still broken too
// //   - still need to do this ^
// //   - data in dump doesn't seem correct when compared to regular selectui without mod
// //   - data we target in selectui ecx, is what we create with sf widget creation, just save the pointer
// //   - now the issue is only rendering the /gn/ shell,
// //   - issue is the race string being empty because our detour isn't being called, hook, convert to regular patch?
// //   - missing the default races in loop
// - 0x00685450 is crash, when we are grabbing 0x209 from ecx
// - now it's 0x34, think we are doing the ptrs wrong
// - maybe midshell get??
// global map vars
size_t shell_map_size = 0;
size_t size_before_org = 0;
raw_shell* shell_names;
std::vector<Memb> races;
size_t raw_shell_size = 12;


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


std::string lookupShell(std::string race_name) {
    for (auto& i : races) {
        if (i.race_name.compare(race_name) == 0) {
            return i.shell_name;
        }
    }
    return "";
}

//shell generation hooks


HMODULE mscvr80;

DWORD loadshells_jmpback = 0;
DWORD loadshells_patch = 0;
DWORD operator_new = 0;
DWORD sf_widget_c = 0;
DWORD add_to_dic = 0;
const char* shell_sm = nullptr;
const char* shell_eld = nullptr;
const char* shell_tyr = nullptr;
const char* shell_csm = nullptr;
const char* shell_ig = nullptr;


int index_i = 0;
const char* cur_shell_load = nullptr;
char shell_load_index = 110;
//maybe use offset of value in double pointer
void __declspec(naked) MidLoadShells() {
    __asm {
        push edx;
        push edi;
        mov ebx, 0;
        mov edi, dword ptr[shell_names];
    shell_load_loop:
        cmp ebx, [shell_map_size];
        jz end_loop;
        push 0x3C8;
        call operator_new;
        mov esi, eax;
        mov [edi + 0x8], eax; // saving the mem address of shell for our use
        add esp, 4;
        mov dl, shell_load_index;
        mov BYTE PTR[esp + 0x2a4], dl;
        mov edx, [edi];
        mov eax, ebp;
        call sf_widget_c;
        push ebp;
        mov esi, eax;
        mov BYTE PTR[esp + 0x2a8], 0x0;
        call add_to_dic;
        mov dl, shell_load_index;
        inc dl;
        mov shell_load_index, dl;
        add ebx, 1;
        add edi, raw_shell_size;
        jmp shell_load_loop;
    end_loop:
        pop edi;
        pop edx;
    }
     __asm{
        jmp[loadshells_jmpback];
    }
}
bool base_shell_yah = false;
DWORD test_jmpback = 0;
DWORD* render_ptr = nullptr;
DWORD render_offset = 0;
DWORD dic_in = 0;
DWORD dic_key = 0;

char* selection_panel_pointer = 0;

DWORD* lookupShellPointer(std::string race_name) {
    for (auto& i : races) {
        if (i.race_name.compare(race_name) == 0) {
            if (i.base_shell) {
                return (DWORD*)i.base_offset;
            }
            else {
                return i.target;
            }
        }
    }
    return races[0].target;
}
bool lookupBaseShell(std::string race_name) {
    for (auto& i : races) {
        if (i.race_name.compare(race_name) == 0) {
            return i.base_shell;
        }
    }
    return false;
}

DWORD getBaseShellOffset(std::string race_name) {
    for (auto& i : races) {
        if (i.race_name.compare(race_name) == 0) {
            return i.base_offset;
        }
    }
    return 0;
}

extern "C" void getRenderPointer() {
    //sh_map.setRacePointer("race_marine", false, tt);
    for (size_t i = 0; i < races.size(); i++) {
        for (size_t j = 0; j < shell_map_size; j++) {
            if (races[i].shell_name.compare(shell_names[j].name) == 0 && !races[i].base_shell) {
                races[i].target = shell_names[j].target;
            }
        }
    }
    render_ptr = shell_names[0].target;
    render_ptr = (DWORD*)lookupShellPointer(shell_name);
    base_shell_yah = lookupBaseShell(shell_name);
    if (base_shell_yah) {
        render_offset = getBaseShellOffset(shell_name);
    }
}

//drawing hooks
void __declspec(naked) TestMidDrawShell() {
    __asm {
        mov cl, [base_shell_yah];
        cmp cl, 1;
        jnz not_base_shell;
        mov ecx, esi;
        add ecx, render_offset;
        mov ecx, dword ptr[ecx];
        jmp select_ui;
    not_base_shell:
        mov ecx, dword ptr [render_ptr];
    select_ui:
        mov al, 0x1;
        call SelectUIElement;
    }
    __asm{ 
            jmp[test_jmpback];
    }
}


DWORD shellget_jmpback = 0;
DWORD shellget_patch = 0;
DWORD shell_manager_slot = 0;
DWORD* temp_shell_target = 0;
//cur_shell_load is same offset so it draws the same fuckign thing since it uses the offset omfg, I think this is right fufcucjusiih
void __declspec(naked) MidShellGet() {
    __asm {
        push ecx;
        push edx;
        push eax;
        mov ecx, dword ptr [shell_manager_slot];
        mov ecx, dword ptr [ecx];
        mov edx, [ecx + 0x8];
        mov eax, [edx + 0x4];
        mov eax, dword ptr[eax + 0xc8];
        add eax, 0x70;
        mov shell_name, eax;
        pop eax;
        pop edx;
        pop ecx;
    }
    __asm {
        push edx;
        push esi;
        mov esi, dword ptr[shell_names];
        mov edx, 0;
    shell_get_loop:
        cmp edx, [size_before_org];
        jz end_loop;
        push edx;
        push esi; // char*
        lea ecx, [esp + 0x20]; // 0x14 isn't right since we fuck with stack here, should be 0x20
        push ecx;
        call edi; // dic instance
        mov ecx, eax;
        call ebx; // dic get key
        lea ecx, [esi + 8]; // target
        push ecx;
        mov ecx, [eax];
        push ecx;
        call DrawUIElement;
        add esi, raw_shell_size;
        pop edx;
        add edx, 1;
        jmp shell_get_loop;
    end_loop:
        pop esi;
        pop edx;
    }
    __asm {
        push dword ptr [shell_ig]
        jmp[shellget_jmpback];
    }
}
DWORD midshell_jmpback = 0;
DWORD midshell_patch = 0;
void __declspec(naked) MidShellSelect() {
    __asm {
        push edi;
        push ebx;
        mov edi, 0;
        mov ebx, shell_names;
    shell_target_loop:
        cmp edi, [size_before_org];
        jz leave_loop;
        mov ecx, [ebx + 0x8]; // assuming the offset to target is 0x8
        xor al, al;
        call SelectUIElement;
        add ebx, raw_shell_size;
        add edi, 1;
        jmp shell_target_loop;
    leave_loop:
        pop ebx;
        pop edi;
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
DWORD drawshell_patch = 0;




typedef char(__thiscall *GenerateWaaaghMeterShell)(char* ecx);
GenerateWaaaghMeterShell gen_waaagh_target = nullptr;

const char* waaagh_meter_mc = nullptr;
const char* waaagh_text = nullptr;
const char* waaagh_shell = nullptr;
const char* global_abilities = nullptr;

typedef bool(__stdcall* PlatGetOption)(const char* option, char* str, unsigned int size);
PlatGetOption plat_getoption = nullptr;

std::string readBeforeSemiColon(std::string line, int* index) {
    std::string ret = "";
    for (*index; *index < line.size() && line[*index] != ';'; (*index)++) {
        ret.push_back(line[*index]);
    }
    return ret;
}
bool raceContains(std::string shell, std::string race) {
    for (int i = 0; i < races.size(); i++) {
        if (races[i].race_name.compare(race) == 0) {
            races[i].base_shell = false;
            races[i].shell_name = shell;
            return true;
        }
    }
    return false;
}

void addShell(std::string name, std::vector<shell>& temp_shells) {
    temp_shells.push_back({ name });
}

void loadConfig(std::string path) {
    std::ifstream file;
    file.open(path);
    std::string line;
    std::vector<shell> temp_shells;
    races.push_back({ "race_marine", "/waaagh_meter_shell/meter_mc/sm", true, 0x40, nullptr });
    races.push_back({ "race_imperial_guard", "/waaagh_meter_shell/meter_mc/ig", true, 0x44, nullptr });
    races.push_back({ "race_eldar", "/waaagh_meter_shell/meter_mc/eld", true, 0x48, nullptr });
    races.push_back({ "race_chaos", "/waaagh_meter_shell/meter_mc/csm", true, 0x50, nullptr });
    races.push_back({ "race_tyranid", "/waaagh_meter_shell/meter_mc/tyr", true, 0x4C, nullptr });
    while (getline(file, line)) {
        int index = 0;
        std::string race = readBeforeSemiColon(line, &index);
        index++;
        std::string shell = readBeforeSemiColon(line, &index);
        if (!raceContains(shell, race)) {
            races.push_back({ race, shell, false, 0, nullptr });
        }
        temp_shells.push_back({ shell });
    }

    file.close();
    size_before_org = temp_shells.size();
    addShell(shell_sm, temp_shells);
    addShell(shell_ig, temp_shells);
    addShell(shell_eld, temp_shells);
    addShell(shell_tyr, temp_shells);
    addShell(shell_csm, temp_shells);
    // do the raw ptrs now
    raw_shell* r_shells = new raw_shell[temp_shells.size()];
    for (size_t i = 0; i < temp_shells.size(); i++) {
        char* str = new char[temp_shells[i].name.size() + 1];
        strcpy_s(str, temp_shells[i].name.size() + 1, temp_shells[i].name.c_str());
        r_shells[i].name = str;
        r_shells[i].val = temp_shells[i].val;
        r_shells[i].target = &r_shells[i].val;
    }
    shell_names = r_shells;
    shell_map_size = temp_shells.size();
}

#ifdef DOW2_SIGSCAN
static size_t GetModuleImageSize(HMODULE module) {
    if (!module) {
        return 0;
    }
    const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(module);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return 0;
    }
    const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(
        reinterpret_cast<const BYTE*>(module) + dos->e_lfanew);
    return nt->Signature == IMAGE_NT_SIGNATURE ? nt->OptionalHeader.SizeOfImage : 0;
}

static bool ScanUnique(
    BYTE* module,
    size_t module_size,
    const char* pattern,
    const char* mask,
    DWORD* address) {
    const SigScanResult first = SigScan(module, module_size, pattern, mask);
    if (!first.found) {
        return false;
    }
    const size_t next_offset = first.address - reinterpret_cast<DWORD>(module) + 1;
    if (next_offset < module_size) {
        const SigScanResult second =
            SigScan(module + next_offset, module_size - next_offset, pattern, mask);
        if (second.found) {
            return false;
        }
    }
    *address = first.address;
    return true;
}

static DWORD ResolveRelativeCall(DWORD instruction) {
    return instruction + 5 + *reinterpret_cast<int32_t*>(instruction + 1);
}

static bool IsModuleAddress(DWORD address, size_t size, size_t module_size) {
    return address >= base && address + size >= address && address + size <= base + module_size;
}

static bool ResolveAddresses() {
    BYTE* module = reinterpret_cast<BYTE*>(base);
    const size_t module_size = GetModuleImageSize(reinterpret_cast<HMODULE>(base));
    DWORD load_continuation = 0;
    DWORD draw_continuation = 0;

    static const char load_pattern[] =
        "\x68\xC8\x03\x00\x00\xE8\x00\x00\x00\x00\x8B\xF0\x83\xC4\x04"
        "\x89\x74\x24\x14\x85\xF6\xC6\x84\x24\xA4\x02\x00\x00\x5C\x74"
        "\x00\xBA\x00\x00\x00\x00\x8B\xC5\xE8\x00\x00\x00\x00\xEB\x00"
        "\x33\xC0\x55\x8B\xF0\xC6\x84\x24\xA8\x02\x00\x00\x00\xE8\x00"
        "\x00\x00\x00";
    static const char load_mask[] =
        "xxxxxx????xxxxxxxxxxxxxxxxxxxx?x????xxx????x?xxxxxxxxxxxxxx????";
    static const char load_continuation_pattern[] =
        "\x68\xD8\x03\x00\x00\xE8\x00\x00\x00\x00\x83\xC4\x04\x89\x44"
        "\x24\x14\x85\xC0\xC6\x84\x24\xA4\x02\x00\x00\x61\x74\x00\x55"
        "\x50\xBA\x00\x00\x00\x00\xE8\x00\x00\x00\x00";
    static const char load_continuation_mask[] =
        "xxxxxx????xxxxxxxxxxxxxxxxxx?xxx????x????";
    static const char shellget_pattern[] =
        "\x68\x00\x00\x00\x00\x8D\x54\x24\x14\x52\xFF\xD7\x8B\xC8\xFF"
        "\xD3\x8B\x00\x8D\x4E\x44\x51\x50\xE8\x00\x00\x00\x00\x68\x00"
        "\x00\x00\x00";
    static const char shellget_mask[] = "x????xxxxxxxxxxxxxxxxxxx????x????";
    static const char shellselect_pattern[] =
        "\x8B\x4E\x50\x32\xC0\xE8\x00\x00\x00\x00\x8B\x0D\x00\x00\x00"
        "\x00\x8B\x51\x08\x8B\x42\x04\x85\xC0\x0F\x84\x00\x00\x00\x00"
        "\x8B\x80\xC8\x00\x00\x00";
    static const char shellselect_mask[] = "xxxxxx????xx????xxxxxxxxxx????xxxxxx";
    static const char drawshell_pattern[] =
        "\x68\xFF\x00\x00\x00\x83\xC0\x70\x68\x00\x00\x00\x00\x50\xFF"
        "\x15\x00\x00\x00\x00\x83\xC4\x0C\x85\xC0\x75\x00\x8B\x6D\x00"
        "\x85\xED\x74\x00\xB0\x01\x38\x85\x09\x02\x00\x00\x0F\x84\x00"
        "\x00\x00\x00";
    static const char drawshell_mask[] =
        "xxxxxxxxx????xxx????xxxxxx?xxxxxx?xxxxxxxxxx????";
    static const char draw_continuation_pattern[] =
        "\x8B\x44\x24\x10\xD9\x80\xBC\x09\x00\x00\xD9\x5E\x24\xD9\x80"
        "\xCC\x09\x00\x00\xD9\x5E\x2C\x68\x00\x00\x00\x00\x8D\x4C\x24"
        "\x14\x51";
    static const char draw_continuation_mask[] = "xxxxxxxxxxxxxxxxxxxxxxx????xxxxx";

    if (!module_size ||
        !ScanUnique(module, module_size, load_pattern, load_mask, &loadshells_patch) ||
        !ScanUnique(module, module_size, load_continuation_pattern,
            load_continuation_mask, &load_continuation) ||
        !ScanUnique(module, module_size, shellget_pattern, shellget_mask, &shellget_patch) ||
        !ScanUnique(module, module_size, shellselect_pattern, shellselect_mask, &midshell_patch) ||
        !ScanUnique(module, module_size, drawshell_pattern, drawshell_mask, &drawshell_patch) ||
        !ScanUnique(module, module_size, draw_continuation_pattern,
            draw_continuation_mask, &draw_continuation)) {
        return false;
    }

    if (*reinterpret_cast<BYTE*>(loadshells_patch) != 0x68 ||
        *reinterpret_cast<DWORD*>(loadshells_patch + 1) != 0x3C8 ||
        *reinterpret_cast<BYTE*>(shellget_patch) != 0x68 ||
        memcmp(reinterpret_cast<void*>(midshell_patch), "\x8B\x4E\x50\x32\xC0\xE8", 6) != 0 ||
        memcmp(reinterpret_cast<void*>(drawshell_patch), "\x68\xFF\x00\x00\x00", 5) != 0) {
        return false;
    }

    loadshells_jmpback = load_continuation;
    operator_new = ResolveRelativeCall(loadshells_patch + 5);
    shell_sm = reinterpret_cast<const char*>(*reinterpret_cast<DWORD*>(loadshells_patch + 0x20));
    sf_widget_c = ResolveRelativeCall(loadshells_patch + 0x26);
    add_to_dic = ResolveRelativeCall(loadshells_patch + 0x3A);
    shell_eld = reinterpret_cast<const char*>(*reinterpret_cast<DWORD*>(loadshells_patch + 0x5F));
    shell_tyr = reinterpret_cast<const char*>(*reinterpret_cast<DWORD*>(loadshells_patch + 0x9E));
    shell_csm = reinterpret_cast<const char*>(*reinterpret_cast<DWORD*>(loadshells_patch + 0xDD));
    shell_ig = reinterpret_cast<const char*>(*reinterpret_cast<DWORD*>(loadshells_patch + 0x11C));

    shellget_jmpback = shellget_patch + 5;
    const DWORD shellget_ig = *reinterpret_cast<DWORD*>(shellget_patch + 1);
    DrawUIElement = ResolveRelativeCall(shellget_patch + 0x17);

    midshell_jmpback = midshell_patch + 10;
    SelectUIElement = ResolveRelativeCall(midshell_patch + 5);
    shell_manager_slot = *reinterpret_cast<DWORD*>(midshell_patch + 0x0C);

    if (*reinterpret_cast<WORD*>(drawshell_patch + 0x2A) != 0x840F) {
        return false;
    }
    test_jmpback =
        drawshell_patch + 0x30 + *reinterpret_cast<int32_t*>(drawshell_patch + 0x2C);

    const DWORD derived_addresses[] = {
        loadshells_jmpback, operator_new, reinterpret_cast<DWORD>(shell_sm),
        sf_widget_c, add_to_dic, reinterpret_cast<DWORD>(shell_eld),
        reinterpret_cast<DWORD>(shell_tyr), reinterpret_cast<DWORD>(shell_csm),
        reinterpret_cast<DWORD>(shell_ig), shellget_jmpback, DrawUIElement,
        midshell_jmpback, SelectUIElement, shell_manager_slot, test_jmpback
    };
    for (DWORD address : derived_addresses) {
        if (!IsModuleAddress(address, 1, module_size)) {
            return false;
        }
    }
    return shellget_ig == reinterpret_cast<DWORD>(shell_ig) &&
        test_jmpback == draw_continuation &&
        strcmp(shell_sm, "/waaagh_meter_shell/meter_mc/sm") == 0 &&
        strcmp(shell_eld, "/waaagh_meter_shell/meter_mc/eld") == 0 &&
        strcmp(shell_tyr, "/waaagh_meter_shell/meter_mc/tyr") == 0 &&
        strcmp(shell_csm, "/waaagh_meter_shell/meter_mc/csm") == 0 &&
        strcmp(shell_ig, "/waaagh_meter_shell/meter_mc/ig") == 0;
}
#endif

static BOOL FailInitialization(const char* message) {
    if (Fatal_f) {
        Fatal_f("%s", message);
    }
    else {
        MessageBoxA(nullptr, message, "ShellPatch", MB_OK | MB_ICONERROR);
    }
    return FALSE;
}

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
        raw_shell_size = sizeof(raw_shell);
        plat = GetModuleHandleA("Platform.dll");
        if (plat) {
            plat_getoption = reinterpret_cast<PlatGetOption>(GetProcAddress(plat, MAKEINTRESOURCEA(78)));
        }
        debug = GetModuleHandleA("Debug.dll");
        if (debug) {
            Timestampedtracef = reinterpret_cast<Timestampedf>(GetProcAddress(debug, MAKEINTRESOURCEA(50)));
            Fatal_f = reinterpret_cast<Fatalf>(GetProcAddress(debug, MAKEINTRESOURCEA(31)));
        }
        if (!base || !plat_getoption || !Timestampedtracef) {
            return FailInitialization("Shell Patch: required game modules or exports were not found.");
        }
#ifdef DOW2_SIGSCAN
        if (!ResolveAddresses()) {
            return FailInitialization("Shell Patch: required DOW2.exe signatures were not found uniquely.");
        }
#else
        shell_sm = reinterpret_cast<const char*>(base + 0xD15278);
        shell_ig = reinterpret_cast<const char*>(base + 0xD15304);
        shell_eld = reinterpret_cast<const char*>(base + 0xD15298);
        shell_tyr = reinterpret_cast<const char*>(base + 0xD152BC);
        shell_csm = reinterpret_cast<const char*>(base + 0xD152E0);
        shell_manager_slot = 0x01335720;
        DrawUIElement = base + 0x6D39C0;
        SelectUIElement = base + 0x285450;
        loadshells_patch = base + 0x73C18A;
        loadshells_jmpback = base + 0x73C2C5;
        operator_new = base + 0xB879D0;
        sf_widget_c = base + 0x285050;
        add_to_dic = base + 0x282760;
        midshell_patch = base + 0x76D023;
        midshell_jmpback = base + 0x76D02D;
        shellget_patch = base + 0x76CF6F;
        shellget_jmpback = base + 0x76CF74;
        drawshell_patch = base + 0x76D0A7;
        test_jmpback = base + 0x76D18F;
#endif
        //getting the module name
        ret = plat_getoption("modname", mod1, 0x200);
        modu = std::string(mod1);
        modu = modu + ".shells";

        // sh_map.loadFile(modu, &size_before_org);
        loadConfig(modu);
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
        //sh_map.addShell("/waaagh_meter_shell/meter_mc/gn");
        //sh_map.addShell("/waaagh_meter_shell/meter_mc/nec");
       /* sh_map.addShell((const char*)base + 0xD15278); //sm
        sh_map.addShell((const char*)base + 0xD15304);//ig
        sh_map.addShell((const char*) base + 0xD15298); //eld
        sh_map.addShell((const char*) base + 0xD152BC); //tyr
        sh_map.addShell((const char*) base + 0xD152E0); //csm*/
        //used
        
        //new patch stuff
        //used
        addToDic = base + 0x282760;

        mscvr80 = GetModuleHandleA("MSVCR80");
        if (mscvr80) {
            const DWORD resolved_operator_new =
                reinterpret_cast<DWORD>(GetProcAddress(mscvr80, MAKEINTRESOURCEA(15)));
#ifdef DOW2_SIGSCAN
            if (resolved_operator_new) {
                operator_new = resolved_operator_new;
            }
#else
            operator_new = resolved_operator_new;
#endif
        }
        
        if (!JmpPatch(reinterpret_cast<BYTE*>(loadshells_patch), (DWORD)MidLoadShells, 5)) {
            return FailInitialization("Shell Patch: failed to install shell-loading hook.");
        }
        
        //new version
        
        if (!JmpPatch(reinterpret_cast<BYTE*>(midshell_patch), (DWORD)MidShellSelect, 10)) {
            return FailInitialization("Shell Patch: failed to install shell-selection hook.");
        }

        if (!JmpPatch(reinterpret_cast<BYTE*>(shellget_patch), (DWORD)MidShellGet, 5)) {
            return FailInitialization("Shell Patch: failed to install shell-get hook.");
        }
        

        if (!JmpPatch(reinterpret_cast<BYTE*>(drawshell_patch), (DWORD)TestMidDrawShell, 5)) {
            return FailInitialization("Shell Patch: failed to install shell-draw hook.");
        }

        util = GetModuleHandleA("Util.dll");
        if (util) {
            dic_in = reinterpret_cast<DWORD>(GetProcAddress(util, MAKEINTRESOURCEA(533)));
            dic_key = reinterpret_cast<DWORD>(GetProcAddress(util, MAKEINTRESOURCEA(385)));;
        }
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
