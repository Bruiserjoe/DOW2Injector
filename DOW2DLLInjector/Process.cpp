#include "Injector.h"
#include <algorithm>
#include <mutex>

void Injector::setProcess(std::string process) {
    //injecting the dlls
    pid = get_proc_id(process.c_str());
    processh = OpenProcess(PROCESS_ALL_ACCESS, 0, pid);
    if (!processh) {
        error("Failed to open process", "Failed to get handle from process");
        return;
    }
}
void Injector::setProcess(DWORD id) {
    //injecting the dlls
    pid = id;
    processh = OpenProcess(PROCESS_ALL_ACCESS, 0, pid);
    if (!processh) {
        error("Failed to open process", "Failed to get handle from process");
        return;
    }
}



//looping through modules
bool CheckModules(HANDLE process) {
    HMODULE modules[1024];
    DWORD needed;
    std::vector<std::string> total;
    if (EnumProcessModules(process, modules, sizeof(modules), &needed)) {
        //useful for debugging
        for (size_t i = 0; i < (needed / sizeof(HMODULE)); i++) {
            TCHAR name[128];
            DWORD size = GetModuleBaseNameA(process, modules[i], name, 128);
            std::string use = "";
            for (size_t j = 0; j < size; j++) {
                use.push_back(name[j]);
            }
            total.push_back(use);
            
            //std::cout << "Module: " +  use << "\n";
        }
        //std::sort(total.begin(), total.end());
        //(total.size() == *prev) ? *count += 1: *count = 0;
        if (total.size() >= 149) {
            return true;
        }

    }


    return false;
}



//this is causing crashes because the injection doesn't happen after load, can cause weird errors
bool Injector::startProcess(std::string args) {
    std::cout << "Starting " + args << "\n";
    //starting dow2
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    //rebuild it
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    LPSTR args2 = (char*)args.c_str();
    if (!CreateProcessA(NULL, args2, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        error("Failed to start", "Failed to start dow2");
        return false;
    }
    std::cout << "Waiting for Dawn of War to load\n";
    //gotta wait till dow2 is shown on screen
    bool sk = false;
    while (true) {
        HWND wind = FindWindowA(NULL, "Dawn of War II");

        if (IsWindowVisible(wind)) {
            if (!sk) {
                setProcess("DOW2.exe");
                SetPriorityClass(processh, HIGH_PRIORITY_CLASS);
            }
            if (CheckModules(processh)) {
                injectDLL("SetupDLL.dll"); //inject this dll and it will tell us when the main menu is loaded
                break;
            }
        }
    }
    std::cout << "Beginning the injection\n";
    return true;
}

size_t countOccur(std::string str, char c) {
    size_t count = 0;
    for (auto& i : str) {
        if (i == c) {
            count++;
        }
    }
    return count;
}

std::string cullSlashExe(std::string str) {
    std::string ret = "";
    size_t c = countOccur(str, '\\');
    size_t p = 0;
    size_t co = 0;
    for (p; p < str.size() && co < c; p++) {
        if (str[p] == '\\') {
            co++;
        }
    }
    //now we are at actual filename
    for (p; p < str.size() && str[p] != '.'; p++) {
        ret.push_back(str[p]);
    }
    return ret;
}
std::mutex mtx;
bool run = true;
void _windThread() {
    Window wind("DOW2 Injector", 600, 300);
    while (true) {
        mtx.lock();
        if (!wind.processMessages()) {
            run = false;
        }
        if (!run) {
            mtx.unlock();
            break;
        }
        mtx.unlock();
        Sleep(10);
    }
}


void Injector::start() {
    OpenClipboard(NULL);
    EmptyClipboard();
    CloseClipboard();

    TCHAR szExeFileName[MAX_PATH];
    GetModuleFileName(NULL, szExeFileName, MAX_PATH);
    exe_name = std::string(szExeFileName);
    exe_name = cullSlashExe(exe_name);

    std::string args = readConfig();
    std::thread thr;
    if (window) {
        thr = std::thread(_windThread);
    }
    findDLLS(mods_folder);
    startProcess(args);
   
    bool er = false;
    while (!er) {
        //reading clipboard to see if the menu has been loaded
        if (OpenClipboard(NULL)) {
            char* buffer = (char*)GetClipboardData(CF_TEXT);
            if (buffer) {
                std::string str(buffer);
                if (str.compare("Menu setup") == 0) {
                    er = true;
                    std::cout << "Main menu loaded\n";
                }
            }
        }
        CloseClipboard();
    }
    OpenClipboard(NULL);
    EmptyClipboard();

    const char* msg = "Injection Start";
    HGLOBAL glob = GlobalAlloc(GMEM_FIXED, sizeof(char) * 16);
    char* buffer = (char*)GlobalLock(glob);
    strcpy_s(buffer, sizeof(char) * 16, msg);
    GlobalUnlock(glob);

    SetClipboardData(CF_TEXT, glob);
    CloseClipboard();
    //order the dlls, this is very important 
    orderDLLS();
    Sleep(sleep_time); //just to be safe
    freeDLL("SetupDLL.dll");
    //injecting mods folder dlls
    for (auto& i : dlls) {
        injectDLL(mods_folder + "\\" + i);
    }
    mtx.lock();
    run = false;
    mtx.unlock();
    if (window) {
        thr.join();
    }
    //system("pause");
}