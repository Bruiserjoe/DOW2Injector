#include "framework.h"

DWORD base;
DWORD cmpAbilitiesAddress;
DWORD cmpGlobalAbilitiesAddress;
DWORD cmpMiniPortraitAddress;
DWORD cmpMiniPortraitObserverAddress;
DWORD cmpSquadTabsAddress;
DWORD cmpBuildQueueUIAddress;
DWORD cmpBuildQueueAddress;
DWORD cmpObserverPlayerListAddress;
DWORD cmpObserverTeamListAddress;
DWORD arraySquadTabs1Address;
DWORD FOWDropdownAddress;

BYTE newValue1[] = { 0x15 };
BYTE newValue2[] = { 0x08 };
BYTE newValue3[] = { 0x06 };
BYTE newValue4[] = { 0x00 };


void JmpPatch2(BYTE* src, BYTE* dst, size_t len) {
    DWORD oldProtect;
    VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &oldProtect);

    intptr_t relativeAddress = dst - (src + 5);
    *src = 0xE9;
    *(intptr_t*)(src + 1) = relativeAddress;

    for (size_t i = 5; i < len; ++i) {
        *(src + i) = 0x90;
    }

    VirtualProtect(src, len, oldProtect, &oldProtect);
}

void* AllocateMemoryWithinProcess(size_t size) {
    return VirtualAllocEx(
        GetCurrentProcess(),
        NULL,
        size,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );
}

void MemoryPatches() {
    MemPatch(reinterpret_cast<BYTE*>(cmpAbilitiesAddress), newValue1, sizeof(newValue1));
    MemPatch(reinterpret_cast<BYTE*>(cmpGlobalAbilitiesAddress), newValue1, sizeof(newValue1));
    MemPatch(reinterpret_cast<BYTE*>(cmpMiniPortraitObserverAddress), newValue1, sizeof(newValue1));
    MemPatch(reinterpret_cast<BYTE*>(cmpMiniPortraitAddress), newValue1, sizeof(newValue1));
    MemPatch(reinterpret_cast<BYTE*>(cmpSquadTabsAddress), newValue1, sizeof(newValue1));
    MemPatch(reinterpret_cast<BYTE*>(cmpBuildQueueUIAddress), newValue1, sizeof(newValue1));
    MemPatch(reinterpret_cast<BYTE*>(cmpBuildQueueAddress), newValue3, sizeof(newValue3));
    MemPatch(reinterpret_cast<BYTE*>(cmpObserverPlayerListAddress), newValue2, sizeof(newValue2));
    MemPatch(reinterpret_cast<BYTE*>(cmpObserverTeamListAddress), newValue2, sizeof(newValue2));
    MemPatch(reinterpret_cast<BYTE*>(FOWDropdownAddress), newValue4, sizeof(newValue4));
}

BOOL PatchArrayInitialization(HANDLE hProcess, DWORD_PTR arrayBaseAddress) {
    size_t patchSize = 9 * 3; // 9 new entries, each instruction is 3 bytes long
    BYTE* newInstructions = (BYTE*)AllocateMemoryWithinProcess(patchSize);

    if (newInstructions) {
        DWORD oldProtect;
        if (!VirtualProtectEx(hProcess, newInstructions, patchSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            return FALSE; // Failed to change memory protection
        }

        BYTE* currentInstruction = newInstructions;
        for (int i = 0; i < 9; ++i) {
            // Construct the mov instruction (3 bytes)
            *currentInstruction = 0xC7; // opcode for 'mov reg/mem, imm32'
            *(currentInstruction + 1) = 0x45; // modrm byte for 'ebp + disp8'
            *(currentInstruction + 2) = 0x50 + (i + 1) * 4; // disp8 (offset from ebp), starting at 0x54

            // Move to the next instruction slot, accounting for the 4-byte immediate value that follows
            currentInstruction += 7;
        }

        JmpPatch2((BYTE*)(base + arraySquadTabs1Address), newInstructions, patchSize);

        if (!VirtualProtectEx(hProcess, newInstructions, patchSize, oldProtect, &oldProtect)) {
            return FALSE; // Failed to restore original memory protection
        }
        return TRUE;
    }
    return FALSE;
}

void InitPatch()
{
    base = (DWORD)GetModuleHandleA("DOW2.exe");
    if (base)
    {
        cmpAbilitiesAddress = (base + 0x00739FA6);
        cmpGlobalAbilitiesAddress = (base + 0x0073A70D);
        cmpMiniPortraitObserverAddress = (base + 0x0073D7C6);
        cmpMiniPortraitAddress = (base + 0x0073B486);
        cmpSquadTabsAddress = (base + 0x007487DE);
        cmpBuildQueueUIAddress = (base + 0x0073B6F5);
        cmpBuildQueueAddress = (base + 0x007715A0);
        cmpObserverPlayerListAddress = (base + 0x0073D937);
        cmpObserverTeamListAddress = (base + 0x0073D94B);
        arraySquadTabs1Address = (base + 0x00748779 + 3);
        FOWDropdownAddress = (base + 0x006D8006);
        MemoryPatches();
        //PatchArrayInitialization(GetCurrentProcess(), arraySquadTabs1Address);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        InitPatch();
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}