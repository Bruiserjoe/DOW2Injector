// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"
DWORD base = 0;
HMODULE debug;
typedef void(__cdecl* Timestampedf)(const char*, ...);
typedef void(__cdecl* Fatalf)(const char*, ...);

Timestampedf Timestampedtracef;
Fatalf Fatal_f;
WSAData wsa;

DWORD32 jmpback_midConnect = 0;
DWORD Net__Connect = 0;
std::string folder = "";
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

// first connect
// 00BCAFFF

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

typedef int(__cdecl* sub_BC09F0_t)(DWORD* a1, DWORD* a2);
sub_BC09F0_t original_sub_BC09F0 = nullptr;

int __cdecl DetouredSub_BC09F0(DWORD* a1, DWORD* a2)
{
    // Your custom code here, e.g. logging or modifying the parameters
    Timestampedtracef("[NETPATCH] BC09F0 hook!");

    // Optionally, modify the parameters or perform custom behavior
    // You can also call the original function if needed
    const char* cc = (const char*)a1;
    if (original_sub_BC09F0) {
        return original_sub_BC09F0(a1, a2);  // Call the original function
    }

    // Custom behavior if you don’t want to call the original function
    return 0;
}


typedef int (WINAPI* send_t)(SOCKET s, const char* buf, int len, int flags);
send_t original_send = send;

/*
00BE184E
00BFB7E4
00BFB376
00C11A1D
00BDE65D
00BD92BD
00BC2E6D
00BD0365
00BD193A

00BCA078
00BC0245
00BC0AD3
005433D8
0055919C
006299BC
a1 of this^ is eventually passed down in parts to 00BE184E

*/

uintmax_t send_count = 0;
int WINAPI my_send(SOCKET s, const char* buf, int len, int flags) {
    // Log, modify, or inspect the data
    // std::string remoteIP = GetRemoteIP(s);
    Timestampedtracef("[Hooked send] Remote IP: %s\n", GetRemoteIP(s));
    Timestampedtracef("[Hooked send] Data length: %d\n", len);
    if (buf && len > 0) {
        Timestampedtracef("Data: %.*s\n", len, buf);
    }
    std::ofstream f;
    std::string log = folder+"send_packet_" + std::to_string(send_count) + ".txt";
    f.open(log, std::ios::binary);
    f.write(buf, len);
    f.close();
    send_count++;
    std::ofstream simp;
    simp.open(folder+"send_callers.txt", std::ios::app);
    void* caller = getCallerAddress();
    void* nextup = getCallerAddressNextUp();
    log = "caller " + std::to_string(send_count - 1) + PointerToHexString(caller) + "," + PointerToHexString(nextup) + "\n";
    simp.write(log.c_str(), log.size());
    simp.close();
    // Call the original send function
    return original_send(s, buf, len, flags);
}

static decltype(&recv) real_recv = recv;
uintmax_t recv_count = 0;
int WSAAPI my_recv(SOCKET s, char* buf, int len, int flags)
{
    int ret = real_recv(s, buf, len, flags);

    if (ret > 0) {
        // Log the received data
        Timestampedtracef("[Hooked recv] Remote IP: %s\n", GetRemoteIP(s));
        Timestampedtracef("[Hooked recv] Received %d bytes:\n", ret);
        std::ofstream f;
        std::string log = folder+"recv_packet_" + std::to_string(recv_count) + ".txt";
        f.open(log, std::ios::binary);
        f.write(buf, len);
        f.close();
        std::ofstream simp;
        simp.open(folder + "recv_callers.txt", std::ios::app);
        void* caller = getCallerAddress();
        void* nextup = getCallerAddressNextUp();
        log = "caller " + std::to_string(recv_count) + PointerToHexString(caller) + "," + PointerToHexString(nextup) + "\n";
        simp.write(log.c_str(), log.size());
        simp.close();
        recv_count++;
        //fwrite(buf, 1, ret, stdout);
        //printf("\n");
    }

    return ret;
}

typedef int (WINAPI* connect_t)(SOCKET s, const struct sockaddr* name, int namelen);
connect_t original_connect = connect;

