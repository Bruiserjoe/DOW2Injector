#pragma once
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <string>
#include <detours.h>
#include <fstream>
#include <vector>

typedef void(__cdecl* Timestampedf)(const char*, ...);
typedef void(__cdecl* Fatalf)(const char*, ...);

extern Timestampedf Timestampedtracef;
extern Fatalf Fatal_f;

//equation that determines how the cullsphere is modulated
class Config {
private:
	float max_distance;
	float rate;
	std::string readBufferLine(std::vector <char>& buffer, size_t* index) {
		std::string str = "";
		for (; *index < buffer.size() && buffer[*index] != '\n'; (*index)++) {
			str.push_back(buffer[*index]);
		}
		(*index)++;
		return str;
	}
public:
	Config() {
		max_distance = 800.0f;
		rate = 0.0037f;
	}

	Config(std::string path) {
		//path to equation
		std::ifstream fi;
		max_distance = 800.0f;
		rate = 0.0037f;
		fi.open(path, std::ifstream::ate | std::ifstream::binary);
		if (!fi) {
			max_distance = 800.0f;
		}
		else {
			std::streamsize size = fi.tellg();
			fi.close();
			fi.open(path, std::ios::binary);
			std::vector <char> buffer = std::vector<char>(size);
			fi.seekg(0, std::ios::beg);
			if (fi.read(buffer.data(), size)) {
				size_t index = 0;
				std::string max_str = readBufferLine(buffer, &index);
				max_distance = (max_str.size() > 0) ? std::stof(max_str) : 800.0f;
				std::string rate_str = readBufferLine(buffer, &index);
				rate = (rate_str.size() > 0) ? std::stof(rate_str) : 0.0037f;
			}
		}
		fi.close();
	}
	//copy constructor
	Config(const Config& e) {
		this->max_distance = e.max_distance;
		this->rate = e.rate;
	}

	Config& operator=(const Config& e) {
		//self assignment
		if (this == &e) {
			return *this;
		}
		this->max_distance = e.max_distance;
		return *this;
	}

	float getMax() {
		return max_distance;
	}
	float getRate() {
		return rate;
	}
};