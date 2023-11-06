#pragma once
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <vector>
#include <string>
#include <psapi.h>
#include <detours.h>
#include <algorithm>
#include <fstream>
#include <thread>
#include <mutex>


void MemPatch(BYTE* dst, BYTE* src, size_t size) {
    DWORD prot;
    VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &prot);
    std::memcpy(dst, src, size);
    VirtualProtect(dst, size, prot, &prot);
}

void NopPatch(BYTE* dst, size_t size) {
    DWORD prot;
    VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &prot);
    std::memset(dst, 0x90, size);
    VirtualProtect(dst, size, prot, &prot);
}

bool JmpPatch(BYTE* dst, DWORD target, size_t size) {
    if (size < 5) {
        return false;
    }
    DWORD prot;
    VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &prot);
    std::memset(dst, 0x90, size);
    DWORD relativeaddr = (target - (DWORD)dst) - 5;

    *(dst) = 0xE9;
    *(DWORD*)((DWORD)dst + 1) = relativeaddr;
    VirtualProtect(dst, size, prot, &prot);
    return true;
}
