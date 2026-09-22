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

const BYTE team_setup_original[] = { 0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8 };
const BYTE populate_slots_original[] = { 0xC7, 0x45, 0xB4, 0x06, 0x00, 0x00, 0x00 };
const BYTE observer_dropdown_original[] = { 0xE8, 0xD7, 0x00, 0x00, 0x00 };
const BYTE observer_condition_original[] = {
    0x80, 0x7D, 0x0C, 0x00, 0x74, 0x12,
    0x80, 0x7D, 0x10, 0x00, 0x75, 0x0C
};

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
        slots_root = base + 0x00F35A78;

        team_setup_jmp_back = base + 0x39DF46;
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x39DF40), (DWORD)TeamSetupPatch, 6);

        populate_slots_jmp_back = base + 0x91493;
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x9148C), (DWORD)PopulateSlotsPatch, 7);

        observer_dropdown = base + 0x91FC2;
        observer_call_jmp_back = base + 0x91EEB;
        closed_slot_path = base + 0x91EC2;
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x91EE6), (DWORD)ObserverDropdownPatch, 5);

        observer_condition_execute = base + 0x921DC;
        observer_condition_skip = base + 0x921E8;
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x921D0), (DWORD)ObserverConditionPatch, 12);
        break;

    case DLL_PROCESS_DETACH:
        if (lpReserved == nullptr && base != 0) {
            MemPatch(reinterpret_cast<BYTE*>(base + 0x921D0), observer_condition_original, sizeof(observer_condition_original));
            MemPatch(reinterpret_cast<BYTE*>(base + 0x91EE6), observer_dropdown_original, sizeof(observer_dropdown_original));
            MemPatch(reinterpret_cast<BYTE*>(base + 0x9148C), populate_slots_original, sizeof(populate_slots_original));
            MemPatch(reinterpret_cast<BYTE*>(base + 0x39DF40), team_setup_original, sizeof(team_setup_original));
        }
        break;
    }
    return TRUE;
}
