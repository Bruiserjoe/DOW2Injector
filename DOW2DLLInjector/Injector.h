#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCKAPI_ 
//https://stackoverflow.com/questions/21399650/cannot-include-both-files-winsock2-windows-h
#pragma comment(lib, "ws2_32.lib")
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
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>


//current todo 1.8.1
    // -add support for only loading dlls set in load order
    // -make cullsphere patch adjustable by user
    // -shell patch
    // -fix requistion point upgrades fucking up healthbar


//backlog
// -improve mesh drawing - look into view class in spooge.dll - https://learn.microsoft.com/en-us/previous-versions/windows/desktop/bb318658(v=vs.85)
//      -figure out what Device* + 0xf8 is, most likely main draw function, look in DrawOfSomeSort function
// -game finder for all mods, so easier to find match, similar to FAF(need money for this)
// -figure out how to make lobby size "infinite"
// -custom team layouts
// -superheavy mode with no limits on popcap or heavies limit as a toggle
// -make ui mod which allows adding new elements to every ui page (basically expanded shell patch)
// -look into suggestions
    // -Fix soundbug, probably in shark.dll or fmod related; (SOUND: streaming buffer overflow: )
    // -add lan back
    // -improve performance, improve model drawing(main issue with performance)
    // -Fix last stand map issues
    // -Add more heros to last stand
    // -reverse server code

class Window {
private:
    HWND wind;
    HINSTANCE instance;
    HDC dc;
    LPCSTR class_name;
    size_t width;
    size_t height;
    ULONG_PTR plus_token;
public:
    Window(const Window&) = delete;
    Window& operator =(const Window&) = delete;
    Window(LPCSTR name, size_t w, size_t h, std::string img_path);
    ~Window();


    int getWidth() {
        return width;
    }
    int getHeight() {
        return height;
    }
    HWND getHwnd() {
        return wind;
    }
    bool processMessages();

};



//don't pass this around in functions, no copy constructor or move constructor
class Injector {
private:
    DWORD pid;
    HANDLE processh;
    SOCKET tcpsock;
    std::string exe_name;
    std::string setupdll_name;
    std::string mods_folder;
    std::vector<std::string> dlls;
    std::vector<std::string> load_order;
    size_t sleep_time;
    std::string image_path;
    std::string cfg_path;
    bool window = true;
    bool local_folder = false;
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
        //exit(-1);
    }
public:
    ~Injector() {
        CloseHandle(processh);
    }
    void start(std::string cfgpath);

    //config related
    std::string readConfig(std::string path);
    //playercfg related
    void createcfgpath(std::string path);
    void communicatecfgpath(SOCKET sock);
    //process related
    bool startProcess(std::string args);
    void setProcess(std::string process);
    void setProcess(DWORD id);
    //dll related
    bool injectDLL(std::string name);
    bool findDLLS(std::string folder);
    void orderDLLS();
    bool freeDLL(std::string name);
};