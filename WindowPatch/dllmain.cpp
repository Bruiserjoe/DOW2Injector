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
#ifdef DOW2_SIGSCAN
DWORD* plat_render_width_global = nullptr;
DWORD* plat_render_height_global = nullptr;
BYTE* plat_renderer_active_flag = nullptr;
void** plat_renderer_global = nullptr;
#endif

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
#ifdef DOW2_SIGSCAN
    DWORD* width = plat_render_width_global;
    DWORD* height = plat_render_height_global;
    if (!width || !height || !plat_renderer_active_flag || !plat_renderer_global) {
        return;
    }
#else
    BYTE* p = reinterpret_cast<BYTE*>(plat);
    DWORD* width = reinterpret_cast<DWORD*>(p + 0x1581C);
    DWORD* height = reinterpret_cast<DWORD*>(p + 0x15820);
#endif
    if (*width == render_width && *height == render_height) {
        return;
    }
    *width = render_width;
    *height = render_height;
#ifdef DOW2_SIGSCAN
    if (*plat_renderer_active_flag) {
        void* renderer = *plat_renderer_global;
#else
    if (*(p + 0x141F5)) {
        void* renderer = *reinterpret_cast<void**>(p + 0x15704);
#endif
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

#ifdef DOW2_SIGSCAN
namespace {
    size_t GetModuleImageSize(HMODULE module) {
        if (!module) {
            return 0;
        }
        const IMAGE_DOS_HEADER* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(module);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
            return 0;
        }
        const IMAGE_NT_HEADERS* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(
            reinterpret_cast<const BYTE*>(module) + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) {
            return 0;
        }
        return nt->OptionalHeader.SizeOfImage;
    }

    bool FailPatch(const char* reason) {
        std::string message = "[WindowPatch] SigScan initialization failed: ";
        message += reason;
        message += "\n";
        OutputDebugStringA(message.c_str());
        return false;
    }

    bool FindUnique(
        HMODULE module,
        size_t module_size,
        const char* name,
        const char* pattern,
        const char* mask,
        DWORD* address) {
        SigScanResult result = SigScan(reinterpret_cast<BYTE*>(module), module_size, pattern, mask);
        if (!result.found) {
            return FailPatch(name);
        }

        BYTE* module_start = reinterpret_cast<BYTE*>(module);
        BYTE* next = reinterpret_cast<BYTE*>(result.address) + 1;
        size_t remaining = module_size - static_cast<size_t>(next - module_start);
        if (SigScan(next, remaining, pattern, mask).found) {
            std::string duplicate = name;
            duplicate += " matched more than once";
            return FailPatch(duplicate.c_str());
        }

        *address = result.address;
        return true;
    }

    bool HasBytes(DWORD address, const BYTE* expected, size_t size) {
        const BYTE* actual = reinterpret_cast<const BYTE*>(address);
        for (size_t i = 0; i < size; ++i) {
            if (actual[i] != expected[i]) {
                return false;
            }
        }
        return true;
    }

    bool IsModuleAddress(HMODULE module, size_t module_size, const void* address, size_t size) {
        DWORD start = reinterpret_cast<DWORD>(module);
        DWORD value = reinterpret_cast<DWORD>(address);
        return value >= start && size <= module_size && value - start <= module_size - size;
    }

    bool ApplyMemPatch(DWORD address, const BYTE* value, size_t size, const char* name) {
        if (!MemPatch(reinterpret_cast<BYTE*>(address), value, size)) {
            return FailPatch(name);
        }
        return true;
    }

    bool ApplyJmpPatch(DWORD address, DWORD target, size_t size, const char* name) {
        if (!JmpPatch(reinterpret_cast<BYTE*>(address), target, size)) {
            return FailPatch(name);
        }
        return true;
    }

    bool InitSigScanPatch() {
        base = reinterpret_cast<DWORD>(GetModuleHandleA("DOW2.exe"));
        plat = GetModuleHandleA("Platform.dll");
        spooge = GetModuleHandleA("Spooge.dll");
        if (!base || !plat || !spooge) {
            return FailPatch("required game modules are not loaded");
        }

        size_t plat_size = GetModuleImageSize(plat);
        size_t spooge_size = GetModuleImageSize(spooge);
        if (!plat_size || !spooge_size) {
            return FailPatch("invalid module image");
        }

        DWORD plat_wm_anchor = 0;
        DWORD plat_set_window_anchor = 0;
        DWORD plat_fullscreen_style_anchor = 0;
        DWORD plat_fullscreen_insert_anchor = 0;
        DWORD plat_fullscreen_hook_anchor = 0;
        DWORD plat_visible_anchor = 0;
        DWORD operand_anchors[6]{};
        DWORD spooge_anchors[4]{};

        if (!FindUnique(plat, plat_size, "Platform WM_SIZE",
            "\x57\xFF\x15\x00\x00\x00\x00\x80\x3D\x00\x00\x00\x00\x00\x8B\x4C\x24\x18\x8B\x54\x24\x1C\x89\x0D\x00\x00\x00\x00\x89\x15\x00\x00\x00\x00\x74\x00\x8B\x0D\x00\x00\x00\x00\x8B\x01\x8B\x50\x4C\xFF\xD2",
            "xxx????xx????xxxxxxxxxxx????xx????x?xx????xxxxxxx", &plat_wm_anchor)
            || !FindUnique(plat, plat_size, "Platform SetWindowSize",
                "\x83\xEC\x10\xA1\x00\x00\x00\x00\xF7\xD8\x1B\xC0\x56\x8B\x35\x00\x00\x00\x00\x25\x00\x00\x38\x2F\x05\x00\x00\xC8\x10",
                "xxxx????xxxxxxx????xxxxxxxxxx", &plat_set_window_anchor)
            || !FindUnique(plat, plat_size, "Platform fullscreen style",
                "\x68\x00\x00\x00\x90\x6A\xF0\x50\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x90\xC7\x05\x00\x00\x00\x00\x08\x00\x00\x00\xFF\xD6",
                "xxxxxxxxxx????xxxxxx????xxxxxx", &plat_fullscreen_style_anchor)
            || !FindUnique(plat, plat_size, "Platform fullscreen insert-after",
                "\x8B\x15\x00\x00\x00\x00\x68\x08\x01\x00\x00\x50\x51\x6A\x00\x6A\x00\x6A\xFF\x52\xFF\x15\x00\x00\x00\x00",
                "xx????xxxxxxxxxxxxxxxx????", &plat_fullscreen_insert_anchor)
            || !FindUnique(plat, plat_size, "Platform fullscreen hook",
                "\xA1\x00\x00\x00\x00\x50\xFF\x15\x00\x00\x00\x00\xC7\x05\x00\x00\x00\x00\x01\x00\x00\x00\x5E\xC2\x08\x00",
                "x????xxx????xx????xxxxxxxx", &plat_fullscreen_hook_anchor)
            || !FindUnique(plat, plat_size, "Platform SetVisible",
                "\x68\x0B\x04\x00\x00\x6A\x00\x6A\x00\x6A\x00\x6A\x00\x6A\xFF\x50\xFF\xD6\x8B\x0D\x00\x00\x00\x00",
                "xxxxxxxxxxxxxxxxxxxx????", &plat_visible_anchor)
            || !FindUnique(plat, plat_size, "Platform operand group 1",
                "\x8B\x45\x14\x8B\x0D\x00\x00\x00\x00\x0F\xBF\xD0\xC1\xE8\x10\x0F\xBF\xC0\x83\xC1\xFF",
                "xxxxx????xxxxxxxxxxxx", &operand_anchors[0])
            || !FindUnique(plat, plat_size, "Platform operand group 2",
                "\xA1\x00\x00\x00\x00\xDB\x05\x00\x00\x00\x00\x83\xEC\x40\x85\xC0\x53",
                "x????xx????xxxxxx", &operand_anchors[1])
            || !FindUnique(plat, plat_size, "Platform operand group 3",
                "\xA1\x00\x00\x00\x00\x8D\x48\xFF\x85\xC9\x89\x4C\x24\x04\xDB\x44\x24\x04",
                "x????xxxxxxxxxxxxx", &operand_anchors[2])
            || !FindUnique(plat, plat_size, "Platform operand group 4",
                "\xA1\x00\x00\x00\x00\x83\xC0\xFF\x85\xC0\x89\x44\x24\x04\xDB\x44\x24\x04",
                "x????xxxxxxxxxxxxx", &operand_anchors[3])
            || !FindUnique(plat, plat_size, "Platform operand group 5",
                "\x8B\x15\x00\x00\x00\x00\xDB\x05\x00\x00\x00\x00\x85\xD2\x7D\x00\xD8\x05\x00\x00\x00\x00",
                "xx????xx????xxx?xx????", &operand_anchors[4])
            || !FindUnique(plat, plat_size, "Platform operand group 6",
                "\xA1\x00\x00\x00\x00\x85\xC0\x8B\x3D\x00\x00\x00\x00\x7E\x00\x6A\x24",
                "x????xxxx????x?xx", &operand_anchors[5])
            || !FindUnique(spooge, spooge_size, "Spooge windowed format",
                "\x8B\x6C\x24\x1C\x0F\xB6\x45\x10\x56\x83\xE0\x01\x57\x8B\xF1",
                "xxxxxxxxxxxxxxx", &spooge_anchors[0])
            || !FindUnique(spooge, spooge_size, "Spooge windowed present",
                "\x8B\x8F\x60\x1B\x00\x00\x89\x4E\x1C\x0F\xB6\x55\x10\x23\xD0\x33\xC9\x89\x56\x20",
                "xxxxxxxxxxxxxxxxxxxx", &spooge_anchors[1])
            || !FindUnique(spooge, spooge_size, "Spooge client sync",
                "\x74\x00\x85\xC9\x74\x00\x89\x86\xC4\x02\x00\x00\x89\x8E\xC8\x02\x00\x00\x5F\x5E\x5D",
                "x?xxx?xxxxxxxxxxxxxxx", &spooge_anchors[2])
            || !FindUnique(spooge, spooge_size, "Spooge resize window",
                "\x83\xEC\x10\x56\x8B\xF1\x8B\x86\xC8\x02\x00\x00\x8B\x8E\xC4\x02\x00\x00",
                "xxxxxxxxxxxxxxxxxx", &spooge_anchors[3])) {
            return false;
        }

        plat_window_setup_orig = reinterpret_cast<DWORD>(GetProcAddress(plat, MAKEINTRESOURCEA(184)));
        plat_window_setWindowSize_orig = reinterpret_cast<DWORD>(GetProcAddress(plat, MAKEINTRESOURCEA(180)));
        plat_window_fullscreenSize_orig = reinterpret_cast<DWORD>(GetProcAddress(plat, MAKEINTRESOURCEA(173)));
        plat_window_setVisible_orig = reinterpret_cast<DWORD>(GetProcAddress(plat, MAKEINTRESOURCEA(179)));
        if (!plat_window_setup_orig || !plat_window_setWindowSize_orig
            || !plat_window_fullscreenSize_orig || !plat_window_setVisible_orig) {
            return FailPatch("Platform window exports are unavailable");
        }

        DWORD fullscreen_style_patch = plat_fullscreen_style_anchor + 0x18;
        DWORD fullscreen_insert_patch = plat_fullscreen_insert_anchor + 0x11;
        DWORD fullscreen_hook = plat_fullscreen_hook_anchor + 0x0C;
        DWORD visible_insert_patch = plat_visible_anchor + 0x0D;
        DWORD set_window_hook = plat_set_window_anchor;
        DWORD wm_size_hook = plat_wm_anchor + 0x0E;
        DWORD spooge_format_patch = spooge_anchors[0] + 0x04;
        DWORD spooge_present_patch = spooge_anchors[1] + 0x09;
        DWORD spooge_client_hook = spooge_anchors[2] + 0x06;
        DWORD spooge_resize_hook = spooge_anchors[3];

        hwnd_global = reinterpret_cast<HWND*>(
            *reinterpret_cast<DWORD*>(plat_fullscreen_insert_anchor + 0x02));
        plat_window_fullscreen_flag = reinterpret_cast<DWORD*>(
            *reinterpret_cast<DWORD*>(plat_fullscreen_hook_anchor + 0x0E));
        plat_window_style_source = reinterpret_cast<DWORD*>(
            *reinterpret_cast<DWORD*>(plat_set_window_anchor + 0x04));
        plat_renderer_active_flag = reinterpret_cast<BYTE*>(
            *reinterpret_cast<DWORD*>(plat_wm_anchor + 0x09));
        plat_render_width_global = reinterpret_cast<DWORD*>(
            *reinterpret_cast<DWORD*>(plat_wm_anchor + 0x18));
        plat_render_height_global = reinterpret_cast<DWORD*>(
            *reinterpret_cast<DWORD*>(plat_wm_anchor + 0x1E));
        plat_renderer_global = reinterpret_cast<void**>(
            *reinterpret_cast<DWORD*>(plat_wm_anchor + 0x26));

        if (!IsModuleAddress(plat, plat_size, hwnd_global, sizeof(HWND))
            || !IsModuleAddress(plat, plat_size, plat_window_fullscreen_flag, sizeof(DWORD))
            || !IsModuleAddress(plat, plat_size, plat_window_style_source, sizeof(DWORD))
            || !IsModuleAddress(plat, plat_size, plat_renderer_active_flag, sizeof(BYTE))
            || !IsModuleAddress(plat, plat_size, plat_render_width_global, sizeof(DWORD))
            || !IsModuleAddress(plat, plat_size, plat_render_height_global, sizeof(DWORD))
            || !IsModuleAddress(plat, plat_size, plat_renderer_global, sizeof(void*))) {
            return FailPatch("derived Platform globals are outside Platform.dll");
        }

        DWORD* width_operands[] = {
            reinterpret_cast<DWORD*>(operand_anchors[0] + 0x05),
            reinterpret_cast<DWORD*>(operand_anchors[1] + 0x01),
            reinterpret_cast<DWORD*>(operand_anchors[1] + 0x07),
            reinterpret_cast<DWORD*>(operand_anchors[2] + 0x01),
            reinterpret_cast<DWORD*>(operand_anchors[3] + 0x01),
            reinterpret_cast<DWORD*>(operand_anchors[4] + 0x02),
            reinterpret_cast<DWORD*>(operand_anchors[4] + 0x08),
            reinterpret_cast<DWORD*>(operand_anchors[4] + 0x75),
            reinterpret_cast<DWORD*>(operand_anchors[5] + 0x01)
        };
        DWORD* height_operands[] = {
            reinterpret_cast<DWORD*>(operand_anchors[0] + 0x37),
            reinterpret_cast<DWORD*>(operand_anchors[1] + 0x1B),
            reinterpret_cast<DWORD*>(operand_anchors[1] + 0x42),
            reinterpret_cast<DWORD*>(operand_anchors[2] + 0x1F),
            reinterpret_cast<DWORD*>(operand_anchors[3] + 0x1C),
            reinterpret_cast<DWORD*>(operand_anchors[4] + 0x21),
            reinterpret_cast<DWORD*>(operand_anchors[4] + 0x2C),
            reinterpret_cast<DWORD*>(operand_anchors[4] + 0x8E),
            reinterpret_cast<DWORD*>(operand_anchors[5] + 0x2B)
        };

        for (DWORD* operand : width_operands) {
            if (!IsModuleAddress(plat, plat_size, operand, sizeof(DWORD))
                || *operand != reinterpret_cast<DWORD>(plat_render_width_global)) {
                return FailPatch("derived Platform width operand did not validate");
            }
        }
        for (DWORD* operand : height_operands) {
            if (!IsModuleAddress(plat, plat_size, operand, sizeof(DWORD))
                || *operand != reinterpret_cast<DWORD>(plat_render_height_global)) {
                return FailPatch("derived Platform height operand did not validate");
            }
        }

        const BYTE fullscreen_style_original[] = { 0x08, 0x00, 0x00, 0x00 };
        const BYTE insert_original[] = { 0x6A, 0xFF };
        const BYTE fullscreen_hook_opcode[] = { 0xC7, 0x05 };
        const BYTE fullscreen_hook_value[] = { 0x01, 0x00, 0x00, 0x00 };
        const BYTE set_window_opcode[] = { 0x83, 0xEC, 0x10, 0xA1 };
        const BYTE wm_size_original[] = { 0x8B, 0x4C, 0x24, 0x18, 0x8B, 0x54, 0x24, 0x1C };
        const BYTE spooge_format_original[] = { 0x0F, 0xB6, 0x45, 0x10 };
        const BYTE spooge_present_original[] = { 0x0F, 0xB6, 0x55, 0x10, 0x23, 0xD0 };
        const BYTE spooge_client_original[] = { 0x89, 0x86, 0xC4, 0x02, 0x00, 0x00, 0x89, 0x8E, 0xC8, 0x02, 0x00, 0x00 };
        const BYTE spooge_resize_original[] = { 0x83, 0xEC, 0x10, 0x56, 0x8B, 0xF1 };

        if (!HasBytes(fullscreen_style_patch, fullscreen_style_original, sizeof(fullscreen_style_original))
            || !HasBytes(fullscreen_insert_patch, insert_original, sizeof(insert_original))
            || !HasBytes(visible_insert_patch, insert_original, sizeof(insert_original))
            || !HasBytes(fullscreen_hook, fullscreen_hook_opcode, sizeof(fullscreen_hook_opcode))
            || *reinterpret_cast<DWORD*>(fullscreen_hook + 0x02) != reinterpret_cast<DWORD>(plat_window_fullscreen_flag)
            || !HasBytes(fullscreen_hook + 0x06, fullscreen_hook_value, sizeof(fullscreen_hook_value))
            || !HasBytes(set_window_hook, set_window_opcode, sizeof(set_window_opcode))
            || *reinterpret_cast<DWORD*>(set_window_hook + 0x04) != reinterpret_cast<DWORD>(plat_window_style_source)
            || !HasBytes(wm_size_hook, wm_size_original, sizeof(wm_size_original))
            || !HasBytes(spooge_format_patch, spooge_format_original, sizeof(spooge_format_original))
            || !HasBytes(spooge_present_patch, spooge_present_original, sizeof(spooge_present_original))
            || !HasBytes(spooge_client_hook, spooge_client_original, sizeof(spooge_client_original))
            || !HasBytes(spooge_resize_hook, spooge_resize_original, sizeof(spooge_resize_original))) {
            return FailPatch("derived patch bytes did not validate");
        }

        plat_window_fullscreen_detour_jmp_back = fullscreen_hook + 10;
        plat_set_window_size_jmp_back = set_window_hook + 8;
        plat_wm_size_jmp_back = wm_size_hook + 8;
        spooge_client_sync_jmp_back = spooge_client_hook + 12;
        spooge_resize_window_jmp_back = spooge_resize_hook + 6;
        client_width = *plat_render_width_global;
        client_height = *plat_render_height_global;

        BYTE fullscreen_patch1[4] = { 0x00, 0x00, 0x04, 0x00 };
        BYTE insert_patch[2] = { 0x6A, 0xFE };
        BYTE windowed_format_patch[4] = { 0x6A, 0x01, 0x58, 0x90 };
        BYTE windowed_present_patch[6] = { 0xBA, 0x01, 0x00, 0x00, 0x00, 0x90 };

        if (!ApplyMemPatch(fullscreen_style_patch, fullscreen_patch1, sizeof(fullscreen_patch1), "fullscreen style patch failed")
            || !ApplyMemPatch(fullscreen_insert_patch, insert_patch, sizeof(insert_patch), "fullscreen insert-after patch failed")
            || !ApplyMemPatch(visible_insert_patch, insert_patch, sizeof(insert_patch), "visible insert-after patch failed")
            || !ApplyJmpPatch(fullscreen_hook, reinterpret_cast<DWORD>(PlatFullscreenSizeToScreenDetour), 10, "fullscreen hook failed")
            || !ApplyJmpPatch(set_window_hook, reinterpret_cast<DWORD>(PlatSetWindowSizeEntryDetour), 8, "SetWindowSize hook failed")
            || !ApplyJmpPatch(wm_size_hook, reinterpret_cast<DWORD>(PlatWmSizeDetour), 8, "WM_SIZE hook failed")) {
            return false;
        }

        DWORD client_width_address = reinterpret_cast<DWORD>(&client_width);
        DWORD client_height_address = reinterpret_cast<DWORD>(&client_height);
        for (DWORD* operand : width_operands) {
            if (!MemPatch(reinterpret_cast<BYTE*>(operand),
                reinterpret_cast<const BYTE*>(&client_width_address), sizeof(client_width_address))) {
                return FailPatch("Platform width operand patch failed");
            }
        }
        for (DWORD* operand : height_operands) {
            if (!MemPatch(reinterpret_cast<BYTE*>(operand),
                reinterpret_cast<const BYTE*>(&client_height_address), sizeof(client_height_address))) {
                return FailPatch("Platform height operand patch failed");
            }
        }

        if (!ApplyMemPatch(spooge_format_patch, windowed_format_patch, sizeof(windowed_format_patch), "Spooge format patch failed")
            || !ApplyMemPatch(spooge_present_patch, windowed_present_patch, sizeof(windowed_present_patch), "Spooge present patch failed")
            || !ApplyJmpPatch(spooge_client_hook, reinterpret_cast<DWORD>(SpoogeClientSyncDetour), 12, "Spooge client hook failed")
            || !ApplyJmpPatch(spooge_resize_hook, reinterpret_cast<DWORD>(SpoogeResizeWindowDetour), 6, "Spooge resize hook failed")) {
            return false;
        }

        return true;
    }
}
#endif

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
#ifdef DOW2_SIGSCAN
        return InitSigScanPatch() ? TRUE : FALSE;
#else
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
#endif
    }
    return TRUE;
}
