// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"
#pragma comment (lib, "ws2_32.lib")
#include <winsock2.h>
#include <WS2tcpip.h>
#include <windows.h>


std::string parseRecv(SOCKET sock) {
    char buf[512];
    recv(sock, buf, 512, 0);
    std::string ret = "";
    for (int i = 0; i < 512 && buf[i] != '|'; i++) {
        ret.push_back(buf[i]);
    }
    return ret;
}

bool checkClipboard(std::string comp) {
    bool ret = false;
    if (OpenClipboard(NULL)) {
        char* buffer = (char*)GetClipboardData(CF_TEXT);
        if (buffer) {
            std::string str(buffer);
            if (str.compare(comp) == 0) {
                ret =  true;
            }
        }
    }
    CloseClipboard();
    return ret;
}



typedef void(__thiscall *MainMenu)(void* ecx, int* param1);
MainMenu org_menu = reinterpret_cast<MainMenu>(0x0047254f);


HMODULE hmod;
std::atomic_bool first = ATOMIC_VAR_INIT(true);
std::ofstream file;
//param1 is null
void __fastcall menudetour(void* ecx, int* param1) {
    file << "StartupDLL: Menu before call\n";
    int* in = (int*)(ecx); //no idea why this works
    org_menu(ecx, in);
    file << "StartupDLL: Menu called\n";
    first = false;
   
}

DWORD menumidhookjmp = 0;
void __declspec(naked) MenuMidHook() {
    __asm {
        push ebp;
        mov ebp, esp;
        and esp, 0x0FFFFFFF8;
        mov first, 0x0;
        jmp[menumidhookjmp];
    }
}

std::string flipstring(std::string str) {
    std::string ret = "";
    for (int i = str.size() - 1; i >= 0; i--) {
        ret.push_back(str[i]);
    }
    return ret;
}

//path and file name up to 256 bytes each
std::string cfgfile;
char* cfgp;
//path actually 1024 bytes max
std::string path;
char* pathp;
//reads the clipboard and sets the cfg path variables
bool setcfg(SOCKET sock) {
    std::string t = parseRecv(sock);
    if (t.compare("hello") != 0) {
        std::string cfg;
        int i = t.size() - 1;
        for (; i >= 0 && t[i] != '/'; i--) {
            cfg.push_back(t[i]);
        }
        cfg = flipstring(cfg);
        cfgfile = cfg;
        cfgp = (char*)cfgfile.c_str();
        std::string p;
        for (; i >= 0; i--) {
            p.push_back(t[i]);
        }
        p = flipstring(p);
        path = p;
        pathp = (char*)path.c_str();
        send(sock, "cfg|", 4, 0);
        return true;
    }
    return false;
}


DWORD base;
DWORD jmpback_midcfgload;
void __declspec(naked) MidCfgLoad() {
    __asm {
        add esp, 0x0C;
        push cfgp;
        mov eax, esi;
        push pathp;
        lea eax, [ebp - 0x118];
        push eax;
        jmp[jmpback_midcfgload];
    }
}

//https://www.youtube.com/watch?v=WDn-htpBlnU
DWORD WINAPI MainThread(LPVOID param) {
    bool er = false;
    // Initialize Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        return -1;
    }

    SOCKET tcpsock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpsock < 0) {
        return -1;
    }
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(54000);
    inet_pton(AF_INET, "127.0.0.1", &hint.sin_addr);
    iResult = connect(tcpsock, (sockaddr*)&hint, sizeof(hint));
    if (iResult == SOCKET_ERROR) {
        closesocket(tcpsock);
        WSACleanup();
        return -1;
    }
    
    
    //getting the clipboard
    while (!setcfg(tcpsock));

    while (!er) {
        if (!first) {
            send(tcpsock, "menu|", 5, 0);
            er = true;
            file << "StartupDLL: Startup packet sent\n";
        }
    }
    BYTE* src = (BYTE*)"\x55\x8B\xEC\x83\xE4\xF8";
    MemPatch(reinterpret_cast<BYTE*>(base + 0x7254F), src, 6);
    src = (BYTE*)"\x83\xC4\x0C\x68\x08\x57\x08\x01";
    MemPatch(reinterpret_cast<BYTE*>(base + 0x1D36B), src, 8);
    file.close();
    closesocket(tcpsock);
    WSACleanup();
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        file.open("mod_logs\\startup.log");
        base = (DWORD)GetModuleHandleA("DOW2.exe");
        menumidhookjmp = base + 0x72555;
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x7254F), (DWORD)MenuMidHook, 6);
        hmod = hModule;

        CreateThread(0, 0, MainThread, hModule, 0, 0);
        
        jmpback_midcfgload = (base + 0x1D37D);
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x1D36B), (DWORD)MidCfgLoad, 8);
    case DLL_PROCESS_DETACH:
        
        break;
    }
    return TRUE;
}

