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

//https://www.youtube.com/watch?v=KTS9Ujhqm6I
//https://www.youtube.com/watch?v=mKUSLJjlajg

//Relic DLLs left to be reversed
//  -Localizer.dll
//  -LuaConfig.dll
//  -MathBox.dll
//  -Memory.dll
//  -Platoform.dll
//  -Shark.dll
//  -SimEngine.dll
//  -Spooge.dll
//  -STLPort.dll
//  -Util.dll
//  -XThread.dll
//  -Bugsplat.dll
//  -BugsplatRC.dll
//  -Debug.dll
//  -FileParser.dll
//  -FileSystem.dll

//curent todo 1.9
    // -fatalf from config file, IE check if the proper launch options are in use X
    // -borderless fullscreen patch 
    // -extend module file type so you don't need the seperate config files scattered all over the dow2 folder
    // -multiple module loading, the top level loaded will overwrite anything loaded after it
    // -new, more responsive tools for editing mod data
    // -scripts for converting file types to dow 2 file types
    // -merge rust patch (https://github.com/RipleyTom/rustpatch)
    // -fix crashes on gamemode patch
    // -    think maybe is from mscvp versions being different?? I think I'm doing everything right otherwise - compiling with vs2005 does not fix crash
    // -    copy map list loading more closely! - this is the path probably??
    // -change how cullsphere patch works, rip out algorithm solver, just add options for max curve etc X
    // -add networking patch (connect to custom relicnet/lobby server)
    // -stable release version of reversed relicnet server
    // -launcher (add game finder when server reversed)
    // -move all mods over to using timestampedf for error logging X
    // -immediate dll injection option X

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

//current todo 2.0
    // -ui patch (add elements to any ui page, also create new ui pages)
    // -add lan back
    // -performance patch 
        // -fix ui rendering being seperate draw calls
          //           -look into view class in spooge.dll - https://learn.microsoft.com/en-us/previous-versions/windows/desktop/bb318658(v=vs.85)
         //              -figure out what Device* + 0xf8 is, most likely main draw function, look in DrawOfSomeSort function



//don't pass this around in functions, no copy constructor or move constructor
class Injector {
private:
    DWORD pid;
    std::string launch_options;
    std::string exe_name;
    std::string mods_folder;
    std::vector<std::string> dlls;
    std::vector<std::string> load_order;
    std::vector<std::string> immediate_load;
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
public:
    //config related
    bool readConfig(std::string path);
    std::string getModuleCmdLine();
    bool testCmdLine();
    //dll related
    void injectImmediate();

    void start(std::string cfgpath);

    //playercfg related
    std::string createcfg(std::string module);

    std::string getExe() {
        TCHAR szExeFileName[MAX_PATH];
        GetModuleFileName(NULL, szExeFileName, MAX_PATH);
        std::string ret = std::string(szExeFileName);
        return ret;
    }
    std::string getLaunchOptions() {
        return launch_options;
    }
};