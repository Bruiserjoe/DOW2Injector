#include "Injector.h"
#include <psapi.h>
#include <algorithm>

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
std::string readAfterColonWithSpaces(std::string data, size_t start) {
    size_t pos = start;
    for (pos; pos < data.size() && data[pos] != ':'; pos++);
    pos++;
    //now skip spaces before the actual data
    for (pos; pos < data.size() && (data[pos] == ' ' || data[pos] == '\t'); pos++);
    std::string ret1 = "";
    for (pos; pos < data.size() && data[pos] != '\n'; pos++) {
        ret1.push_back(data[pos]);
    }
    return ret1;
}
std::string readLine(std::string data, size_t* start) {
    std::string ret = "";
    for (*start; *start < data.size() && data[*start] != '\n'; (*start)++) {
        ret.push_back(data[*start]);
    }
    (*start)++;
    return ret;
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
        size_t pos;
        std::string con = "";
        pos = str.find("exe-path:");
        con = readAfterColonWithSpaces(str, pos);
        if (con.compare("local") != 0) {
            ret = con;
        }
        else {
            ret = "DOW2.exe";
        }
        pos = str.find("module:");
        con = readAfterColon(str, pos);
        if (con.compare("none") != 0) {
            ret = ret + " -modname " + con;
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
        pos = str.find("skip-movies:");
        con = readAfterColon(str, pos);
        if (con.compare("true") == 0) {
            ret = ret + " -nomovies";
        }
        pos = str.find("windowed:");
        con = readAfterColon(str, pos);
        if (con.compare("true") == 0) {
            ret = ret + "-window";
        }
        pos = str.find("sleep-after-menu:");
        con = readAfterColon(str, pos);
        sleep_time = std::stoi(con);
        //reading load order
        pos = str.find("load-order:");
        std::string line = readLine(str, &pos);
        while ((line = readLine(str, &pos)).compare("end-load")) {
            load_order.push_back(line);
        }
    }
    else {
        std::ofstream file;
        file.open("loader.config");
        file << "module: none\n";
        file << "mod-folder: mods\n";
        file << "dev: false\n";
        file << "skip-movies: true\n";
        file << "windowed: false\n";
        file << "sleep-after-menu: 500\n";
        file << "exe-path: local\n";
        file << "load-order:\n"; 
        file << "end-load\n";
        file.close();
        mods_folder = "mods";
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

//looping through modules
bool CheckModules(HANDLE process) {
    HMODULE modules[1024];
    DWORD needed;
    std::vector<std::string> total;
    if (EnumProcessModules(process, modules, sizeof(modules), &needed)) {
        //useful for debugging
        for (int i = 0; i < (needed / sizeof(HMODULE)); i++) {
            TCHAR name[128];
            DWORD size = GetModuleBaseNameA(process, modules[i], name, 128);
            std::string use = "";
            for (int j = 0; j < size; j++) {
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

void Injector::orderDLLS() {
    for (int i = 0; i < dlls.size() && i < load_order.size(); i++) {
        if (load_order[i].compare( dlls[i]) != 0) {
            for (int j = 0; j < dlls.size(); j++) {
                if (dlls[j].compare(load_order[i]) == 0) {
                    std::string temp = dlls[i];
                    dlls[i] = dlls[j];
                    dlls[j] = temp;
                    break;
                }
            }
        }
    }
}


void Injector::start() {
    OpenClipboard(NULL);
    EmptyClipboard();
    CloseClipboard();

    std::string args = readConfig();
    findDLLS(mods_folder);
    startProcess(args);
    bool er = false;
    while (!er) {
        //reading clipboard to see if the menu has been loaded
        if (OpenClipboard(NULL)) {
            char* buffer = (char*)GetClipboardData(CF_TEXT);
            if (buffer) {
                std::string str(buffer);
                if (str.compare("Menu setup") == 0) {
                    er = true;
                    std::cout << "Main menu loaded\n";
                }
            }
        }
        CloseClipboard();
    }
    //order the dlls, this is very important 
    orderDLLS();
    Sleep(sleep_time); //just to be safe
    //injecting mods folder dlls
    for (auto& i : dlls) {
        injectDLL(mods_folder + "\\" + i);
    }
    system("pause");
}