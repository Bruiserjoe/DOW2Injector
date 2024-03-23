#pragma once
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <detours.h>
#include <string>
#include <vector>
#include <fstream>


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

class ShellMap {
private:
    struct Memb {
        std::string race_name;
        std::string shell_name;
        bool base_shell;
        DWORD base_offset;
        DWORD* target;
    };

    //probably use hashmap to lookup the correct shell for each race_
    std::vector<Memb> races;
public:
    ShellMap() {
        races.push_back({ "race_marine", "/waaagh_meter_shell/meter_mc/gn", false, 0, nullptr });
        races.push_back({ "race_imperial_guard", "/waaagh_meter_shell/meter_mc/ig", true, 0x44, nullptr });
        races.push_back({ "race_eldar", "/waaagh_meter_shell/meter_mc/sm", true, 0x40, nullptr });
        races.push_back({ "race_chaos", "/waaagh_meter_shell/meter_mc/csm", true, 0x50, nullptr });
        races.push_back({ "race_tyranid", "/waaagh_meter_shell/meter_mc/tyr", true, 0x4C, nullptr });
    }

    void setRacePointer(std::string race_name, bool base, DWORD* target) {
        for (auto& i : races) {
            if (i.race_name.compare(race_name) == 0) {
                if (base) {
                    i.base_shell = true;
                    i.target = nullptr;
                    i.base_offset = (DWORD)target;
                }
                else {
                    i.base_shell = false;
                    i.target = target;
                }
            }
        }
    }

    bool lookupBaseShell(std::string race_name) {
        for (auto& i : races) {
            if (i.race_name.compare(race_name) == 0) {
                return i.base_shell;
            }
        }
        return false;
    }

    DWORD lookupShellPointer(std::string race_name) {
        for (auto& i : races) {
            if (i.race_name.compare(race_name) == 0) {
                if (i.base_shell) {
                    return i.base_offset;
                }
                else {
                    return (DWORD)i.target;
                }
            }
        }
        return (DWORD)nullptr;
    }

    std::string lookupShell(std::string race_name) {
        for (auto& i : races) {
            if (i.race_name.compare(race_name) == 0) {
                return i.shell_name;
            }
        }
        return "";
    }
};