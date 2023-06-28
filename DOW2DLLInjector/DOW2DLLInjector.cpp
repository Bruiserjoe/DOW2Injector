#include "Injector.h"







//https://reverseengineering.stackexchange.com/questions/8120/making-application-load-dll-at-start
//https://www.unknowncheats.me/forum/programming-for-beginners/83148-automatically-inject-dll.html






int main()
{
    /*
    //starting dow2
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    const char* path = "D:\\SteamLibrary\\steamapps\\common\\Dawn of War II - Retribution\\DOW2.exe -modname Anni2 -dev";
    LPSTR args = (char*)path;
    CreateProcessA(NULL, args, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

    //gotta wait till dow2 is shown on screen
    while (true) {
        HWND wind = FindWindowA(NULL, "Dawn of War II");
        if (IsWindowVisible(wind)) {
            break;
        }
    }*/


    //CloseHandle(pi.hProcess);
    //CloseHandle(pi.hThread);
    //WINDOWPLACEMENT wp;


    Injector inject;
    inject.start();
   /* std::string args = inject.readConfig();
    
    inject.startProcess(args);
    inject.setProcess("DOW2.exe");
    inject.injectDLL("BaseDLL.dll");*/

   /* char dll_name[] = "BaseDLL.dll";
    char dll_path[MAX_PATH] = { 0 };
    GetFullPathName(dll_name, MAX_PATH, dll_path, NULL);
    if (!inject.file_exists(dll_path)) {
        error("dll dow2injector.dll is not present", "File can't be found please redownload");
        return -1;
    }


    LPVOID loc = VirtualAllocEx(processh, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!loc) {
        CloseHandle(processh);
        error("Failed to allocate", "Failed to allocate memory in program");
        return -1;
    }

    bool err = WriteProcessMemory(processh, loc, dll_path, strlen(dll_path), nullptr);
    
    if (!err) {
        CloseHandle(processh);
        error("Failed to write", "Failed to write process memory on main injection stage");
        return -1;
    }
    
    HANDLE hthread = CreateRemoteThread(processh, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, loc, 0, nullptr);
    
    if (!hthread) {
        CloseHandle(processh);
        error("Failed to attach thread", "Failed to attach thread to dow2");
        return -1;
    }
    
    //error("Inject success", "Sucessfully injected");

    //Sleep(20000);
    WaitForSingleObject(hthread, INFINITE);
    CloseHandle(processh);
    VirtualFree(loc, strlen(dll_path), MEM_RELEASE);
    CloseHandle(hthread);*/
}
