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
		return {-1, -1};
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