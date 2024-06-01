#include "Injector.h"
#include <algorithm>
#include <mutex>

/*void Injector::setProcess(std::string process) {
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
std::string parseRecv(SOCKET sock) {
    char buf[512];
    recv(sock, buf, 512, 0);
    std::string ret = "";
    for (int i = 0; i < 512 && buf[i] != '|'; i++) {
        ret.push_back(buf[i]);
    }
    return ret;
}*/


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
/*
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
        std::string error_msg = "Failed to start DOW2, ";
        if (local_folder) {
            error_msg = error_msg + "check if the injector is in the right folder!";
        }
        else {
            error_msg = "your DOW2.exe path isn't correct!";
        }
        error("Failed to start", error_msg.c_str());
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
                injectDLL(setupdll_name); //inject this dll and it will tell us when the main menu is loaded
                break;
            }
        }
    }
    std::cout << "Beginning the injection\n";
    return true;
} */

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
void _windThread(std::string img_path) {
    Window wind("DOW2 Injector", 600, 300, img_path);
    auto start = std::chrono::high_resolution_clock::now();
    while (true) {
        mtx.lock();
        auto t = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> dur = t - start;
        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur);
        if (!wind.processMessages()) {
            run = false;
        }
        else if (ms.count() >= 60000) {
            run = false;
            MessageBoxA(0, "Timeout Error", "Loading took too long, exiting injector", 0);
            exit(-1);
        }
        if (!run) {
            mtx.unlock();
            break;
        }
        mtx.unlock();
        Sleep(10);
    }
}

//https://learn.microsoft.com/en-us/windows/win32/ipc/pipes
//https://learn.microsoft.com/en-us/windows/win32/winsock/initializing-winsock
//https://learn.microsoft.com/en-us/windows/win32/winsock/winsock-functions
void Injector::start(std::string cfgpath) {
    std::string args = readConfig(cfgpath);
    //startup window related stuff
    std::thread thr;
    if (window) {
        thr = std::thread(_windThread, this->image_path);
    }
    if (!findDLLS(mods_folder)) {
        mtx.lock();
        run = false;
        mtx.unlock();
        if (window) {
            thr.join();
        }
        return;
    }

    //don't need to use socket anymore
    //just need to have seperate function to setup the player config



    //order the dlls, this is very important(not really) 
    orderDLLS();
    Sleep(sleep_time); //just to be safe, some stuff still loading on first menu call
    //injecting mods folder dlls
    for (auto& i : dlls) {
        if (contains(i) && !injectDLL(mods_folder + "\\" + i)) {
            break;
        }
    }
    mtx.lock();
    run = false;
    mtx.unlock();
    if (window) {
        thr.join();
    }
    //system("pause");
}