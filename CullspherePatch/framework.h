#pragma once
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <string>
#include <detours.h>
#include <fstream>
#include <vector>

//equation that determines how the cullsphere is modulated
class Equation {
private:
	enum act {val = 0, add = 1, sub = 2, multi = 3, div = 4, sin = 5, cos = 6, tan = 7, dist = 8, pl = 9, pr = 10};
	struct op {
		act ac;
		float val;
	};
	std::vector <op> ops;
	std::string eq;
	void ev() {
		std::string cur_number = "";
		for (uint32_t i = 0; i < eq.size(); i++) {
			switch (eq[i]) {
				case 's':
					if (eq[i + 1] == 'i' && eq[i + 2] == 'n') {
						ops.push_back({ sin });
						i += 2;
					}
					cur_number.clear();
				break;
				case 'd':
					if (eq[i + 1] == 'i' && eq[i + 2] == 's' && eq[i + 3] == 't') {
						ops.push_back({ dist });
						i += 3;
					}
					cur_number.clear();
					break;
				case '*':

					ops.push_back({ multi });
					break;
				case '+':
					ops.push_back({ add });
					break;
				case '-':
					ops.push_back({ sub });
					break;
				case '/':
					ops.push_back({ div });
					break;
				case '(':
					ops.push_back({ pl });
					break;
				case ')':
					ops.push_back({ pr });
					break;
				case ' ':
					if (cur_number.size() > 0) {
						ops.push_back({ val, std::stof(cur_number) });
						cur_number.clear();
					}
					break;
				default:
					cur_number.push_back(eq[i]);
					i++;
					break;
			}
		}
	}
public:
	Equation() {

	}

	Equation(std::string path) {
		//path to equation
		std::ifstream fi;
		fi.open(path);
		std::string def = "(sin(dist * 0.0037 - 0.5) + 1.0) * 30";
		if (!fi) {
			eq = def;
		}
		else {
			std::streamsize size = fi.tellg();
			std::vector <char> buffer = std::vector<char>(size);
			fi.seekg(0, std::ios::beg);
			if (fi.read(buffer.data(), size)) {
				//parse the string into operators
				eq = "";
				for (auto& i : buffer) {
					if (i != '\n') {
						eq.push_back(i);
					}
					else {
						break;
					}
				}
			}
		}
		fi.close();
	}
	//copy constructor
	Equation (const Equation& e) {
		this->eq = e.eq;
	}
	//move constructor
	Equation (Equation&& e) noexcept {
		this->eq = e.eq;
	}

	float evaluate (float distance) {
		
	}
};