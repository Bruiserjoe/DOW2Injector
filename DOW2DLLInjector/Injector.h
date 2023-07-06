#pragma once
#include <iostream>
#include <Windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <thread>
#include <libloaderapi.h>
#include <TlHelp32.h>
#include <Psapi.h>



//todo
// -improve mesh drawing - look into view class in spooge.dll - https://learn.microsoft.com/en-us/previous-versions/windows/desktop/bb318658(v=vs.85)
//  -figure out what Device* + 0xf8 is, most likely main draw function, look in DrawOfSomeSort function
// -figure out where the menu screens are stored so we can add our own and edit the behavior
// -look into dll hijacking
// -add config for the setupdll file
// -fix the crash on 8p ffa - look at function found and also find the positions where the team is edited
// -superheavy mode with no limits on popcap or heavies limit as a toggle
// -look into suggestions
//  -Fix soundbug
//  -add lan back
//  -improve performance
//  -Fix last stand map issues
//  -Add more heros to last stand
//  -reverse server code
//  -look into re-adding dow 1 pathfinding supposed to be more efficient

//don't pass this around in functions, no copy constructor or move constructor
class Injector {
private:
    DWORD pid;
    HANDLE processh;
    std::string mods_folder;
    std::vector<std::string> dlls;
    std::vector<std::string> load_order;
    size_t sleep_time;
    DWORD get_proc_id(const char* name) {
        DWORD p_id = 0;
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap != INVALID_HANDLE_VALUE) {
            if (Process32First(snap, &pe32)) {
                do {
                    //comparing without case sensitivity
                    if (_stricmp(pe32.szExeFile, name) == 0) {
                        p_id = pe32.th32ProcessID;
                        break;
                    }
                } while (Process32Next(snap, &pe32));
            }
        }
        CloseHandle(snap);
        return p_id;
    }
    bool file_exists(std::string filename) {
        struct stat buffer;
        return (stat(filename.c_str(), &buffer) == 0);
    }
    void error(const char* err_title, const char* err_message) {
        MessageBoxA(0, err_message, err_title, 0);
        exit(-1);
    }
public:
    ~Injector() {
        CloseHandle(processh);
    }
    void start();

    //config related
    std::string readConfig();
    //process related
    bool startProcess(std::string args);
    void setProcess(std::string process);
    void setProcess(DWORD id);
    //dll related
    bool injectDLL(std::string name);
    void findDLLS(std::string folder);
    void orderDLLS();
    bool freeDLL(std::string name);
};