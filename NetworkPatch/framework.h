#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <detours.h>  // Detours include
#include <chrono>
#include <iomanip>
#include "..\Profiler\PluginExports.h"

void* getCallerAddress() {
	void* stack[10];
	uint16_t frames = CaptureStackBackTrace(0, 10, stack, nullptr);
	if (frames >= 2) {
		return stack[2];
	}
	return nullptr;
}

void* getCallerAddressNextUp() {
	void* stack[10];
	uint16_t frames = CaptureStackBackTrace(0, 10, stack, nullptr);
	if (frames >= 3) {
		return stack[3];
	}
	return nullptr;
}

std::string PointerToHexString(void* ptr) {
	std::ostringstream oss;
	oss << "0x" << std::hex << reinterpret_cast<uintptr_t>(ptr);
	return oss.str();
}

bool createFolder(std::string folderName) {
	if (CreateDirectoryA(folderName.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS) {
		return true;
	}
	else {
		return false;
	}
}

std::string GetCurrentTimeString() {
	const auto tp_utc =  std::chrono::system_clock::now();
	const time_t tim = std::chrono::system_clock::to_time_t(tp_utc);
	std::tm local_tm;
	localtime_s(&local_tm, &tim);
	std::ostringstream oss;
	oss << std::put_time(&local_tm, "%Y_%m_%d%__H_%M_%S");  // Change format as needed
	return oss.str();
}