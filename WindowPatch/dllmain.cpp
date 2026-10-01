// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"
DWORD base;
HMODULE plat;
HMODULE spooge;
HWND* hwnd_global = nullptr;

DWORD plat_window_setup_orig = 0;
// only called for windowed windows, fullscreen has seperate function for some reason
DWORD jmpback = 0;
void __declspec(naked) PlatWindowModeSet() {
    __asm {
        and eax, 0xB0000000;
        add eax, 0x90000000;
        jmp[jmpback];
    }
}

WNDPROC original_wndproc = nullptr;
DWORD* plat_window_fullscreen_flag = nullptr;
// real client size, used by Platform's mouse pixel <-> normalized conversions
DWORD client_width = 0;
DWORD client_height = 0;

bool GetWindowMonitorRect(HWND hwnd, RECT* rect) {
    MONITORINFO monitor_info{};
    monitor_info.cbSize = sizeof(monitor_info);
    if (!GetMonitorInfoA(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor_info)) {
        return false;
    }
    *rect = monitor_info.rcMonitor;
    return true;
}

LRESULT CALLBACK MonitorSizedWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_WINDOWPOSCHANGING &&
        plat_window_fullscreen_flag && *plat_window_fullscreen_flag &&
        !IsIconic(hwnd)) {
        WINDOWPOS* pos = reinterpret_cast<WINDOWPOS*>(lparam);
        RECT monitor_rect;
        // skip minimize transitions, which move the window to -32000
        if (!(pos->flags & SWP_NOSIZE) && pos->x > -32000 && GetWindowMonitorRect(hwnd, &monitor_rect)) {
            pos->x = monitor_rect.left;
            pos->y = monitor_rect.top;
            pos->cx = monitor_rect.right - monitor_rect.left;
            pos->cy = monitor_rect.bottom - monitor_rect.top;
            pos->flags &= ~SWP_NOMOVE;
        }
    }
    return CallWindowProcA(original_wndproc, hwnd, msg, wparam, lparam);
}

void __stdcall ResizeWindowToMonitor() {
    if (!hwnd_global || !*hwnd_global) {
        return;
    }

    HWND hwnd = *hwnd_global;
    // Spooge restores its saved wndproc on device teardown, removing ours
    if (reinterpret_cast<WNDPROC>(GetWindowLongA(hwnd, GWL_WNDPROC)) != MonitorSizedWndProc) {
        WNDPROC previous = reinterpret_cast<WNDPROC>(
            SetWindowLongA(hwnd, GWL_WNDPROC, reinterpret_cast<LONG>(MonitorSizedWndProc)));
        if (previous) {
            original_wndproc = previous;
        }
    }

    RECT monitor_rect;
    if (!GetWindowMonitorRect(hwnd, &monitor_rect)) {
        return;
    }

    ClipCursor(nullptr);

    if (!SetWindowPos(
        hwnd,
        HWND_NOTOPMOST,
        monitor_rect.left,
        monitor_rect.top,
        monitor_rect.right - monitor_rect.left,
        monitor_rect.bottom - monitor_rect.top,
        SWP_FRAMECHANGED |
        SWP_SHOWWINDOW |
        SWP_NOOWNERZORDER
    )) {
        return;
    }

    RECT client_rect;
    if (GetClientRect(hwnd, &client_rect)) {
        client_width = client_rect.right - client_rect.left;
        client_height = client_rect.bottom - client_rect.top;
    }

    SetActiveWindow(hwnd);
    SetFocus(hwnd);
    SetForegroundWindow(hwnd);

    if (GetForegroundWindow() == hwnd) {
        ClipCursor(&monitor_rect);
    }
}

DWORD plat_window_screen_detour_jmp_back = 0;
void __declspec(naked) PlatWindowSizeToScreenDetour() {
    __asm {
        pushfd;
        pushad;
        call ResizeWindowToMonitor;
        popad;
        popfd;

        // MoveWindow is __stdcall and would have removed its six arguments.
        add esp, 0x18;
        jmp[plat_window_screen_detour_jmp_back];
    }
}

DWORD render_width = 0;
DWORD render_height = 0;

// repoint a relocated absolute operand that references Platform+expected_rva to new_target
void PatchPlatformOperand(DWORD operand_rva, DWORD expected_rva, DWORD* new_target) {
    DWORD* operand = reinterpret_cast<DWORD*>(reinterpret_cast<BYTE*>(plat) + operand_rva);
    if (*operand != reinterpret_cast<DWORD>(plat) + expected_rva) {
        return;
    }
    DWORD value = reinterpret_cast<DWORD>(new_target);
    MemPatch(reinterpret_cast<BYTE*>(operand), reinterpret_cast<BYTE*>(&value), 4);
}

