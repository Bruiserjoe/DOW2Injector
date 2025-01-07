#include "Injector.h"




bool Injector::injectDLL(std::string name) {
    pid = GetCurrentProcessId();
    std::cout << "Injecting: " + name << "\n";
    char dll_path[MAX_PATH] = { 0 };
    GetFullPathName(name.c_str(), MAX_PATH, dll_path, NULL);
    if (!file_exists(dll_path)) {
        std::string str = "PROFILER: dll " + name + " is not present";
        Fatal_f(str.c_str());
        return false;
    }

    HMODULE hmod = LoadLibraryA(dll_path);
    modules.push_back({ name, hmod });
    return true;
}


bool Injector::findDLLS(std::string folder) {
    WIN32_FIND_DATA FileData;
    std::string files = folder + "\\*";
    HANDLE hFind = FindFirstFile(files.c_str(), &FileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        Fatal_f("PROFILER: Folder is invalid");
        return false;
    }
    std::cout << "Reading mods folder\n";
    do
    {
        std::string cur = "";
        for (int i = 0; i < 260 && FileData.cFileName[i] != '\0'; i++) {
            cur.push_back(FileData.cFileName[i]);
        }
        if (cur.find(".dll") != std::string::npos) {
            std::cout << cur << "\n";
            dlls.push_back(cur);
        }
    } while (FindNextFile(hFind, &FileData) != 0);
    FindClose(hFind);
    std::cout << "Mods folder read\n";
    return true;
}

void Injector::orderDLLS() {
    for (size_t i = 0; i < dlls.size() && i < load_order.size(); i++) {
        if (load_order[i].compare(dlls[i]) != 0) {
            for (size_t j = 0; j < dlls.size(); j++) {
                if (dlls[j].compare(load_order[i]) == 0) {
                    std::string temp = dlls[i];
                    dlls[i] = dlls[j];
                    dlls[j] = temp;
                    break;
                }
            }
        }
    }
}

bool Injector::contains(std::string dll) {
    if (!strict_load) {
        return true;
    }
    for (auto& i : load_order) {
        if (i.compare(dll) == 0) {
            return true;
        }
    }
    return false;
}

bool Injector::freeDLL(std::string dll_name){
    for (auto& i : modules) {
        if (i.dll.compare(dll_name) == 0) {
            if (FreeLibrary(i.hmod)) {
                return true;
            }
        }
    }


    return false;
}

//config needs to be read before running this, btw
void Injector::injectImmediate() {
    for (auto& i : immediate_load) {
        if (!injectDLL(mods_folder + "\\" + i)) {
            return;
        }
    }
}