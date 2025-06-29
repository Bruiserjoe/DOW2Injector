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
//  -figure out what the first ip connected to does
//  -00BCAEA0 is main connect
//  -hook wherever the ip is set and change to desired ip
//  -if can hook ips then we can start work on actual server

// following rllinkasynchttp up
// 0055919C
// 0062DEC5

// Helper: Get remote IP from socket
const char* GetRemoteIP(SOCKET s) {
    sockaddr_storage addr;
    int addr_len = sizeof(addr);
    char ipstr[INET6_ADDRSTRLEN] = { 0 };

    if (getpeername(s, (sockaddr*)&addr, &addr_len) == 0) {
        void* ip_ptr = nullptr;

        if (addr.ss_family == AF_INET) {
            sockaddr_in* ipv4 = (sockaddr_in*)&addr;
            ip_ptr = &(ipv4->sin_addr);
        }
        else if (addr.ss_family == AF_INET6) {
            sockaddr_in6* ipv6 = (sockaddr_in6*)&addr;
            ip_ptr = &(ipv6->sin6_addr);
        }

        if (ip_ptr) {
            inet_ntop(addr.ss_family, ip_ptr, ipstr, sizeof(ipstr));
            return ipstr;
        }
    }

    return "Unknown";
}

typedef int (WINAPI* send_t)(SOCKET s, const char* buf, int len, int flags);
send_t original_send = send;

int WINAPI my_send(SOCKET s, const char* buf, int len, int flags) {
    // Log, modify, or inspect the data
    // std::string remoteIP = GetRemoteIP(s);
    Timestampedtracef("[Hooked send] Remote IP: %s\n", GetRemoteIP(s));
    Timestampedtracef("[Hooked send] Data length: %d\n", len);
    if (buf && len > 0) {
        Timestampedtracef("Data: %.*s\n", len, buf);
    }

    // Call the original send function
    return original_send(s, buf, len, flags);
}

static decltype(&recv) real_recv = recv;

int WSAAPI my_recv(SOCKET s, char* buf, int len, int flags)
{
    int ret = real_recv(s, buf, len, flags);

    if (ret > 0) {
        // Log the received data
        Timestampedtracef("[Hooked recv] Remote IP: %s\n", GetRemoteIP(s));
        Timestampedtracef("[Hooked recv] Received %d bytes:\n", ret);
        //fwrite(buf, 1, ret, stdout);
        //printf("\n");
    }

    return ret;
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

typedef int(__thiscall* HttpRequestAsync_t)(DWORD_PTR dwContext);
HttpRequestAsync_t TrueHttpRequestAsync = nullptr;

// this is unused by dow2 I think
int __fastcall MyHttpRequestAsync(DWORD_PTR dwContext)
{
    // Log or modify parameters here
    Timestampedtracef("[Hook] HttpRequestAsync called! dwContext = 0x%p\n", *(DWORD*)(dwContext + 640));

    // Optionally call the original function
    return TrueHttpRequestAsync(dwContext);
}

typedef int(__stdcall* WorldwideLoginServiceCtor_t)(int a1, int a2);
WorldwideLoginServiceCtor_t original_ctor = nullptr;

int __stdcall MyWorldwideLoginServiceCtor(int a1, int a2)
{
    // Log or modify input parameters
    Timestampedtracef("[Hook] WorldwideLoginService_Constructor called! a1 = 0x%X, a2 = 0x%X\n", a1, a2);
    Timestampedtracef("[HOOK] a2 %d", a2);
    // Optionally modify a1/a2 before calling the original
    // a1 = ...; a2 = ...;

    int result = original_ctor(a1, a2);

    // Log or modify result
    Timestampedtracef("[Hook] Constructor returned: 0x%X\n", result);

    return result;
}

DWORD WINAPI MainThread(LPVOID param) {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    DetourAttach(&(PVOID&)original_send, my_send);
    DetourAttach(&(PVOID&)original_connect, my_connect);
    DetourAttach(&(PVOID&)original_ctor, MyWorldwideLoginServiceCtor);
    DetourAttach(&(PVOID&)real_recv, my_recv);
    //DetourAttach(&(PVOID&)TrueHttpRequestAsync, MyHttpRequestAsync);
    return DetourTransactionCommit() == NO_ERROR;
    return 0;
}
DWORD jmpback_midnetinit = 0;
void __declspec(naked) MidNetInit() {
    __asm {
        mov eax, fs:0x0;
        jmp[jmpback_midnetinit];
    }
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
        TrueHttpRequestAsync = reinterpret_cast<HttpRequestAsync_t>(base + 0x12E630);
        original_ctor = reinterpret_cast<WorldwideLoginServiceCtor_t>(base + 0x22A200);
        // JmpPatch(reinterpret_cast<BYTE*>(base + 0x23B610), (DWORD)MidConnect, 5);
        CreateThread(0, 0, MainThread, hModule, 0, 0);
        jmpback_midnetinit = base + 0x168AE;
        JmpPatch(reinterpret_cast<BYTE*>(base + 0x168A8), (DWORD)MidNetInit, 6);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