// when the window is already monitor-sized no WM_SIZE arrives, so Platform would keep the
// previous render size; mirror its WM_SIZE handler (Platform+0xB2A6..0xB2BF) manually
void __stdcall SyncPlatformRenderSize() {
    if (!render_width || !render_height) {
        return;
    }
    BYTE* p = reinterpret_cast<BYTE*>(plat);
    DWORD* width = reinterpret_cast<DWORD*>(p + 0x1581C);
    DWORD* height = reinterpret_cast<DWORD*>(p + 0x15820);
    if (*width == render_width && *height == render_height) {
        return;
    }
    *width = render_width;
    *height = render_height;
    if (*(p + 0x141F5)) {
        void* renderer = *reinterpret_cast<void**>(p + 0x15704);
        if (renderer) {
            typedef void(__thiscall* SizeChangedFn)(void*);
            reinterpret_cast<SizeChangedFn>((*reinterpret_cast<void***>(renderer))[0x4C / 4])(renderer);
        }
    }
}

DWORD plat_window_fullscreen_detour_jmp_back = 0;
void __declspec(naked) PlatFullscreenSizeToScreenDetour() {
    __asm {
        // esi is still pushed: [esp+8] = width, [esp+0xC] = height
        push eax;
        mov eax, dword ptr[esp + 0xC];
        mov render_width, eax;
        mov eax, dword ptr[esp + 0x10];
        mov render_height, eax;
        mov eax, plat_window_fullscreen_flag;
        mov dword ptr[eax], 1;
        pop eax;

        pushfd;
        pushad;
        call ResizeWindowToMonitor;
        call SyncPlatformRenderSize;
        popad;
        popfd;
        jmp[plat_window_fullscreen_detour_jmp_back];
    }
}

// SetWindowSize only clears the fullscreen flag at its end; clear it up front so
// the windowed resize isn't treated as borderless by our hooks
DWORD plat_set_window_size_jmp_back = 0;
DWORD* plat_window_style_source = nullptr;
void __stdcall LeaveBorderless() {
    if (plat_window_fullscreen_flag && *plat_window_fullscreen_flag) {
        ClipCursor(nullptr);
    }
    *plat_window_fullscreen_flag = 0;
}

void __declspec(naked) PlatSetWindowSizeEntryDetour() {
    __asm {
        pushad;
        call LeaveBorderless;
        popad;
        // replaced: sub esp, 10h; mov eax, [Platform+0x15834]
        sub esp, 0x10;
        mov eax, plat_window_style_source;
        mov eax, dword ptr[eax];
        jmp[plat_set_window_size_jmp_back];
    }
}

// Platform's WM_SIZE handler stores the client size as the game's width/height;
// report the render resolution instead so the UI lays out for the back buffer
DWORD plat_wm_size_jmp_back = 0;
void __declspec(naked) PlatWmSizeDetour() {
    __asm {
        pushfd;
        mov ecx, dword ptr[esp + 0x18 + 4];
        mov edx, dword ptr[esp + 0x1C + 4];
        mov client_width, ecx;
        mov client_height, edx;
        cmp render_width, 0;
        je done;
        cmp render_height, 0;
        je done;
        push eax;
        mov eax, plat_window_fullscreen_flag;
        cmp dword ptr[eax], 0;
        pop eax;
        je done;
        mov ecx, render_width;
        mov edx, render_height;
    done:
        popfd;
        jmp[plat_wm_size_jmp_back];
    }
}

DWORD plat_window_setWindowSize_orig = 0;
DWORD plat_window_fullscreenSize_orig = 0;
DWORD plat_window_setVisible_orig = 0;

bool __stdcall IsBorderless() {
    return plat_window_fullscreen_flag && *plat_window_fullscreen_flag;
}

// Spooge+0x1A212: client-size sync into the device mode; keep it for windowed mode only
DWORD spooge_client_sync_jmp_back = 0;
void __declspec(naked) SpoogeClientSyncDetour() {
    __asm {
        push eax;
        push ecx;
        call IsBorderless;
        test al, al;
        pop ecx;
        pop eax;
        jnz skip;
        mov dword ptr[esi + 0x2C4], eax;
        mov dword ptr[esi + 0x2C8], ecx;
    skip:
        jmp[spooge_client_sync_jmp_back];
    }
}

