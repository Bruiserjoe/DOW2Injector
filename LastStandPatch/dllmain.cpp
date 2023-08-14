// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"

//get solo game booting working
// -maybe try getting force start working in regular pvp
//get new heros able to be added

DWORD base;
HMODULE platform;

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
//0046fe0c - 0 or laststandstartgame
//00449501 - 1 StartMultiplayerGame - doesnt matter what it passes to function below this(probably actually does)
//004491EC - 2 - passes esi to StartMultiplayerGame, esi is eax
//0047985B - 3 - eax is set to "dword ptr[esi + 0xa0], esi is ecx
//004531F7 - 4 - passes this, 
//00682013 - 5 - passes this
//00680CAC - 6 - passes strange member of param1?
//00680D17 - 7 - calls the previous function somewhere(just return for above spot)
//0040e637 - 8 - passes an member of this functions param1(ecx), is in ebx, so member of application
//0041D961 - 9, - passes esi to ecx then calls the function above, esi is Application variable

//0041D961 - Applicaton, in ecx
//0040e637 - mov param1, dword ptr[eax + 0x137c];, mov eax, dword ptr[ebx + 0x4];, mov ebx, ecx(param1);
//00680CAC - mov ecx, edi; mov edi, dword ptr[esi]; mov esi, dword ptr[eax + 0x4]; mov eax, dword ptr[ecx]; mov ecx, dword ptr[ebx]; mov ebx, dword ptr[ebp + param1];
//00682013 - passes param1, which is above value, just ecx unchanged
//004531F7 - mov this(ecx), dword ptr[esi]; mov esi, dword ptr[edi + 0x34]; mov edi, this(ecx);
//0047985B - mov eax, dword ptr[esi + 0xa0];, mov esi, ecx(prev param1);
//004491EC - push esi;, mov esi, eax;

//find where the param + 0x2c is edited and hook that function, look at second function in call tree

typedef void(__stdcall *MultiplayerGameStartHandler)(); //takes a parameter in eax
MultiplayerGameStartHandler mgstarth_org = nullptr;
DWORD mgstarth_addr = 0;

typedef void* (__stdcall *GetApplication)();
GetApplication get_app = nullptr;
DWORD32 startmultiparam = 0;
void setStartMultiplayerParam() {
    //Plat::App::GetApplication() to get base
    DWORD32 base1 = (DWORD32)get_app();
    base1 = (base + 0xf35088);
    __asm {
        push edi;
        push eax;
        mov eax, dword ptr[base1];
        mov edi, dword ptr[eax];
        mov eax, edi;
        /*mov edi, dword ptr[eax + 0x4];
        mov eax, edi;
        mov edi, dword ptr[eax + 0x137c];
        mov eax, edi;*/
        //two possiblitues
       
        
        mov edi, dword ptr[eax];
        mov eax, dword ptr[edi];
        mov edi, dword ptr[eax + 0x4];
        mov eax, dword ptr[edi];

        mov edi, dword ptr[eax + 0x34];
        mov eax, dword ptr[edi];

        mov edi, dword ptr[eax + 0xa0];

        mov startmultiparam, edi;
        pop eax
        pop edi;
    }
    
}

extern "C" void EZFUNCTION() {
    NopPatch(reinterpret_cast<BYTE*>(base + 0x491e4), 2);
    NopPatch(reinterpret_cast<BYTE*>(base + 0x491ec), 4);
    NopPatch(reinterpret_cast<BYTE*>(base + 0x491f0), 4);
}

extern "C" void EZFUNCTION2() {
    MemPatch(reinterpret_cast<BYTE*>(base + 0x491e4), (BYTE*)"\x75\x0e", 2);
    MemPatch(reinterpret_cast<BYTE*>(base + 0x491ec), (BYTE*)"\x83\x4e\x2c\xff", 4);
    MemPatch(reinterpret_cast<BYTE*>(base + 0x491f0), (BYTE*)"\xc6\x46\x30\x00", 4);
}


DWORD jmpbackaddr;
void __declspec(naked) MatchmakeButton() {
    __asm {
        pop ecx;
        
        call setStartMultiplayerParam;
        call EZFUNCTION;
        /*push eax;
        call startmg_org;
        */
        mov ecx, startmultiparam;
        mov edx, dword ptr[ecx + 0x2c];
        //try and figure out why chaning ecx+0x2c throws an exception, make sure the startmultiparam is actual value we need
        mov eax, ecx;
        call mgstarth_org; //this doesnt start game because the +0x2c isnt zero
        call EZFUNCTION2;
        jmp [jmpbackaddr];
    }
}

//79CB9
//0045FEC7
//00458175


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
            //multistart_org(slast_param1);
            //gmset_org((void**)slast_param1);
            //setStartMultiplayerParam();
            __asm {
               // mov eax, startmultiparam;
                //call mgstarth_org;
            }
            setStartMultiplayerParam();
            DWORD32 t = startmultiparam;
            __asm {
                mov ecx, t;
                mov edx, dword ptr [ecx + 0x2c];
                mov dword ptr[ecx + 0x2c], 0x0;
                mov eax, startmultiparam;
                call mgstarth_org;
            }
            //mgstarth_org();
            //startmg_org(startmultiparam);
            //startmg_org(startmultiparam);
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
        platform = GetModuleHandleA("Platform.dll");

        weirdglobal = (base + 0xf3567c);
        GameInfo = (base + 0xf35a78);
        slast_param1 = (base + 0xc89c84);
        slast_param2 = (base + 0xc8f86c);
        mgstarth_addr = (base + 0x4917a);

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

        mgstarth_org = reinterpret_cast<MultiplayerGameStartHandler>(base + 0x4917a);
        if (platform) {
            get_app = reinterpret_cast<GetApplication>(GetProcAddress(platform, MAKEINTRESOURCEA(79)));
            //setStartMultiplayerParam();
        }

        ned1 = reinterpret_cast<Need1>(base + 0x458df);
        ned2 = reinterpret_cast<Need2>(base + 0x45a18);
        ned3 = reinterpret_cast<Need3>(base + 0x4a7cb);

        //patching the call to LastStandSetMaps to get the param1 we need


        jmpbackaddr = (base + 0x792a2);
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x7929d), (DWORD)MatchmakeButton, 5);

        //noping jzs in StartMultiplayerGame, might break everything
        //NopPatch(reinterpret_cast<BYTE*>(base + 0x493bf), 6);
        //NopPatch(reinterpret_cast<BYTE*>(base + 0x493cd), 6);

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

