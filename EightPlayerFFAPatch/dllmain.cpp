// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"

DWORD base = 0;
DWORD slots_root = 0;

DWORD team_setup_jmp_back = 0;
DWORD populate_slots_jmp_back = 0;
DWORD observer_dropdown = 0;
DWORD observer_call_jmp_back = 0;
DWORD closed_slot_path = 0;
DWORD observer_condition_execute = 0;
DWORD observer_condition_skip = 0;

DWORD team_setup_hook = 0;
DWORD populate_slots_hook = 0;
DWORD observer_call_hook = 0;
DWORD observer_condition_hook = 0;
bool patches_applied = false;

const BYTE team_setup_original[] = { 0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8 };
const BYTE populate_slots_original[] = { 0xC7, 0x45, 0xB4, 0x06, 0x00, 0x00, 0x00 };
const BYTE observer_dropdown_original[] = { 0xE8, 0xD7, 0x00, 0x00, 0x00 };
const BYTE observer_condition_original[] = {
    0x80, 0x7D, 0x0C, 0x00, 0x74, 0x12,
    0x80, 0x7D, 0x10, 0x00, 0x75, 0x0C
};

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

static bool ResolveAddresses(HMODULE module)
{
    BYTE* image = nullptr;
    size_t image_size = 0;
    DWORD globals_anchor = 0;
    DWORD populate_anchor = 0;
    DWORD observer_anchor = 0;

    static const char globals_pattern[] =
        "\xA1\x00\x00\x00\x00\x8B\x15\x00\x00\x00\x00\x05\x7C\x01"
        "\x00\x00\x50\xE8\x00\x00\x00\x00\xC3";
    static const char globals_mask[] = "x????xx????xxxxxxx????x";
    static const char team_pattern[] =
        "\x55\x8B\xEC\x83\xE4\xF8\x6A\xFF\x68\x00\x00\x00\x00\x64"
        "\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x81"
        "\xEC\x50\x07\x00\x00";
    static const char team_mask[] =
        "xxxxxxxxx????xxxxxxxxxxxxxxxxxxxx";
    static const char populate_pattern[] =
        "\xE8\x00\x00\x00\x00\x84\xC0\x74\x00\xC7\x45\xB4\x06\x00"
        "\x00\x00\xC7\x45\xC4\x01\x00\x00\x00\xEB\x00";
    static const char populate_mask[] = "x????xxx?xxxxxxxxxxxxxxx?";
    static const char observer_pattern[] =
        "\x80\x7C\x24\x14\x00\x0F\x84\x00\x00\x00\x00\x0F\xB6\x46"
        "\x74\x50\xFF\x75\x08\xE8\x00\x00\x00\x00\xE9\x00\x00\x00"
        "\x00\x80\x7C\x24\x14\x00\x0F\x84\x00\x00\x00\x00\xFF\x74"
        "\x24\x10\xFF\x75\x08\xE8\x00\x00\x00\x00\xE9\x00\x00\x00"
        "\x00";
    static const char observer_mask[] =
        "xxxxxxx????xxxxxxxxx????x????xxxxxxx????xxxxxxxx????x????";
    static const char condition_pattern[] =
        "\x80\x7D\x0C\x00\x74\x00\x80\x7D\x10\x00\x75\x00\x8D\x45"
        "\xCC\x50\xFF\x75\x08\xE8\x00\x00\x00\x00\x68\xFD\x77\x08"
        "\x00";
    static const char condition_mask[] =
        "xxxxx?xxxxx?xxxxxxxx????xxxxx";

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
            team_pattern,
            team_mask,
            team_setup_hook)
        || !FindUnique(
            image,
            image_size,
            populate_pattern,
            populate_mask,
            populate_anchor)
        || !FindUnique(
            image,
            image_size,
            observer_pattern,
            observer_mask,
            observer_anchor)
        || !FindUnique(
            image,
            image_size,
            condition_pattern,
            condition_mask,
            observer_condition_hook)) {
        return false;
    }

    slots_root = *reinterpret_cast<const DWORD*>(globals_anchor + 1);
    populate_slots_hook = populate_anchor + 0x09;
    observer_call_hook = observer_anchor + 0x2F;

    if (!slots_root
        || memcmp(
            reinterpret_cast<const void*>(team_setup_hook),
            team_setup_original,
            sizeof(team_setup_original)) != 0
        || memcmp(
            reinterpret_cast<const void*>(populate_slots_hook),
            populate_slots_original,
            sizeof(populate_slots_original)) != 0
        || memcmp(
            reinterpret_cast<const void*>(observer_call_hook),
            observer_dropdown_original,
            sizeof(observer_dropdown_original)) != 0
        || memcmp(
            reinterpret_cast<const void*>(observer_condition_hook),
            observer_condition_original,
            sizeof(observer_condition_original)) != 0
        || *reinterpret_cast<const BYTE*>(observer_call_hook) != 0xE8) {
        return false;
    }

    team_setup_jmp_back = team_setup_hook + sizeof(team_setup_original);
    populate_slots_jmp_back =
        populate_slots_hook + sizeof(populate_slots_original);
    observer_dropdown = DecodeRelativeTarget(observer_call_hook);
    observer_call_jmp_back =
        observer_call_hook + sizeof(observer_dropdown_original);
    closed_slot_path = observer_anchor + 0x0B;
    observer_condition_execute = observer_condition_hook + 0x0C;
    observer_condition_skip = observer_condition_hook + 0x18;
    return true;
}
#endif

static bool FailInitialization(const char* message)
{
    OutputDebugStringA(message);
    OutputDebugStringA("\n");
    return false;
}

