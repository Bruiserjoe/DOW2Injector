#pragma once
//https://stackoverflow.com/questions/21399650/cannot-include-both-files-winsock2-windows-h
#pragma comment (lib,"Gdiplus.lib")
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
#include <gdiplus.h>
#include <stdio.h>
#include <stdlib.h>


//add dbinternal::timestampedtracef for error throwing
//issue with side-by-side config could be from differing c++ compiled run times

//current todo 1.8.2
    // -replace profiler.dll
//current todo 1.8.3
    //gamemode patch
        // -custom team layouts
        // -figure out how to make lobby size "infinite"
        // -lobby that has tickboxes which are dynamically filled and correspond to .scar files located in a folder you designate
        // if it's ticked - that scar file is loaded on mission start
        // -neutral ai loaded on game start in lobby slot
        // -make gamemodes have limited races
        // -superheavy mode with no limits on popcap or heavies limit as a toggle
        // -fix requistion point upgrades fucking up healthbar

//backlog
// -improve mesh drawing - look into view class in spooge.dll - https://learn.microsoft.com/en-us/previous-versions/windows/desktop/bb318658(v=vs.85)
//      -figure out what Device* + 0xf8 is, most likely main draw function, look in DrawOfSomeSort function
// -game finder for all mods, so easier to find match, similar to FAF(need money for this)
// -make ui mod which allows adding new elements to every ui page (basically expanded shell patch)
// -look into suggestions
    // -Fix soundbug, probably in shark.dll or fmod related; (SOUND: streaming buffer overflow: )
    // -add lan back
    // -improve performance, improve model drawing(main issue with performance)
    //          -fix ui rendering being seperate draw calls
    // -Fix last stand map issues
    // -Add more heros to last stand
    // -reverse server code


//don't pass this around in functions, no copy constructor or move constructor
class Injector {
private:
    DWORD pid;
    // HANDLE processh;
    // SOCKET tcpsock;
    std::string exe_name;
    // std::string setupdll_name;
    std::string mods_folder;
    std::vector<std::string> dlls;
    std::vector<std::string> load_order;
    struct Module {
        std::string dll;
        HMODULE hmod;
    };
    std::vector<Module> modules;

    size_t sleep_time;
    std::string image_path;
    std::string cfg_path;
    // bool window = true;
    bool local_folder = false;
    bool strict_load = false;
 
    bool file_exists(std::string filename) {
        struct stat buffer;
        return (stat(filename.c_str(), &buffer) == 0);
    }
    void error(const char* err_message) {
        // MessageBoxA(0, err_message, err_title, 0);
        exit(-1);
    }

    //playercfg related
    void createcfgpath(std::string path, std::string module);
    //dll related
    bool injectDLL(std::string name);
    bool findDLLS(std::string folder);
    void orderDLLS();
    bool contains(std::string dll);
    bool freeDLL(std::string name);
    //config related
    std::string readConfig(std::string path);
public:
    ~Injector() {
        // CloseHandle(processh);
    }
    void start(std::string cfgpath);

    //playercfg related
    std::string createcfg(std::string module);
};