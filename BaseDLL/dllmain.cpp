// dllmain.cpp : Defines the entry point for the DLL application.
#include "dllmain.h"


typedef void(__thiscall *setGamemode)(void* ecx, DWORD32 param2);
setGamemode setgame_target = reinterpret_cast<setGamemode>(0x004882c6); //function before hook

GamemodeMap map;



//rebuild
//http://jbremer.org/x86-api-hooking-demystified/
//https://www.x86matthew.com/view_post?id=stealth_hook

//https://www.unknowncheats.me/forum/c-and-c-/154364-detourfunction-__thiscall.html
//https://www.unknowncheats.me/forum/programming-for-beginners/424330-hook-functions.html
//https://guidedhacking.com/threads/introduction-to-calling-conventions-for-beginners.20041/
// 
//is a member function of a class so we use fastcall to work around having to use thiscall
//still causing crashes
void __fastcall setgamemodedetour(void* ecx, DWORD32 param2) {
    std::ofstream file;
    file.open("mod_logs\\gamemode.log");
    //try both eax and ebx
    file << "Before assembly\n";
    char* stor;
    __asm mov [stor], ebx;
    if (stor != NULL) {
        file << "Stor value: " + std::to_string((int)stor) + "\n";
        file << "ECX value: " + std::to_string((int)ecx) + "\n";
        size_t cast = (size_t)ecx;

        Mode m = map.getMode(cast);
        *(stor + 0x5b) = m.ffa;
        *(stor + 0x5c) = m.t_ffa;

        DWORD32* dw = (DWORD32*)(stor + 0x50);
        *dw = param2;
        dw = (DWORD32*)(stor + 0x54);
        *dw = (DWORD32)ecx;
    }
    file << "Function finished\n";
    file.close();
    return;
    //return setgame_target(ecx, param2);
}



//hook the function which edits the displayed gamemode value and save that string for use

typedef void(__stdcall *GamemodeChange)();
GamemodeChange gc = reinterpret_cast<GamemodeChange>(0x00486f6a);

//recreate locstring so we can get the locstring value from ebp like in possiblegamemodestring

void __stdcall gamechangedetour() {
    gc();
    char* stor;
    __asm mov [stor], ebp;
    

}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    DetourRestoreAfterWith();
    DetourIsHelperProcess();
    std::ofstream f;
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        map.readConfig("mods\\gmd.cfg");
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach((void**)&setgame_target, setgamemodedetour);
        DetourTransactionCommit();
        

        //MessageBoxA(NULL, "DLL Injected", "DLL injected", MB_OK);
        //CreateThread(0, 0, MainThread, hModule, 0, 0);
        break;
    case DLL_PROCESS_DETACH:
        //f.open("mod_logs\\detachg.log");
        //f << "Detached gamemode\n";
        /*DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach((void**)&setgame_target, setgamemodedetour);
        DetourTransactionCommit();*/
        //file.close();
        break;
    }


    return TRUE;
}

