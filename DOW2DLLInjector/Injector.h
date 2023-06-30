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


//structure
// main
// -Loop through mod dll directory
//     -And create DLL object 
// -loop through the dlls and inject them into dow2.exe
// -exit


//todo
// -add load order
// -add setting dow2.exe path so don't have to store in main dow2 folder

//don't pass this around in functions, no copy constructor or move constructor
class Injector {
private:
    DWORD pid;
    HANDLE processh;
    std::string mods_folder;
    std::vector<std::string> dlls;
    DWORD get_proc_id(const char* name) {
        DWORD p_id = 0;
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap != INVALID_HANDLE_VALUE) {
            if (Process32First(snap, &pe32)) {
                do {
                    if (strcmp(pe32.szExeFile, name) == 0) {
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
    //dll related
    bool injectDLL(std::string name);
    void findDLLS(std::string folder);
};