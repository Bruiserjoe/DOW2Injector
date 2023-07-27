// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"

//get solo game booting working
// -maybe try getting force start working in regular pvp
//get new heros able to be added

DWORD base;


DWORD32 weirdglobal;




//actual start game function
typedef int(__stdcall* StartGame)(int param1);
StartGame start_org = nullptr;
//function which starts last stand
typedef void(__stdcall* StartLastStand)(int param1);
StartLastStand slast_org = nullptr;
//function which supposedly enables the start button on the page
typedef void(__fastcall* EnableStartButton)(void* param1);
EnableStartButton enst_org = nullptr;


DWORD jmpbackaddr;
void __declspec(naked) TestFunc() {
    __asm {
        //pop edx;
        mov ecx, 0x0;
        mov cl, 0x1;
        mov byte ptr[weirdglobal + 0xc], cl;
        //push 0x0;
        //call start_org;
        call slast_org;
        jmp [jmpbackaddr];
    }
}
DWORD jmpbackaddr2;
void __declspec(naked) TestFunc2() {
    __asm {
        mov eax, map_list;
        jmp [jmpbackaddr2];
    }
}


typedef void(__stdcall *LastStandSetMaps)(void* param1);
LastStandSetMaps last_setmaps = nullptr;

int LobbyParam = 0;

void __stdcall LastSetMapsDetour(void* param1) {
    //LobbyParam = param1;
    last_setmaps(param1);
}

typedef void(__fastcall *LastStandLobby)(int param1, void* unused, void* unused2);
LastStandLobby lastlobby_org = nullptr;

void __fastcall LastLobbyDetour(int param1, void* unused, void* unused2) {
    LobbyParam = param1;
    lastlobby_org(param1, unused, unused2);
}



//in last stand just starts matchmaking even though this function call is wrong
typedef void(__fastcall *OnClickStart)(int param1, void* unused);
OnClickStart clickst_org = nullptr;



BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{


    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        base = (DWORD)GetModuleHandleA("DOW2.exe");

        weirdglobal = (base + 0xf3567c);

        last_setmaps = reinterpret_cast<LastStandSetMaps>(base + 0x7a5de);
        lastlobby_org = reinterpret_cast<LastStandLobby>(base + 0x78aa1);

        start_org = reinterpret_cast<StartGame>(base + 0x5b9eb);
        slast_org = reinterpret_cast<StartLastStand>(base + 0x75ee3);
        enst_org = reinterpret_cast<EnableStartButton>(base + 0x7796e);
        clickst_org = reinterpret_cast<OnClickStart>(base + 0x79288);

        //patching the call to LastStandSetMaps to get the param1 we need


        jmpbackaddr = (base + 0x792a2);
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x7929d), (DWORD)TestFunc, 5);

        NopPatch(reinterpret_cast<BYTE*>(base + 0x75f3a), 4);

        

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach((void**)&last_setmaps, LastSetMapsDetour);
        DetourAttach((void**)&lastlobby_org, LastLobbyDetour);
        DetourTransactionCommit();

        break;
    case DLL_PROCESS_DETACH:
        
        break;
    }
    return TRUE;
}

