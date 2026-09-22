#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <cstring>

bool MemPatch(BYTE* dst, const BYTE* src, size_t size) {
    DWORD old_protection;
    if (!VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &old_protection)) {
        return false;
    }

    std::memcpy(dst, src, size);
    FlushInstructionCache(GetCurrentProcess(), dst, size);

    DWORD unused;
    VirtualProtect(dst, size, old_protection, &unused);
    return true;
}

bool JmpPatch(BYTE* dst, DWORD target, size_t size) {
    if (size < 5) {
        return false;
    }
    DWORD old_protection;
    if (!VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &old_protection)) {
        return false;
    }

    std::memset(dst, 0x90, size);
    DWORD relativeaddr = (target - (DWORD)dst) - 5;

    *(dst) = 0xE9;
    *(DWORD*)((DWORD)dst + 1) = relativeaddr;
    FlushInstructionCache(GetCurrentProcess(), dst, size);

    DWORD unused;
    VirtualProtect(dst, size, old_protection, &unused);
    return true;
}