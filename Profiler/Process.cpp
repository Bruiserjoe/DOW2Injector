#include "Injector.h"
#include <algorithm>
#include <mutex>

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

//https://learn.microsoft.com/en-us/windows/win32/ipc/pipes
//https://learn.microsoft.com/en-us/windows/win32/winsock/initializing-winsock
//https://learn.microsoft.com/en-us/windows/win32/winsock/winsock-functions
void Injector::start(std::string cfgpath) {
    if (!readConfig(cfgpath)) {
        return;
    }
    //startup window related stuff
    if (!findDLLS(mods_folder)) {
        return;
    }

    //order the dlls, this is very important(not really) 
    orderDLLS();
    //injecting mods folder dlls
    for (auto& i : dlls) {
        bool skip = false;
        for (auto& j : immediate_load) {
            if (j.compare(i) == 0) {
                skip = true;
                break;
            }
        }
        if (!skip) {
            if (contains(i) && !injectDLL(mods_folder + "\\" + i)) {
                break;
            }
        }
    }
}