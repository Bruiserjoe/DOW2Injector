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


struct Mode {
	int ffa;
	int t_ffa;
	bool verify;
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

	bool getVerify(std::string line) {
		size_t pos = line.find("verify:");
		pos += 7;
		std::string str;
		for (pos; pos < line.size() && line[pos] != ';'; pos++) {
			if (line[pos] != ' ' && line[pos] != '\t') {
				str.push_back(line[pos]);
			}
		}
		bool r = str.compare("true") == 0;
		return r;
	}
public:
	
	GamemodeMap() {
		for (size_t i = 0; i < 100; i++) {
			lookup.push_back({ 0, 0});
		}
	}
	void insert(size_t index, int ffa, int t_ffa, bool verify) {
		if (index < lookup.size()) {
			lookup[index].ffa = ffa;
			lookup[index].t_ffa = t_ffa;
			lookup[index].verify = verify;
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
			insert(getIndex(line), getFFA(line), getTFFA(line), getVerify(line));
		}
		file.close();
	}
	
};







