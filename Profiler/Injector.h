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

typedef void(__cdecl *Timestampedf)(const char*, ...);
typedef void(__cdecl *Fatalf)(const char*, ...);

extern Timestampedf Timestampedtracef;
extern Fatalf Fatal_f;


//current todo 1.8.3
    // -fatalf from config file, IE check if the proper launch options are in use
    // -fix requistion point upgrades fucking up healthbar
    // -move all mods over to using timestampedf for error logging
    //gamemode patch
        // -custom team layouts
        // -figure out how to make lobby size "infinite"
        // -lobby that has tickboxes which are dynamically filled and correspond to .scar files located in a folder you designate
        // if it's ticked - that scar file is loaded on mission start
        // -neutral ai loaded on game start in lobby slot
        // -make gamemodes have limited races
        // -superheavy mode with no limits on popcap or heavies limit as a toggle

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
    std::string exe_name;
    std::string mods_folder;
    std::vector<std::string> dlls;
    std::vector<std::string> load_order;
    struct Module {
        std::string dll;
        HMODULE hmod;
    };
    std::vector<Module> modules;

    std::string cfg_path;
    bool strict_load = false;
 
    bool file_exists(std::string filename) {
        struct stat buffer;
        return (stat(filename.c_str(), &buffer) == 0);
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
    bool readConfig(std::string path);
public:
    void start(std::string cfgpath);

    //playercfg related
    std::string createcfg(std::string module);
};