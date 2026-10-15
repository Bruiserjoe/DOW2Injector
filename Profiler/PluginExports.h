#pragma once
#include <Windows.h>
#include <cstddef>

#ifdef PROFILER_EXPORTS
#define PLUGIN_API __declspec(dllexport)
#else
#define PLUGIN_API __declspec(dllimport)
#endif

struct SigScanResult {
	bool found;
	DWORD address;
};

extern "C" {
	PLUGIN_API bool MemPatch(BYTE* dst, const BYTE* src, size_t size);
	PLUGIN_API SigScanResult SigScan(BYTE* start, size_t length, const char* pattern, const char* mask);
	PLUGIN_API bool NopPatch(BYTE* dst, size_t size);
	PLUGIN_API bool JmpPatch(BYTE* dst, DWORD target, size_t size);
}