#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include "ASMBasicFuncs.h"
#include "..\Profiler\PluginExports.h"

DWORD MainAddress = 0;
DWORD jmpBack_ASM_ReplayFix = 0;
DWORD jmpBack_ASM_ReplayFix_SkipOver6 = 0;

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
#endif


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

bool InitPatch()
{
    printf("[8PReplayFix] - Patch start.\n");
    HMODULE module = GetModuleHandleA("DOW2.exe");
    DWORD base = reinterpret_cast<DWORD>(module);
    if (base)
    {
#ifdef DOW2_SIGSCAN
        BYTE* image = nullptr;
        size_t image_size = 0;
        DWORD replay_anchor = 0;
        static const char replay_pattern[] =
            "\x85\xF6\x8B\xF8\x74\x00\x8B\xCF\x69\xC9\xD8\x00\x00\x00"
            "\x83\xBC\x19\xB0\x00\x00\x00\x00";
        static const char replay_mask[] = "xxxxx?xxxxxxxxxxxxxxxx";
        static const BYTE replay_original[] =
            { 0x8B, 0xCF, 0x69, 0xC9, 0xD8, 0x00, 0x00, 0x00 };

        if (!GetImageRange(module, image, image_size)
            || !FindUnique(
                image,
                image_size,
                replay_pattern,
                replay_mask,
                replay_anchor)
            || *reinterpret_cast<BYTE*>(replay_anchor + 4) != 0x74) {
            printf("[8PReplayFix] - Signature resolution failed.\n");
            return false;
        }

        MainAddress = replay_anchor + 0x06;
        if (memcmp(
                reinterpret_cast<const void*>(MainAddress),
                replay_original,
                sizeof(replay_original)) != 0) {
            printf("[8PReplayFix] - Hook bytes did not validate.\n");
            return false;
        }

        jmpBack_ASM_ReplayFix = MainAddress + sizeof(replay_original);
        const signed char skip_displacement =
            *reinterpret_cast<signed char*>(replay_anchor + 5);
        jmpBack_ASM_ReplayFix_SkipOver6 =
            replay_anchor + 6 + skip_displacement;
#else
        jmpBack_ASM_ReplayFix = base + 0x00766F1B;
        jmpBack_ASM_ReplayFix_SkipOver6 = base + 0x00766F6D;
        MainAddress = base + 0x00766F13;
#endif
        PatchJmp(MainAddress, ASM_ReplyFix, 3);
        printf("[8PReplayFix] - Patch successfully applied.\n");
        return true;
    }
    else
    {
        printf("[8PReplyFix] - GetModuleHandleA for DOW2.exe returned null.\n");
        return false;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) 
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        return InitPatch();
    }

    return TRUE;
}