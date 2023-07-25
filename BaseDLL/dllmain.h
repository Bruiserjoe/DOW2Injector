#pragma once
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <string>
#include <detours.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>

struct Mode {
	int ffa;
	int t_ffa;
};
typedef Mode* ModeP;

//map generated on startup being read from the config file 
class GamemodeMap {
private:
	std::vector<Mode> lookup;
	//dont hash anything above or equal to 100 or else we will have collisions. But whos gonna add enough for that? So should be fine
	size_t hash(size_t index) {
		return index % 100;
	}
	size_t getIndex(std::string line) {
		size_t i;
		std::string parse;
		for (i = 0; i < line.size() && line[i] != ':'; i++) {
			parse.push_back(line[i]);
		}
		i = std::stoi(parse);
		return i;
	}
	int getFFA(std::string line) {
		size_t pos = line.find("ffa:");
		pos += 5;
		std::string ans;
		for (pos; pos < line.size() && line[pos] != ';'; pos++) {
			if (line[pos] != ' ' && line[pos] != '\t') {
				ans.push_back(line[pos]);
			}
		}
		if (ans.compare("true") == 0) {
			return 1;
		}
		else {
			return 0;
		}
	}
	int getTFFA(std::string line) {
		size_t pos = line.find("tffa:");
		pos += 6;
		std::string ans;
		for (pos; pos < line.size() && line[pos] != ';'; pos++) {
			if (line[pos] != ' ' && line[pos] != '\t') {
				ans.push_back(line[pos]);
			}
		}
		if (ans.compare("true") == 0) {
			return 1;
		}
		else {
			return 0;
		}
	}

public:
	GamemodeMap() {
		for (size_t i = 0; i < 100; i++) {
			lookup.push_back({ 0, 0});
		}

	}
	void insert(size_t index, int ffa, int t_ffa) {
		if (index < lookup.size()) {
			lookup[index].ffa = ffa;
			lookup[index].t_ffa = t_ffa;
		}
	}
	Mode getMode(size_t index) {
		if (index < lookup.size()) {
			return lookup[index];
		}
		return {0, 0};
	}
	void readConfig(std::string path) {
		std::ifstream file;
		file.open(path);
		std::string line;
		while (getline(file, line)) {
			insert(getIndex(line), getFFA(line), getTFFA(line));
		}
		file.close();
	}


};
void MemPatch(BYTE* dst, BYTE* src, size_t size) {
	DWORD prot;
	VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &prot);
	std::memcpy(dst, src, size);
	VirtualProtect(dst, size, prot, &prot);
}

void NopPatch(BYTE* dst, size_t size) {
	DWORD prot;
	VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &prot);
	std::memset(dst, 0x90, size);
	VirtualProtect(dst, size, prot, &prot);
}

bool JmpPatch(BYTE* dst, DWORD target, size_t size) {
	if (size < 5) {
		return false;
	}
	DWORD prot;
	VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &prot);
	std::memset(dst, 0x90, size);
	DWORD relativeaddr = (target - (DWORD)dst) - 5;

	*(dst) = 0xE9;
	*(DWORD*)((DWORD)dst + 1) = relativeaddr;
	VirtualProtect(dst, size, prot, &prot);
	return true;
}


typedef void(__stdcall *LoadMaps)(char* path, void* param2);
LoadMaps ldmaps_org = nullptr;
typedef DWORD32* (__thiscall *MapDropdown)(void* tis, int param1);
MapDropdown mpdrp_org = nullptr;
typedef void(__stdcall *DropDownAdd)(int param1);
DropDownAdd drpadd_org = nullptr;

//we keep our own list of maps for each folder, so we can set the list to what ever we want
class MapLoader {
private:
	struct MapAddr {
		DWORD32 addr; //actual data addr
		size_t g_index; //game index
		std::string path; //file path
	};

	DWORD offset;
	DWORD campaign_maps;
	DWORD ffa_maps;
	DWORD pvp_maps;
	DWORD laststand_maps;
	std::vector<MapAddr> map_lists;
public:
	MapLoader() {
		offset = 0;
		campaign_maps = 0;
		ffa_maps = 0;
		pvp_maps = 0;
		laststand_maps = 0;
	}
	MapLoader(DWORD base) {
		offset = base + 0xf357a0;
		campaign_maps = offset + 0x0;
		ffa_maps = offset + 0x3c;
		pvp_maps = offset + 0xc;
		laststand_maps = offset + 0x30;
	}
	~MapLoader() {
		for (auto& i : map_lists) {
			std::free((void*)i.addr);
		}
	}
	size_t generateMapList(std::string file_path, size_t g_index) {
		void* dat = std::malloc(100); //no idea how big this has to be
		DWORD t = (DWORD)dat;
		ldmaps_org((char*)file_path.c_str(), dat);
		map_lists.push_back({t, g_index, file_path});
		return map_lists.size() - 1;
	}
	DWORD getMapList(std::string name) {

	}
	DWORD __cdecl getMapList(size_t game) {
		for (auto& i : map_lists) {
			if (i.g_index == game) {
				return i.addr;
			}
		}
		return pvp_maps;
	}


};
typedef int* (__stdcall *ResetInfo)(int* param1);
ResetInfo rinfo_org = nullptr;
typedef DWORD32(__fastcall *MapReadInfoFile)(DWORD32 *param1);
MapReadInfoFile mri_file = nullptr;
typedef void(__fastcall *SomethingMapInfo)(int* param1, void* info);
SomethingMapInfo smth_info = nullptr;
typedef void(__stdcall *SomethingPath)(char* param1);
SomethingPath smth_path = nullptr;

DWORD32 campaign_maps;

/*
void LoadMapFolder(std::string path, void** dat) {
	
	//loop through the folder and find all the files
	WIN32_FIND_DATA FileData;
	std::string files = path + "\\*";
	HANDLE hFind = FindFirstFile((LPCWSTR)files.c_str(), &FileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		//error("Folder is invalid", "Invalid folder passed");
		return;
	}
	std::vector<std::string> folder;
	do
	{
		std::string cur = "";
		for (int i = 0; i < 260 && FileData.cFileName[i] != '\0'; i++) {
			cur.push_back(FileData.cFileName[i]);
		}
		//if (cur.find(".dll") != std::string::npos) {
			//std::cout << cur << "\n";
			folder.push_back(cur);
		//}
	} while (FindNextFile(hFind, &FileData) != 0);
	FindClose(hFind);
	//now we do the actual file loop
	void* data = std::malloc(76);
	if (data != NULL) {
		std::memset(data, 0, 76);
		
	}
	DWORD32 info = (DWORD32)data;
	DWORD32 MapInfo[3];
	for (auto& i : folder) {
		if (i.find(".scenario") != std::string::npos && i.find(".scenariomarker") == std::string::npos) {
			//resetting info
			rinfo_org((int*)MapInfo);
			//SomethingPath
			std::string p(i);
			smth_path((char*)p.data());
			//Reading map info
			DWORD32 u = mri_file(MapInfo);
			if (u != NULL) {
				//SomethingMapInfo
				__asm {
					push esi;
					push ecx;
					mov esi, info;
					lea ecx, [MapInfo];
					call smth_info;
					pop ecx;
					pop esi;
					//mov esi, dword ptr [inf];
				}
				//smth_info((int*)MapInfo, (void*)info); //probably gotta call this in assembly because compiler used esi to pass original data structure into it
				
			}
			//adddropdownitem
			drpadd_org((int)MapInfo);
		}
	}

	*dat = data;
}*/