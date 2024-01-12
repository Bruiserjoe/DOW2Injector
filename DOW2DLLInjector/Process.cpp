#include "Injector.h"
#include <algorithm>
#include <mutex>
void Injector::setProcess(std::string process) {
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
}


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
        error("Failed to start", "Failed to start dow2");
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
                injectDLL("SetupDLL.dll"); //inject this dll and it will tell us when the main menu is loaded
                break;
            }
        }
    }
    std::cout << "Beginning the injection\n";
    return true;
}

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
    while (true) {
        mtx.lock();
        if (!wind.processMessages()) {
            run = false;
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
void Injector::start() {
    OpenClipboard(NULL);
    EmptyClipboard();
    CloseClipboard();


    TCHAR szExeFileName[MAX_PATH];
    GetModuleFileName(NULL, szExeFileName, MAX_PATH);
    exe_name = std::string(szExeFileName);
    std::string path = exe_name;
    exe_name = cullSlashExe(exe_name);
    createcfgpath(path);
    std::string args = readConfig();
    std::thread thr;
    if (window) {
        thr = std::thread(_windThread, this->image_path);
    }
    findDLLS(mods_folder);
    startProcess(args);

    //socket setup
    int iResult;
    WSADATA wsaData;
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cout << "Winsock failed to setup " << iResult << "\n";
        return;
    }

    SOCKET tcpsock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpsock < 0) {
        std::cout << "Failed to create socket!";
        return;
    }
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(54000);
    hint.sin_addr.S_un.S_addr = INADDR_ANY;
    iResult = bind(tcpsock, (sockaddr*)&hint, sizeof(hint));
    if (iResult == SOCKET_ERROR) {
        std::cout << "Failed to bind socket " << iResult << "\n";
        return;
    }
    iResult = listen(tcpsock, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        std::cout << "Listen failed " << iResult << "\n";
        return;
    }
    sockaddr_in client;
    int clientsize = sizeof(client);
    SOCKET client_sock = accept(tcpsock, (sockaddr*)&client, &clientsize);
    if (client_sock < 0) {
        std::cout << "failed to accept socket connection\n";
        return;
    }
    char host[NI_MAXHOST];
    char service[NI_MAXHOST];
    ZeroMemory(host, NI_MAXHOST);
    ZeroMemory(service, NI_MAXHOST);
    if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
        std::cout << host << " connect on port " << service << std::endl;
    }
    else {
        inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
        std::cout << host << " connect on port " << ntohs(client.sin_port) << std::endl;

    }
    std::string hh = "hello|";
    send(client_sock, hh.c_str(), hh.length(), 0);


    communicatecfgpath(client_sock);

    bool er = false;
    while (!er) {
        //reading the socket to see if the menu has been loaded
        std::string rev = parseRecv(client_sock);
        if (rev.compare("menu") == 0) {
            std::cout << "Main menu loaded\n";
            er = true;
        }
    }
    //order the dlls, this is very important 
    orderDLLS();
    Sleep(sleep_time); //just to be safe
    freeDLL("SetupDLL.dll");
    //injecting mods folder dlls
    for (auto& i : dlls) {
        injectDLL(mods_folder + "\\" + i);
    }
    mtx.lock();
    run = false;
    mtx.unlock();
    if (window) {
        thr.join();
    }
    //system("pause");
}