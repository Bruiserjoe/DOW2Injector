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
    struct shell {
        std::string name;
        DWORD val = 0;
        DWORD* target = &val;
    };

    struct Memb {
        std::string race_name;
        std::string shell_name;
        bool base_shell;
        DWORD base_offset;
        DWORD* target;
    };
    size_t size_before_org = 0;
    std::vector<shell> shell_names; //vector for all new shells added
    //probably use hashmap to lookup the correct shell for each race_
    std::vector<Memb> races;

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
public:
    ShellMap() {
        //races.push_back({ "race_marine", "/waaagh_meter_shell/meter_mc/gn", false, 0, nullptr });
        races.push_back({ "race_marine", "/waaagh_meter_shell/meter_mc/sm", true, 0x40, nullptr });
        //races.push_back({ "race_imperial_guard", "/waaagh_meter_shell/meter_mc/nec", false, 0, nullptr });
        races.push_back({ "race_imperial_guard", "/waaagh_meter_shell/meter_mc/ig", true, 0x44, nullptr });
        races.push_back({ "race_eldar", "/waaagh_meter_shell/meter_mc/eld", true, 0x48, nullptr });
        races.push_back({ "race_chaos", "/waaagh_meter_shell/meter_mc/csm", true, 0x50, nullptr });
        races.push_back({ "race_tyranid", "/waaagh_meter_shell/meter_mc/tyr", true, 0x4C, nullptr });
    }

    void loadFile(std::string path) {
        std::ifstream file;
        file.open(path);
        std::string line;
        while (getline(file, line)) {
            int index = 0;
            std::string race = readBeforeSemiColon(line, &index);
            index++;
            std::string shell = readBeforeSemiColon(line, &index);
            if (!raceContains(shell, race)) {
                races.push_back({ race, shell, false, 0, nullptr });
            }
            addShell(shell);
        }

        file.close();
        size_before_org = shell_names.size();
    }

    //shell logic
    
    int getShellIndex(std::string race_mame) {
        for (int i = 0; i < shell_names.size(); i++) {
            for (auto& j : races) {
                if (j.race_name.compare(race_mame) == 0 && j.shell_name.compare(shell_names[i].name) == 0) {
                    return i;
                }
            }
        }
        return 0;
    }

    int shellNum() {
        return size_before_org;
    }
    int totalShells() {
        return shell_names.size();
    }

    const char* getShell(int index) {
        return shell_names[index].name.c_str();
    }

    void addShell(std::string name) {
        shell_names.push_back({name});
        shell_names[shell_names.size() - 1].target = &shell_names[shell_names.size() - 1].val;
    }

    DWORD* getShellTarget(int index) {
        return shell_names[index].target;
    }

    void updateShellTarget(int index, DWORD* target) {
        shell_names[index].target = target;
    }

    //race stuff
    
    void updateRacePointers() {
        for (int i = 0; i < races.size(); i++) {
            for (int j = 0; j < shell_names.size(); j++) {
                if (races[i].shell_name.compare(shell_names[j].name) == 0 && !races[i].base_shell) {
                    races[i].target = shell_names[j].target;
                }
            }
        }
    }
    
    
    void setRacePointer(std::string race_name, bool base, DWORD* target) {
        for (int i = 0; i < (int)races.size(); i++) {
            if (races[i].race_name.compare(race_name) == 0) {
                if (base) {
                    races[i].base_shell = true;
                    races[i].target = nullptr;
                    races[i].base_offset = (DWORD)target;
                }
                else {
                    races[i].base_shell = false;
                    races[i].target = target;
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

    DWORD getBaseShellOffset(std::string race_name) {
        for (auto& i : races) {
            if (i.race_name.compare(race_name) == 0) {
                return i.base_offset;
            }
        }
        return 0;
    }

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

    std::string lookupShell(std::string race_name) {
        for (auto& i : races) {
            if (i.race_name.compare(race_name) == 0) {
                return i.shell_name;
            }
        }
        return "";
    }
};