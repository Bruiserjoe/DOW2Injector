#pragma once
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>



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

typedef void(__stdcall* LoadMaps)(char* path, void* param2);
LoadMaps ldmaps_org = nullptr;
typedef DWORD32* (__thiscall* MapDropdown)(void* tis, int param1);
MapDropdown mpdrp_org = nullptr;
typedef void(__stdcall* DropDownAdd)(int param1);
DropDownAdd drpadd_org = nullptr;

//we keep our own list of maps for each folder, so we can set the list to what ever we want
/*class MapLoader {
private:
	struct MapAddr {
		DWORD32 addr; //actual data addr
		size_t g_index; //game index
		std::string path; //file path
	};

	DWORD offset;
	DWORD32 campaign_maps;
	std::vector<MapAddr> map_lists;
public:
	MapLoader() {
		offset = 0;
		campaign_maps = 0;
	}
	MapLoader(DWORD base) {
		offset = base + 0xf357a0;
		campaign_maps = *((DWORD32*)*((DWORD32*)offset)) + 0x0;
	}
	~MapLoader() {
		for (auto& i : map_lists) {
			std::free((void*)i.addr);
		}
	}
	size_t generateMapList(std::string file_path, size_t g_index) {
		void* dat = std::malloc(100); //no idea how big this has to be
		DWORD32 t = (DWORD32)dat; //can't use the default map list since it seems to already have it packed tight
		char* d = (char*)t;
		*(DWORD32*)(t + 0x48) = 0;
		//this works now?
		ldmaps_org((char*)file_path.c_str(), (void*)t); //figure out why we get ntl error here, probably arguments


		DWORD32* tp = mpdrp_org((d + 4), (int)(d + 4)); //begin pointer
		//loop to set drop down buttons?
		for (DWORD32* i = tp; i != (DWORD32*)(d + 0x4); i += 0x1eb) {
			//do the map cleanupread function
			drpadd_org((int)i);
		}
		//(DWORD32*)(t + 4) = tp;
		map_lists.push_back({ t, g_index, file_path });
		return map_lists.size() - 1;
	}
	DWORD32 getMapList(std::string name) {

	}
	DWORD32 __cdecl getMapList(size_t game) {
		for (auto& i : map_lists) {
			if (i.g_index == game) {
				return i.addr;
			}
		}
		DWORD32 t = 0;
		if (g_ffa == 0 && g_tffa == 0) {
			char* data;
			__asm {
				push edx;
				mov edx, dword ptr[campaign_maps];
				add edx, 0xc;
				mov data, edx;
				mov t, edx;
				pop edx;
			}
		}
		else {
			__asm {
				push edx;
				mov edx, dword ptr[campaign_maps];
				add edx, 0x3c;
				mov t, edx;
				pop edx;
			}
		}

		return t;
	}


};
*/


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
	//gets the map list from the config file
	std::string getList(std::string line) {
		size_t pos = line.find("list:");
		pos += 5;
		std::string str;
		for (pos; pos < line.size() && line[pos] != ';'; pos++) {
			if (line[pos] != ' ' && line[pos] != '\t') {
				str.push_back(line[pos]);
			}
		}
		if (str.compare("default") != 0) {
			str = "Data:Maps/" + str + "/";
		}
		return str;
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







