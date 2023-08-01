// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"

//get solo game booting working
// -the lobby doesnt exist till a match is made, so we gotta create one first
// -maybe try getting force start working in regular pvp
//get new heros able to be added

DWORD base;


DWORD32 weirdglobal;

DWORD32 GameInfo;


//actual start game function
typedef int(__stdcall* StartGame)(int param1);
StartGame start_org = nullptr;
//StartLastStand parameter
DWORD32 slast_param1 = 0;
//parameter used in recretion of receiving a startgame packet
DWORD32 slast_param2 = 0;

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

typedef void(__stdcall *SetuphelperOnMessage)(int param1, int param2);
SetuphelperOnMessage setuph_org = nullptr;

typedef void(__stdcall *StartMultiplayerGame)(int param1);
StartMultiplayerGame startmg_org = nullptr;

bool start_mode = false;

typedef int* (__stdcall* Need1)(void* param1);
Need1 ned1 = nullptr;
typedef bool(__stdcall* Need2)(void* param1, void* param2);
Need2 ned2 = nullptr;
typedef void(__stdcall* Need3)(void* param1);
Need3 ned3 = nullptr;

void* p1;
int p2;

//00602B99 - 
//0053B02A - 
//00449C40 - 1

//do this now
//00449501 - 1 StartMultiplayerGame
//004491EC - 2 
//0047985B - 3
//004531F7 - 4
//00682013 - 5
//00680CAC - 6
//00680CAC - 6
//00680D17 - 7 - calls the previous function somewhere
//0040e637 - 8
//0041D961 - 9, - analyze the parameter from here down and see what we get, then trying passing to GameSetupFormStartGame
DWORD jmpbackaddr;
void __declspec(naked) MatchmakeButton() {
    __asm {
        pop ecx;
        push slast_param1;
        call slast_org;
        jmp [jmpbackaddr];
    }
}

//79CB9
//0045FEC7
//00458175

extern "C" void EZFUNCTION() {

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

typedef void(__thiscall *LobbyCallback)(void* tis, int param1);
LobbyCallback lobbyback_org = nullptr;

void __fastcall LobbyCallbackDetour(void* tis, void* unused, int param1) {
    p1 = tis;
    p2 = param1;
    lobbyback_org(tis, param1);
}

//in last stand just starts matchmaking even though this function call is wrong
typedef void(__fastcall *OnClickStart)(int param1, void* unused);
OnClickStart clickst_org = nullptr;

//00681B3C
//00E07293
typedef void(__stdcall *GameSetupFormStartGame)(void** param1);
GameSetupFormStartGame gmset_org = nullptr;

typedef int(__fastcall *CreateMatch)(void* param1);
CreateMatch createmat_org = nullptr;
//00E07C88
// //00E07291
//00DB8CC2
typedef void(__fastcall *MultiplayerOnClickStart)(int param1);
MultiplayerOnClickStart multistart_org = nullptr;

//ignore this for now
//use startmultiplayergame instead
//try finding the caller of last stand start game, then get the param from there, and recreate the function call around it
//look into MainFunctionCalling? and MenuCallbacks to get the parameter to multistart_org 
DWORD WINAPI MainThread(LPVOID param) {
    while (true) {
        if (GetAsyncKeyState(VK_F6) & 0x80000) {
            multistart_org(slast_param1);
            //gmset_org((void**)slast_param1);
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
        CreateThread(0, 0, MainThread, hModule, 0, 0);
        base = (DWORD)GetModuleHandleA("DOW2.exe");

        weirdglobal = (base + 0xf3567c);
        GameInfo = (base + 0xf35a78);
        slast_param1 = (base + 0xc89c84);
        slast_param2 = (base + 0xc8f86c);

        last_setmaps = reinterpret_cast<LastStandSetMaps>(base + 0x7a5de);
        lastlobby_org = reinterpret_cast<LastStandLobby>(base + 0x78aa1);
        lobbyback_org = reinterpret_cast<LobbyCallback>(base + 0x1412d0);

        start_org = reinterpret_cast<StartGame>(base + 0x5b9eb);
        slast_org = reinterpret_cast<StartLastStand>(base + 0x75ee3);
        enst_org = reinterpret_cast<EnableStartButton>(base + 0x7796e);
        clickst_org = reinterpret_cast<OnClickStart>(base + 0x79288);
        setuph_org = reinterpret_cast<SetuphelperOnMessage>(base + 0x49990);

        lastmatchmake = reinterpret_cast<LastStandStartMatchmake>(base + 0x7934a);
        gmset_org = reinterpret_cast<GameSetupFormStartGame>(base + 0x6fe0c);
        createmat_org = reinterpret_cast<CreateMatch>(base + 0x1edf80);
        multistart_org = reinterpret_cast<MultiplayerOnClickStart>(base + 0x6ffe3);
        startmg_org = reinterpret_cast<StartMultiplayerGame>(base + 0x49386);

        ned1 = reinterpret_cast<Need1>(base + 0x458df);
        ned2 = reinterpret_cast<Need2>(base + 0x45a18);
        ned3 = reinterpret_cast<Need3>(base + 0x4a7cb);

        //patching the call to LastStandSetMaps to get the param1 we need


        jmpbackaddr = (base + 0x792a2);
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x7929d), (DWORD)MatchmakeButton, 5);

        
        jmpbackaddr2 = (base + 0x47463);
        //JmpPatch(reinterpret_cast<BYTE*>(base + 0x47459), (DWORD)MidCheckGameValid, 5);


        //jmpbackaddr3 = (base + 0x1a34a);
        //JmpPatch(reinterpret_cast<BYTE*>(base + 0x1a291), (DWORD)MidAroundUnknownGame, 6);
       
        //NopPatch(reinterpret_cast<BYTE*>(base + 0x75f3a), 4);

        src = (BYTE*)"\xB8\x01\x00\x00\x00\x90";
        //MemPatch(reinterpret_cast<BYTE*>(base + 0x46a49), src, 6);
        //NopPatch(reinterpret_cast<BYTE*>(base + 0x46a59), 6);       

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach((void**)&last_setmaps, LastSetMapsDetour);
        DetourAttach((void**)&lastlobby_org, LastLobbyDetour);
        DetourAttach((void**)&lobbyback_org, LobbyCallbackDetour);
        DetourTransactionCommit();

        break;
    case DLL_PROCESS_DETACH:
        
        break;
    }
    return TRUE;
}