void __declspec(naked) TeamSetupPatch() {
    __asm {
        pushfd;
        push eax;
        push ecx;
        push edx;

        mov eax, dword ptr [slots_root];
        mov eax, dword ptr [eax];
        test eax, eax;
        jz two_teams;
        mov eax, dword ptr [eax + 0x17C];

        mov ecx, dword ptr [esp + 0x14];
        test ecx, ecx;
        jz two_teams;
        mov edx, dword ptr [ecx - 0x0C];

        test edx, 0x600;
        jnz split_teams;
        test edx, 0x180;
        jnz store_teams;

    two_teams:
        mov eax, 2;
        jmp store_teams;

    split_teams:
        shr eax, 1;

    store_teams:
        mov dword ptr [esp + 0x20], eax;

        pop edx;
        pop ecx;
        pop eax;
        popfd;

        push ebp;
        mov ebp, esp;
        and esp, 0xFFFFFFF8;
        jmp dword ptr [team_setup_jmp_back];
    }
}

void __declspec(naked) PopulateSlotsPatch() {
    __asm {
        pushfd;
        push eax;

        mov eax, dword ptr [slots_root];
        mov eax, dword ptr [eax];
        test eax, eax;
        jz six_slots;
        cmp dword ptr [eax + 0x17C], 8;
        jl six_slots;

        mov dword ptr [ebp - 0x4C], 8;
        jmp populate_done;

    six_slots:
        mov dword ptr [ebp - 0x4C], 6;

    populate_done:
        pop eax;
        popfd;
        jmp dword ptr [populate_slots_jmp_back];
    }
}

void __declspec(naked) ObserverDropdownPatch() {
    __asm {
        pushfd;
        push eax;

        mov eax, dword ptr [slots_root];
        mov eax, dword ptr [eax];
        test eax, eax;
        jz use_observer_dropdown;
        cmp dword ptr [eax + 0x17C], 8;
        jl use_observer_dropdown;

        pop eax;
        popfd;
        add esp, 8;
        jmp dword ptr [closed_slot_path];

    use_observer_dropdown:
        pop eax;
        popfd;
        call dword ptr [observer_dropdown];
        jmp dword ptr [observer_call_jmp_back];
    }
}

void __declspec(naked) ObserverConditionPatch() {
    __asm {
        push eax;

        mov eax, dword ptr [slots_root];
        mov eax, dword ptr [eax];
        test eax, eax;
        jz check_original_conditions;
        cmp dword ptr [eax + 0x17C], 8;
        jge execute_observer_code;

    check_original_conditions:
        pop eax;
        cmp byte ptr [ebp + 0x0C], 0;
        je skip_observer_code;
        cmp byte ptr [ebp + 0x10], 0;
        jne skip_observer_code;
        jmp dword ptr [observer_condition_execute];

    execute_observer_code:
        pop eax;
        jmp dword ptr [observer_condition_execute];

    skip_observer_code:
        jmp dword ptr [observer_condition_skip];
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        base = (DWORD)GetModuleHandleA("DOW2.exe");
        if (!base) {
            return FailInitialization(
                "[EightPlayerFFAPatch] DOW2.exe was not loaded.");
        }

#ifdef DOW2_SIGSCAN
        if (!ResolveAddresses(reinterpret_cast<HMODULE>(base))) {
            return FailInitialization(
                "[EightPlayerFFAPatch] Signature resolution failed.");
        }
#else
        slots_root = base + 0x00F35A78;

        team_setup_hook = base + 0x39DF40;
        team_setup_jmp_back = base + 0x39DF46;

        populate_slots_hook = base + 0x9148C;
        populate_slots_jmp_back = base + 0x91493;

        observer_dropdown = base + 0x91FC2;
        observer_call_jmp_back = base + 0x91EEB;
        closed_slot_path = base + 0x91EC2;
        observer_call_hook = base + 0x91EE6;

        observer_condition_execute = base + 0x921DC;
        observer_condition_skip = base + 0x921E8;
        observer_condition_hook = base + 0x921D0;
#endif

        if (!JmpPatch(
                reinterpret_cast<BYTE*>(team_setup_hook),
                (DWORD)TeamSetupPatch,
                sizeof(team_setup_original))
            || !JmpPatch(
                reinterpret_cast<BYTE*>(populate_slots_hook),
                (DWORD)PopulateSlotsPatch,
                sizeof(populate_slots_original))
            || !JmpPatch(
                reinterpret_cast<BYTE*>(observer_call_hook),
                (DWORD)ObserverDropdownPatch,
                sizeof(observer_dropdown_original))
            || !JmpPatch(
                reinterpret_cast<BYTE*>(observer_condition_hook),
                (DWORD)ObserverConditionPatch,
                sizeof(observer_condition_original))) {
            return FailInitialization(
                "[EightPlayerFFAPatch] Failed to apply a patch.");
        }
        patches_applied = true;
        break;

    case DLL_PROCESS_DETACH:
        if (lpReserved == nullptr && patches_applied) {
            MemPatch(reinterpret_cast<BYTE*>(observer_condition_hook), observer_condition_original, sizeof(observer_condition_original));
            MemPatch(reinterpret_cast<BYTE*>(observer_call_hook), observer_dropdown_original, sizeof(observer_dropdown_original));
            MemPatch(reinterpret_cast<BYTE*>(populate_slots_hook), populate_slots_original, sizeof(populate_slots_original));
            MemPatch(reinterpret_cast<BYTE*>(team_setup_hook), team_setup_original, sizeof(team_setup_original));
        }
        break;
    }
    return TRUE;
}
