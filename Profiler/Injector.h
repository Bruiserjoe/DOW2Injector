#pragma once
//https://stackoverflow.com/questions/21399650/cannot-include-both-files-winsock2-windows-h
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
#include <stdio.h>
#include <stdlib.h>
#include <detours.h>

typedef void(__cdecl *Timestampedf)(const char*, ...);
typedef void(__cdecl *Fatalf)(const char*, ...);

extern Timestampedf Timestampedtracef;
extern Fatalf Fatal_f;


//curent todo 1.9
    // -merge rust patch (https://github.com/RipleyTom/rustpatch)
    // -change how cullsphere patch works, rip out algorithm solver, just add options for max curve etc X
    // -add networking patch (connect to custom relicnet/lobby server)
    // -stable release version of reversed relicnet server
    // -launcher (add game finder when server reversed)
    // -fatalf from config file, IE check if the proper launch options are in use
    // -move all mods over to using timestampedf for error logging X
    // -header to easily load common functions for patches

//current todo 1.9.1
    // -fix requistion point upgrades fucking up healthbar
    //gamemode patch
        // -custom team layouts
        // -figure out how to make lobby size "infinite"
        // -lobby that has tickboxes which are dynamically filled and correspond to .scar files located in a folder you designate
        //       -superheavy mode with no limits on popcap or heavies limit as a toggle
        //       -if it's ticked - that scar file is loaded on mission start
        // -neutral ai loaded on game start in lobby slot
        // -make gamemodes have limited races
        // -fix crash on private lobby or joining gamemode with custom map pools
        

//backlog
// -improve mesh drawing - look into view class in spooge.dll - https://learn.microsoft.com/en-us/previous-versions/windows/desktop/bb318658(v=vs.85)
//      -figure out what Device* + 0xf8 is, most likely main draw function, look in DrawOfSomeSort function
// -make ui mod which allows adding new elements to every ui page (basically expanded shell patch)
// -look into suggestions
    // -add lan back
    // -improve performance, improve model drawing(main issue with performance)
    //          -fix ui rendering being seperate draw calls
    // -Fix last stand map issues
    // -Add more heros to last stand


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

    std::string getExe() {
        TCHAR szExeFileName[MAX_PATH];
        GetModuleFileName(NULL, szExeFileName, MAX_PATH);
        std::string ret = std::string(szExeFileName);
        return ret;
    }
};