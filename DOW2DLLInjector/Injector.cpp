#include "Injector.h"


void error(const char* err_title, const char* err_message) {
    MessageBoxA(0, err_message, err_title, 0);
    exit(-1);
}

std::string readAfterColon(std::string data, size_t start) {
    size_t pos = start;
    for (pos; pos < data.size() && data[pos] != ':'; pos++);
    pos++;
    std::string ret1 = "";
    for (pos; pos < data.size() && data[pos] != '\n'; pos++) {
        if (data[pos] != ' ') {
            ret1.push_back(data[pos]);
        }
    }
    return ret1;
}


std::string Injector::readConfig() {
    std::cout << "Reading config \n";
    std::string ret = "";
    if (file_exists("loader.config")) {
        std::ifstream file;
        file.open("loader.config");
        std::stringstream stream;
        stream << file.rdbuf();
        std::string str = stream.str();
        size_t pos = str.find("module:");
        std::string con = "";
        con = readAfterColon(str, pos);
        if (con.compare("none") == 0) {
            ret = "DOW2.exe";
        }
        else {
            ret = "DOW2.exe -modname " + con;
        }
        pos = str.find("mod-folder:");
        con.clear();
        con = readAfterColon(str, pos);
        mods_folder = con;
        pos = str.find("dev:");
        con = readAfterColon(str, pos);
        if (con.compare("true") == 0) {
            ret = ret + " -dev";
        }
    }
    else {
        std::ofstream file;
        file.open("loader.config");
        file << "module: none\n";
        file << "mod-folder: mods\n";
        file << "dev: false\n";
        file.close();
        ret = "DOW2.exe";
    }
    return ret;
}


void Injector::setProcess(std::string process) {
    //injecting the dlls
    pid = get_proc_id(process.c_str());
    processh = OpenProcess(PROCESS_ALL_ACCESS, 0, pid);
    if (!processh) {
        error("Failed to open process", "Failed to get handle from process");
        return;
    }
}


bool Injector::injectDLL(std::string name) {
    std::cout << "Injecting: " + name << "\n";
    //char dll_name[] = "BaseDLL.dll";
    char dll_path[MAX_PATH] = { 0 };
    GetFullPathName(name.c_str(), MAX_PATH, dll_path, NULL);
    if (!file_exists(dll_path)) {
        std::string str = "dll " + name + " is not present";
        error(str.c_str(), "File can't be found please redownload");
        return false;
    }

    LPVOID loc = VirtualAllocEx(processh, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!loc) {
        CloseHandle(processh);
        error("Failed to allocate", "Failed to allocate memory in program");
        return false;
    }

    bool err = WriteProcessMemory(processh, loc, dll_path, strlen(dll_path), nullptr);

    if (!err) {
        CloseHandle(processh);
        error("Failed to write", "Failed to write process memory on main injection stage");
        return false;
    }

    HANDLE hthread = CreateRemoteThread(processh, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, loc, 0, nullptr);

    if (!hthread) {
        CloseHandle(processh);
        error("Failed to attach thread", "Failed to attach thread to dow2");
        return false;
    }

    //error("Inject success", "Sucessfully injected");

    //Sleep(20000);
    WaitForSingleObject(hthread, INFINITE);
    VirtualFree(loc, strlen(dll_path), MEM_RELEASE);
    CloseHandle(hthread);
    return true;
}



bool Injector::startProcess(std::string args) {
    std::cout << "Starting " + args << "\n";
    //starting dow2
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    const char* path = "D:\\SteamLibrary\\steamapps\\common\\Dawn of War II - Retribution\\DOW2.exe -modname Anni2 -dev";
    LPSTR args2 = (char*)path;
    if (!CreateProcessA(NULL, args2, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        return false;
    }
    std::cout << "Waiting for Dawn of War to load\n";
    //gotta wait till dow2 is shown on screen
    while (true) {
        HWND wind = FindWindowA(NULL, "Dawn of War II");
        if (IsWindowVisible(wind)) {
            break;
        }
    }
    std::cout << "Begining the injection\n";
    return true;
}



void Injector::findDLLS(std::string folder) {
    WIN32_FIND_DATA FileData;
    std::string files = folder + "\\*";
    HANDLE hFind = FindFirstFile(files.c_str(), &FileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        error("Folder is invalid", "Invalid folder passed");
        return;
    }
    std::cout << "Reading mods folder\n";
    do
    {
        std::string cur = "";
        for (int i = 0; i < 260 && FileData.cFileName[i] != '\0'; i++) {
            cur.push_back(FileData.cFileName[i]);
        }
        if (cur.find(".dll") != std::string::npos) {
            std::cout << cur << "\n";
            dlls.push_back(cur);
        }
    } while (FindNextFile(hFind, &FileData) != 0);
    FindClose(hFind);
    std::cout << "Mods folder read\n";
}




void Injector::start() {
    std::string args = readConfig();
    findDLLS(mods_folder);
    startProcess(args);
    setProcess("DOW2.exe");
    for (auto& i : dlls) {
        injectDLL(mods_folder + "\\" + i);
    }
}