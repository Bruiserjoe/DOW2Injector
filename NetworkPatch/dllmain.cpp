// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"
DWORD base = 0;
HMODULE debug;
typedef void(__cdecl* Timestampedf)(const char*, ...);
typedef void(__cdecl* Fatalf)(const char*, ...);

Timestampedf Timestampedtracef;
Fatalf Fatal_f;

DWORD32 jmpback_midConnect = 0;
DWORD Net__Connect = 0;
void __declspec(naked) MidConnect() {
    __asm {
        call Net__Connect;
        jmp[jmpback_midConnect];
    }
}

// what's best way to start???
// maybe hooking every windows function??

typedef int (WINAPI* send_t)(SOCKET s, const char* buf, int len, int flags);
send_t original_send = send;

int WINAPI my_send(SOCKET s, const char* buf, int len, int flags) {
    // Log, modify, or inspect the data
    Timestampedtracef("[Hooked send] Data length: %d\n", len);
    if (buf && len > 0) {
        Timestampedtracef("Data: %.*s\n", len, buf);
    }

    // Call the original send function
    return original_send(s, buf, len, flags);
}

typedef int (WINAPI* connect_t)(SOCKET s, const struct sockaddr* name, int namelen);
connect_t original_connect = connect;

int WINAPI my_connect(SOCKET s, const struct sockaddr* name, int namelen) {
    char ipStr[INET_ADDRSTRLEN] = { 0 };

    if (name->sa_family == AF_INET) {
        struct sockaddr_in* addr_in = (struct sockaddr_in*)name;
        inet_ntop(AF_INET, &(addr_in->sin_addr), ipStr, INET_ADDRSTRLEN);
        Timestampedtracef("[HOOKED connect] Connecting to IP: %s, Port: %d\n",
            ipStr, ntohs(addr_in->sin_port));
    }

    return original_connect(s, name, namelen);
}



DWORD WINAPI MainThread(LPVOID param) {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    DetourAttach(&(PVOID&)original_send, my_send);
    DetourAttach(&(PVOID&)original_connect, my_connect);

    return DetourTransactionCommit() == NO_ERROR;
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        base = (DWORD)GetModuleHandleA("DOW2.exe");
        debug = GetModuleHandleA("Debug.dll");
        if (debug) {
            Timestampedtracef = reinterpret_cast<Timestampedf>(GetProcAddress(debug, MAKEINTRESOURCEA(50)));
            Fatal_f = reinterpret_cast<Fatalf>(GetProcAddress(debug, MAKEINTRESOURCEA(31)));
        }
        jmpback_midConnect = base + 0x23B615;
        Net__Connect = base + 0x258030;
        // JmpPatch(reinterpret_cast<BYTE*>(base + 0x23B610), (DWORD)MidConnect, 5);
        CreateThread(0, 0, MainThread, hModule, 0, 0);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