int WINAPI my_connect(SOCKET s, const struct sockaddr* name, int namelen) {
    char ipStr[INET_ADDRSTRLEN] = { 0 };
    struct sockaddr_in* addr_in = (struct sockaddr_in*)name;
    sockaddr_in clientService;
    clientService.sin_family = AF_INET;
    InetPtonA(AF_INET, "127.0.0.1", &clientService.sin_addr.s_addr);
    clientService.sin_port = addr_in->sin_port;
    if (name->sa_family == AF_INET) {
        inet_ntop(AF_INET, &(addr_in->sin_addr), ipStr, INET_ADDRSTRLEN);
        Timestampedtracef("[HOOKED connect] Connecting to IP: %s, Port: %d\n",
            ipStr, ntohs(addr_in->sin_port));
    }
    inet_ntop(AF_INET, &(clientService.sin_addr), ipStr, INET_ADDRSTRLEN);
    Timestampedtracef("[HOOKED connect] Changing to IP: %s, Port: %d\n",
        ipStr, ntohs(addr_in->sin_port));
    return original_connect(s, (sockaddr*)&clientService, namelen);
}

typedef int(__thiscall* HttpRequestAsync_t)(DWORD_PTR dwContext);
HttpRequestAsync_t TrueHttpRequestAsync = nullptr;

// this is unused by dow2 I think
// HttpSendRequestEx performs both the send and the receive for the response. This does not allow the application to send any extra data beyond the single buffer that was passed to HttpSendRequestEx. 
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

typedef int(__thiscall* CreateSocket_t)(void* _this, int a2, u_short hostshort, void* Src);
CreateSocket_t OriginalCreateSocket = nullptr;
int __fastcall DetourCreateSocket(void* _this, void* /*not used*/, int a2, u_short hostshort, void* Src) {
    // Logging or modifying parameters
    Timestampedtracef("[+] Hooked CreateSocket: a2 = %d, hostshort = %u\n", a2, hostshort);

    // Optionally change parameters
    // a2 = some_other_value;

    // Call original
    return OriginalCreateSocket(_this, a2, hostshort, Src);
}


// Typedef for the original function
typedef int(__fastcall* sub_BCACE0_t)(SOCKET* a1, void* dummy, int a2, DWORD* a3);

// Global pointer to the original
sub_BCACE0_t Original_sub_BCACE0 = nullptr;

// Detour implementation
int __fastcall Hooked_sub_BCACE0(SOCKET* a1, void* /*not used*/, int a2, DWORD* a3) {
    printf("[+] Hooked sub_BCACE0: SOCKET=%p, a2=%d, a3[0]=%08X\n", a1, a2, a3 ? a3[0] : 0);

    // You can modify args here, or block the call, or log, etc.

    return Original_sub_BCACE0(a1, nullptr, a2, a3);  // Call original
}

DWORD WINAPI MainThread(LPVOID param) {

    folder = "packet_log/netreadd2_" + GetCurrentTimeString()+"/";
    createFolder(folder);
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    DetourAttach(&(PVOID&)original_send, my_send);
    DetourAttach(&(PVOID&)original_connect, my_connect);
    DetourAttach(&(PVOID&)original_ctor, MyWorldwideLoginServiceCtor);
    DetourAttach(&(PVOID&)real_recv, my_recv);
    DetourAttach(&(PVOID&)OriginalCreateSocket, DetourCreateSocket);
    DetourAttach(&(PVOID&)original_sub_BC09F0, DetouredSub_BC09F0);
    //DetourAttach(&(PVOID&)Original_sub_BCACE0, Hooked_sub_BCACE0);
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

//Net::CreatePacketHeader could be useful
// look at wireshark net capture
// figure out where the buffer is created for http send


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
        Original_sub_BCACE0 = reinterpret_cast<sub_BCACE0_t>(base + 0x7CACE0);
        OriginalCreateSocket = reinterpret_cast<CreateSocket_t>(base + 0x25B7C0);
        original_sub_BC09F0 = reinterpret_cast<sub_BC09F0_t>(base + 0x7C09F0);
        if (WSAStartup(0x202u, &wsa))
        {
            Fatal_f("Failed wsa startup!");
        }
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

