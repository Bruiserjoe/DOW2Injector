// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"

//get solo game booting working
// -maybe try getting force start working in regular pvp
// -fuck with the function before the startgame call in laststandstartgame, use x64dbg to find problem in it
//get new heros able to be added

DWORD base;


DWORD32 weirdglobal;

DWORD32 GameInfo;


//actual start game function
typedef int(__stdcall* StartGame)(int param1);
StartGame start_org = nullptr;
//StartLastStand parameter
DWORD32 slast_param1 = 0;

//function which starts last stand
typedef void(__stdcall* StartLastStand)(int param1);
StartLastStand slast_org = nullptr;
//function which supposedly enables the start button on the page
typedef void(__fastcall* EnableStartButton)(void* param1);
EnableStartButton enst_org = nullptr;

typedef void(__stdcall *LastStandStartMatchmake)(int param1);
LastStandStartMatchmake lastmatchmake = nullptr;

typedef void(__thiscall *LastStandLobbyUpdate)(void* tis, DWORD32 *param1);
LastStandLobbyUpdate lastlobbyupdate = nullptr;



DWORD jmpbackaddr;
void __declspec(naked) MatchmakeButton() {
    __asm {
        pop ecx;
        push slast_param1;
        call slast_org;
        jmp [jmpbackaddr];
    }
}

//using this to edit the value we want in SetupHelperOnMessage
DWORD jmpbackaddr2;
void __declspec(naked) MidCheckGameValid() {
    __asm {
        push edi;
        mov dword ptr[ebp + -0xe8], 0x1;
        pop edi;
        jmp[jmpbackaddr2];
    }
}

DWORD jmpbackaddr3;
void __declspec(naked) MidAroundUnknownGame() {
    __asm {
        mov dword ptr[ebx + 0x814], 0x1;
        //mov dword ptr[ebx], 0x1;
        
        jmp[jmpbackaddr3];
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


DWORD WINAPI MainThread(LPVOID param) {
    while (true) {
        if (GetAsyncKeyState(VK_F6) & 0x80000) {

        }
        Sleep(100);
    }
    return 0;
}
//00449C9E function that calls LastStandLobbyUpdate
//00449C40 function that calls LastStandStartGame


//004476BC
//0044746F

//01813A00
//004C9070
//0057DD50
BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{

    BYTE* src;
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        base = (DWORD)GetModuleHandleA("DOW2.exe");

        weirdglobal = (base + 0xf3567c);
        GameInfo = (base + 0xf35a78);
        slast_param1 = (base + 0xc89c84);

        last_setmaps = reinterpret_cast<LastStandSetMaps>(base + 0x7a5de);
        lastlobby_org = reinterpret_cast<LastStandLobby>(base + 0x78aa1);

        start_org = reinterpret_cast<StartGame>(base + 0x5b9eb);
        slast_org = reinterpret_cast<StartLastStand>(base + 0x75ee3);
        enst_org = reinterpret_cast<EnableStartButton>(base + 0x7796e);
        clickst_org = reinterpret_cast<OnClickStart>(base + 0x79288);

        lastmatchmake = reinterpret_cast<LastStandStartMatchmake>(base + 0x7934a);

        //patching the call to LastStandSetMaps to get the param1 we need


        jmpbackaddr = (base + 0x792a2);
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x7929d), (DWORD)MatchmakeButton, 5);

        
        jmpbackaddr2 = (base + 0x47463);
        //JmpPatch(reinterpret_cast<BYTE*>(base + 0x47459), (DWORD)MidCheckGameValid, 5);


        jmpbackaddr3 = (base + 0x1a34a);
        //JmpPatch(reinterpret_cast<BYTE*>(base + 0x1a291), (DWORD)MidAroundUnknownGame, 6);
       
        //NopPatch(reinterpret_cast<BYTE*>(base + 0x75f3a), 4);

        src = (BYTE*)"\xB8\x01\x00\x00\x00\x90";
        //MemPatch(reinterpret_cast<BYTE*>(base + 0x46a49), src, 6);
        //NopPatch(reinterpret_cast<BYTE*>(base + 0x46a59), 6);       

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

