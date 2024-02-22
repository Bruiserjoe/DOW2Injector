#include "Injector.h"




bool Injector::injectDLL(std::string name) {
    if (!processh || processh == INVALID_HANDLE_VALUE) {
        setProcess(pid);
        std::cout << "Reopening process\n";
    }

    std::cout << "Injecting: " + name << "\n";
    char dll_path[MAX_PATH] = { 0 };
    GetFullPathName(name.c_str(), MAX_PATH, dll_path, NULL);
    if (!file_exists(dll_path)) {
        std::string str = "dll " + name + " is not present";
        error(str.c_str(), "File can't be found please redownload");
        return false;
    }

    LPVOID loc = VirtualAllocEx(processh, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!loc) {
        CloseHandle(processh);
        error("Failed to allocate", "Failed to allocate memory in program");
        return false;
    }

    bool err = WriteProcessMemory(processh, loc, dll_path, strlen(dll_path) + 1, 0);

    if (!err) {
        CloseHandle(processh);
        error("Failed to write", "Failed to write process memory on main injection stage");
        return false;
    }

    HANDLE hthread = CreateRemoteThread(processh, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, loc, 0, 0);

    if (!hthread) {
        CloseHandle(processh);
        error("Failed to attach thread", "Failed to attach thread to dow2");
        return false;
    }
    std::string dll_n = "";
    for (int i = name.size() - 1; i >= 0 && name[i] != '\\'; i--) {
        dll_n.push_back(name[i]);
    }
    for (size_t i = 0, j = dll_n.size() - 1; i <= j; i++, j--) {
        char tem = dll_n[i];
        dll_n[i] = dll_n[j];
        dll_n[j] = tem;
    }

    WaitForSingleObject(hthread, INFINITE);
    VirtualFreeEx(processh, loc, (strlen(dll_path) + 1), MEM_RELEASE);
    //VirtualFree(loc, 0, MEM_RELEASE);
    CloseHandle(hthread);
    return true;
}


bool Injector::findDLLS(std::string folder) {
    WIN32_FIND_DATA FileData;
    std::string files = folder + "\\*";
    HANDLE hFind = FindFirstFile(files.c_str(), &FileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        error("Folder is invalid", "Invalid folder passed");
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


bool Injector::freeDLL(std::string dll_name){
    HMODULE modules[1024];
    DWORD needed;
    HMODULE target = NULL;
    if (EnumProcessModules(processh, modules, sizeof(modules), &needed)) {
        //useful for debugging
        bool found = false;
        for (size_t i = 0; i < (needed / sizeof(HMODULE)); i++) {
            TCHAR name[128];
            DWORD size = GetModuleBaseNameA(processh, modules[i], name, 128);
            std::string use = "";
            for (size_t j = 0; j < size; j++) {
                use.push_back(name[j]);
            }
            MODULEINFO info;
            GetModuleInformation(processh, modules[i], &info, sizeof(info));
            if (use.compare(dll_name) == 0) {
                //std::cout << name << " base address: " << info.lpBaseOfDll << "\n";
                target = (HMODULE)info.lpBaseOfDll;
                found = true;
                break;
            }
            //std::cout << "Module: " +  use << "\n";
        }
        std::cout << "Found " << dll_name << " unloading from target process\n";
        //std::sort(total.begin(), total.end());
        //(total.size() == *prev) ? *count += 1: *count = 0;
        if (found && target != NULL) {
            HANDLE hthread = CreateRemoteThread(processh, 0, 0, (LPTHREAD_START_ROUTINE)FreeLibrary, target, 0, 0);

            if (!hthread) {
                CloseHandle(processh);
                error("Failed to attach thread", "Failed to attach thread to dow2");
                return false;
            }


            WaitForSingleObject(hthread, INFINITE);
            CloseHandle(hthread);
            std::cout << "Library successfully unloaded\n";
            return true;
        }
    }
    


    return false;
}
