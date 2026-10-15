#include "PluginExports.h"

#include <cstdint>
#include <cstring>
#include <limits>
#include <Psapi.h>

namespace {

	bool IsLoadedModuleRange(const BYTE* dst, size_t size) {
		if (!dst || size == 0) {
			return false;
		}

		const uintptr_t start = reinterpret_cast<uintptr_t>(dst);
		constexpr uintptr_t maxAddress = (std::numeric_limits<uintptr_t>::max)();
		if (size > maxAddress - start) {
			return false;
		}
		const uintptr_t end = start + size;

		MEMORY_BASIC_INFORMATION firstRegion{};
		if (VirtualQuery(dst, &firstRegion, sizeof(firstRegion)) != sizeof(firstRegion)
			|| firstRegion.State != MEM_COMMIT
			|| firstRegion.Type != MEM_IMAGE
			|| !firstRegion.AllocationBase) {
			return false;
		}

		MODULEINFO moduleInfo{};
		HMODULE module = static_cast<HMODULE>(firstRegion.AllocationBase);
		if (!K32GetModuleInformation(GetCurrentProcess(), module, &moduleInfo, sizeof(moduleInfo))) {
			return false;
		}

		const uintptr_t moduleStart = reinterpret_cast<uintptr_t>(moduleInfo.lpBaseOfDll);
		if (moduleInfo.SizeOfImage > maxAddress - moduleStart) {
			return false;
		}
		const uintptr_t moduleEnd = moduleStart + moduleInfo.SizeOfImage;
		if (start < moduleStart || end > moduleEnd) {
			return false;
		}

		uintptr_t cursor = start;
		while (cursor < end) {
			MEMORY_BASIC_INFORMATION region{};
			if (VirtualQuery(reinterpret_cast<const void*>(cursor), &region, sizeof(region)) != sizeof(region)
				|| region.State != MEM_COMMIT
				|| region.Type != MEM_IMAGE
				|| region.AllocationBase != module) {
				return false;
			}

			const uintptr_t regionStart = reinterpret_cast<uintptr_t>(region.BaseAddress);
			if (region.RegionSize > maxAddress - regionStart) {
				return false;
			}
			const uintptr_t regionEnd = regionStart + region.RegionSize;
			if (regionEnd <= cursor) {
				return false;
			}
			cursor = regionEnd;
		}

		return true;
	}

	bool FinishPatch(BYTE* dst, size_t size, DWORD oldProtection) {
		FlushInstructionCache(GetCurrentProcess(), dst, size);
		DWORD ignored;
		return VirtualProtect(dst, size, oldProtection, &ignored) != FALSE;
	}
}

bool MemPatch(BYTE* dst, const BYTE* src, size_t size) {
	if (!src || !IsLoadedModuleRange(dst, size)) {
		return false;
	}

	DWORD prot;
	if (!VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &prot)) {
		return false;
	}
	std::memcpy(dst, src, size);
	return FinishPatch(dst, size, prot);
}

bool NopPatch(BYTE* dst, size_t size) {
	if (!IsLoadedModuleRange(dst, size)) {
		return false;
	}

	DWORD prot;
	if (!VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &prot)) {
		return false;
	}
	std::memset(dst, 0x90, size);
	return FinishPatch(dst, size, prot);
}

bool JmpPatch(BYTE* dst, DWORD target, size_t size) {
	if (size < 5 || !IsLoadedModuleRange(dst, size)) {
		return false;
	}

	DWORD prot;
	if (!VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &prot)) {
		return false;
	}
	std::memset(dst, 0x90, size);
	DWORD relativeaddr = (target - reinterpret_cast<DWORD>(dst)) - 5;

	*dst = 0xE9;
	*reinterpret_cast<DWORD*>(reinterpret_cast<DWORD>(dst) + 1) = relativeaddr;
	return FinishPatch(dst, size, prot);
}

SigScanResult SigScan(BYTE* start, size_t length, const char* pattern, const char* mask) {
	const size_t patternLength = std::strlen(mask);
	if (patternLength > length) {
		return { false, 0 };
	}

	for (size_t i = 0; i <= length - patternLength; i++) {
		bool found = true;
		for (size_t j = 0; j < patternLength; j++) {
			if (mask[j] == 'x' && start[i + j] != static_cast<BYTE>(pattern[j])) {
				found = false;
				break;
			}
		}
		if (found) {
			return { true, reinterpret_cast<DWORD>(start) + i };
		}
	}
	return { false, 0 };
}
