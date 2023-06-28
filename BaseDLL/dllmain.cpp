// dllmain.cpp : Defines the entry point for the DLL application.
#include "dllmain.h"


typedef int(__thiscall *setGamemode)(void* ecx, DWORD32 param2);
setGamemode setgame_target = reinterpret_cast<setGamemode>(0x004882c6); //function before hook


//http://jbremer.org/x86-api-hooking-demystified/
//https://www.x86matthew.com/view_post?id=stealth_hook

//https://www.unknowncheats.me/forum/c-and-c-/154364-detourfunction-__thiscall.html
//https://www.unknowncheats.me/forum/programming-for-beginners/424330-hook-functions.html
//https://guidedhacking.com/threads/introduction-to-calling-conventions-for-beginners.20041/
// 
//is a member function of a class so we use fastcall to work around having to use thiscall
int __fastcall setgamemodedetour(void* ecx, DWORD32 param2) {
    //try both eax and ebx

    char* stor;
    __asm mov [stor], ebx;

    if (ecx == (void*)1) {
        *(stor + 0x5b) = 1;
        *(stor + 0x5c) = 0;
    }
    else if (ecx == (void*)2) {
        *(stor + 0x5b) = 0;
        *(stor + 0x5c) = 1;
    }
    else if (ecx == (void*)4) {
        *(stor + 0x5b) = 1;
        *(stor + 0x5c) = 0;
    }
    else {
        *(stor + 0x5b) = 0; //ffa 
        *(stor + 0x5c) = 0; //team ffa
    }

    DWORD32* dw = (DWORD32*)(stor + 0x50);
    *dw = param2;
    dw = (DWORD32*)(stor + 0x54);
    *dw = (DWORD32)ecx;

    return 4;
    //return setgame_target(ecx, param2);
}

//find where the strings for the gamemodes are stored so we can utilize them
//004882c6
DWORD APIENTRY MainThread(LPVOID param) {
   while (true) {
       if (GetAsyncKeyState(VK_F6) & 0x80000) {
           MessageBoxA(NULL, "F6 Press", "F6", MB_OK);
       }
       Sleep(1000);
   }
   return 0;
}

//pretend to be XThread.dll and use that to get loaded instead of injecting


BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    DetourRestoreAfterWith();
    DetourIsHelperProcess();
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach((void**)&setgame_target, setgamemodedetour);
        DetourTransactionCommit();

        //MessageBoxA(NULL, "DLL Injected", "DLL injected", MB_OK);
        //CreateThread(0, 0, MainThread, hModule, 0, 0);
        break;
    case DLL_PROCESS_DETACH:
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach((void**)&setgame_target, setgamemodedetour);
        DetourTransactionCommit();
        break;
    }


    return TRUE;
}