// replaces Spooge's window resize to the back buffer size (Spooge+0x1D1B0, thiscall, no args)
DWORD spooge_resize_window_jmp_back = 0;
void __declspec(naked) SpoogeResizeWindowDetour() {
    __asm {
        push ecx;
        call IsBorderless;
        pop ecx;
        test al, al;
        jnz borderless;
        // replaced: sub esp, 10h; push esi; mov esi, ecx
        sub esp, 0x10;
        push esi;
        mov esi, ecx;
        jmp[spooge_resize_window_jmp_back];
    borderless:
        mov eax, dword ptr[ecx + 0x2C4];
        mov render_width, eax;
        mov eax, dword ptr[ecx + 0x2C8];
        mov render_height, eax;

        mov byte ptr[ecx + 0x1B74], 1;
        pushfd;
        pushad;
        call ResizeWindowToMonitor;
        popad;
        popfd;
        mov byte ptr[ecx + 0x1B74], 0;
        ret;
    }
}
BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        base = (DWORD)GetModuleHandleA("DOW2.exe");

        plat = GetModuleHandleA("Platform.dll");
        spooge = GetModuleHandleA("Spooge.dll");
        if (plat) {
            hwnd_global = reinterpret_cast<HWND*>(reinterpret_cast<BYTE*>(plat) + 0x15810);
            plat_window_setup_orig = (DWORD)GetProcAddress(plat, MAKEINTRESOURCEA(184));
            plat_window_setWindowSize_orig = (DWORD)GetProcAddress(plat, MAKEINTRESOURCEA(180));
            plat_window_fullscreenSize_orig = (DWORD)GetProcAddress(plat, MAKEINTRESOURCEA(173));
            plat_window_setVisible_orig = (DWORD)GetProcAddress(plat, MAKEINTRESOURCEA(179));
            plat_window_fullscreen_flag = reinterpret_cast<DWORD*>(reinterpret_cast<BYTE*>(plat) + 0x141BC);
            jmpback = plat_window_setWindowSize_orig + 0x1D;
            //JmpPatch(reinterpret_cast<BYTE*>(plat_window_setWindowSize_orig + 0x13), (DWORD)PlatWindowModeSet, 10);
            plat_window_screen_detour_jmp_back = plat_window_setWindowSize_orig + 0xD7;
            //JmpPatch(reinterpret_cast<BYTE*>(plat_window_setWindowSize_orig + 0xD1), (DWORD)PlatWindowSizeToScreenDetour, 6);
            // mem patches to make fullscreen window borderless
            BYTE fullscreen_patch1[4] = { 0x00, 0x00, 0x04, 0x00 };
            MemPatch(reinterpret_cast<BYTE*>(plat_window_fullscreenSize_orig + 0x24), fullscreen_patch1, 4);
            BYTE fullscreen_patch2[2] = { 0x6A, 0xFE };
            MemPatch(reinterpret_cast<BYTE*>(plat_window_fullscreenSize_orig + 0x55), fullscreen_patch2, 2);

            BYTE visible_not_topmost_patch[2] = { 0x6A, 0xFE };
            MemPatch(reinterpret_cast<BYTE*>(plat_window_setVisible_orig + 0x5A), visible_not_topmost_patch, 2);

            plat_window_fullscreen_detour_jmp_back = plat_window_fullscreenSize_orig + 0x9A;
            JmpPatch(
                reinterpret_cast<BYTE*>(plat_window_fullscreenSize_orig + 0x90),
                (DWORD)PlatFullscreenSizeToScreenDetour,
                10);

            plat_window_style_source = reinterpret_cast<DWORD*>(reinterpret_cast<BYTE*>(plat) + 0x15834);
            plat_set_window_size_jmp_back = plat_window_setWindowSize_orig + 8;
            JmpPatch(reinterpret_cast<BYTE*>(plat_window_setWindowSize_orig), (DWORD)PlatSetWindowSizeEntryDetour, 8);

            plat_wm_size_jmp_back = reinterpret_cast<DWORD>(plat) + 0xB2A6;
            JmpPatch(reinterpret_cast<BYTE*>(plat) + 0xB29E, (DWORD)PlatWmSizeDetour, 8);

            client_width = *reinterpret_cast<DWORD*>(reinterpret_cast<BYTE*>(plat) + 0x1581C);
            client_height = *reinterpret_cast<DWORD*>(reinterpret_cast<BYTE*>(plat) + 0x15820);
            // mouse/cursor conversions must use the real client size, not the render size
            const DWORD width_operands[] = { 0x32DA, 0x3A11, 0x3A17, 0x3DEE, 0x4016, 0x43AF, 0x43B5, 0x4422, 0x7171 };
            const DWORD height_operands[] = { 0x330C, 0x3A2B, 0x3A52, 0x3E0C, 0x4031, 0x43CE, 0x43D9, 0x443B, 0x719B };
            for (DWORD rva : width_operands) {
                PatchPlatformOperand(rva, 0x1581C, &client_width);
            }
            for (DWORD rva : height_operands) {
                PatchPlatformOperand(rva, 0x15820, &client_height);
            }
        }
        if (spooge) {
            // patches for spooge preventing resolution change of monitor
            BYTE windowed_format_patch[4] = { 0x6A, 0x01, 0x58, 0x90 };
            MemPatch(reinterpret_cast<BYTE*>(spooge) + 0x1D249, windowed_format_patch, 4);

            BYTE windowed_present_patch[6] = { 0xBA, 0x01, 0x00, 0x00, 0x00, 0x90 };
            MemPatch(reinterpret_cast<BYTE*>(spooge) + 0x1E7D9, windowed_present_patch, 6);

            // borderless: keep the requested back buffer size instead of adopting the monitor-sized client rect
            spooge_client_sync_jmp_back = reinterpret_cast<DWORD>(spooge) + 0x1A21E;
            JmpPatch(reinterpret_cast<BYTE*>(spooge) + 0x1A212, (DWORD)SpoogeClientSyncDetour, 12);

            spooge_resize_window_jmp_back = reinterpret_cast<DWORD>(spooge) + 0x1D1B6;
            JmpPatch(reinterpret_cast<BYTE*>(spooge) + 0x1D1B0, (DWORD)SpoogeResizeWindowDetour, 6);
        }
    }
    return TRUE;
}
