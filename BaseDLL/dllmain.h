#pragma once
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define SUBHOOK_STATIC
#define SUBHOOK_FORCE_32BIT 
// Windows Header Files
//#include <MinHook.h>
#include <windows.h>
#include <string>
#include <detours.h>