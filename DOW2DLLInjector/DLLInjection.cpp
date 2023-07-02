#include "Injector.h"




bool Injector::injectDLL(std::string name) {
    std::cout << "Injecting: " + name << "\n";
    //char dll_name[] = "BaseDLL.dll";
    char dll_path[MAX_PATH] = { 0 };
    GetFullPathName(name.c_str(), MAX_PATH, dll_path, NULL);
    if (!file_exists(dll_path)) {
        std::string str = "dll " + name + " is not present";
        error(str.c_str(), "File can't be found please redownload");
        return false;
    }

    LPVOID loc = VirtualAllocEx(processh, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!loc) {
        CloseHandle(processh);
        error("Failed to allocate", "Failed to allocate memory in program");
        return false;
    }

    bool err = WriteProcessMemory(processh, loc, dll_path, strlen(dll_path), nullptr);

    if (!err) {
        CloseHandle(processh);
        error("Failed to write", "Failed to write process memory on main injection stage");
        return false;
    }

    HANDLE hthread = CreateRemoteThread(processh, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, loc, 0, nullptr);

    if (!hthread) {
        CloseHandle(processh);
        error("Failed to attach thread", "Failed to attach thread to dow2");
        return false;
    }


    WaitForSingleObject(hthread, INFINITE);
    VirtualFree(loc, strlen(dll_path), MEM_RELEASE);
    CloseHandle(hthread);
    return true;
}


void Injector::findDLLS(std::string folder) {
    WIN32_FIND_DATA FileData;
    std::string files = folder + "\\*";
    HANDLE hFind = FindFirstFile(files.c_str(), &FileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        error("Folder is invalid", "Invalid folder passed");
        return;
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