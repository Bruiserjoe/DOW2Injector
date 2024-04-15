#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include "ASMBasicFuncs.h"

DWORD MainAddress = 0;
DWORD jmpBack_ASM_ReplayFix = 0;
DWORD jmpBack_ASM_ReplayFix_SkipOver6 = 0;


__declspec(naked) void ASM_ReplyFix() {
    __asm
    {
        cmp eax, 0xFFFFFFFF
        je SkipOver6
        mov ecx, edi
        imul ecx, ecx, 0x000000D8
        jmp[jmpBack_ASM_ReplayFix]
        SkipOver6:
        jmp[jmpBack_ASM_ReplayFix_SkipOver6]
    }
}

void InitPatch()
{
    printf("[8PReplayFix] - Patch start.\n");
    DWORD base = (DWORD)GetModuleHandleA("DOW2.exe");
    if (base)
    {
        jmpBack_ASM_ReplayFix = base + 0x00766F1B;
        jmpBack_ASM_ReplayFix_SkipOver6 = base + 0x00766F6D;
        MainAddress = base + 0x00766F13;
        PatchJmp(MainAddress, ASM_ReplyFix, 3);
        printf("[8PReplayFix] - Patch successfully applied.\n");
    }
    else
    {
        printf("[8PReplyFix] - GetModuleHandleA for DOW2.exe returned null.\n");
    }
   
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) 
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        InitPatch();
        return true;
    }

}