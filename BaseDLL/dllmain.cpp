// dllmain.cpp : Defines the entry point for the DLL application.
#include "dllmain.h"

//returns a pointer of some kind
typedef DWORD32*(__thiscall *setGamemode)(DWORD32 index, DWORD32* address);
setGamemode setgame_target = reinterpret_cast<setGamemode>(0x004882c6); //function before hook

GamemodeMap map;

//Exception thrown at 0x00488164 in DOW2.exe: 0xC0000005: Access violation reading location 0x3E70F050.


//http://jbremer.org/x86-api-hooking-demystified/
//https://www.x86matthew.com/view_post?id=stealth_hook

//https://www.unknowncheats.me/forum/c-and-c-/154364-detourfunction-__thiscall.html
//https://www.unknowncheats.me/forum/programming-for-beginners/424330-hook-functions.html
//https://guidedhacking.com/threads/introduction-to-calling-conventions-for-beginners.20041/
// 
//is a member function of a class so we use fastcall to work around having to use thiscall
//still causing crashes
DWORD32* __fastcall setgamemodedetour(DWORD32 index, DWORD32* address) {
    //DWORD32* out = setgame_target(param2, in_ec);
    


    //std::ofstream file;
    //file.open("mod_logs\\gamemode.log");
    //try both eax and ebx
    //file << "Before assembly\n";

    Mode m = map.getMode(index);

    char ffa = m.ffa;
    char t_ffa = m.t_ffa;
    //getting access violation on eax access
    __asm {
        mov dl, ffa;
        mov byte ptr[ebx + 0x5b], dl;
        mov dl, t_ffa;
        mov byte ptr[ebx + 0x5c], dl;
        mov edx, address;
        mov dword ptr[ebx + 0x50], edx;
        mov edx, index;
        mov dword ptr[ebx + 0x54], edx;
    }

    /*char* stor;
    __asm mov[stor], ebx;
    *(stor + 0x5b) = (char)m.ffa;
    *(stor + 0x5c) = (char)m.t_ffa;

    DWORD32* dw = (DWORD32*)(stor + 0x50);
    *dw = in_ec;
    dw = (DWORD32*)(stor + 0x54);
    *dw = *param2;*/
   
    //std::string t = "Address eax: " + std::to_string((int)eax) + " Stor address: " + std::to_string((int)stor);
    //MessageBoxA(0, t.c_str(), "Same", 0);
    
    //file << "Function finished\n";
    //file.close();
    return address;
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
DWORD base;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    //DetourRestoreAfterWith();
    //DetourIsHelperProcess();
    std::ofstream f;
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        base = (DWORD)GetModuleHandleA("DOW2.exe");
        setgame_target = reinterpret_cast<setGamemode>(base + 0x882c6);
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
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach((void**)&setgame_target, setgamemodedetour);
        DetourTransactionCommit();
        //file.close();
        break;
    }


    return TRUE;
}

