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
	enum act { val = 0, add = 1, sub = 2, multi = 3, div = 4, sin = 5, cos = 6, tan = 7, dist = 8, pl = 9, pr = 10 };
	struct op {
		act ac;
		float val;
	};
	std::vector <op> ops;
	std::string eq;
	float max_distance;
	void ev() {
		std::string cur_number = "";
		for (uint32_t i = 0; i < eq.size(); i++) {
			switch (eq[i]) {
			case 's':
				if (cur_number.size() > 0) {
					ops.push_back({ val, std::stof(cur_number) });
					cur_number.clear();
				}
				if (eq[i + 1] == 'i' && eq[i + 2] == 'n') {
					ops.push_back({ sin });
					i += 2;
				}
				cur_number.clear();
				break;
			case 'c':
				if (cur_number.size() > 0) {
					ops.push_back({ val, std::stof(cur_number) });
					cur_number.clear();
				}
				if (eq[i + 1] == 'o' && eq[i + 2] == 's') {
					ops.push_back({ cos });
					i += 2;
				}
				cur_number.clear();
				break;
			case 'd':
				if (cur_number.size() > 0) {
					ops.push_back({ val, std::stof(cur_number) });
					cur_number.clear();
				}
				if (eq[i + 1] == 'i' && eq[i + 2] == 's' && eq[i + 3] == 't') {
					ops.push_back({ dist });
					i += 3;
				}
				cur_number.clear();
				break;
			case '*':
				ops.push_back({ multi, (cur_number.size() > 0) ? std::stof(cur_number) : 0.0f });
				cur_number.clear();
				break;
			case '+':
				ops.push_back({ add ,  (cur_number.size() > 0) ? std::stof(cur_number) : 0.0f });
				cur_number.clear();
				break;
			case '-':
				ops.push_back({ sub,  (cur_number.size() > 0) ? std::stof(cur_number) : 0.0f });
				cur_number.clear();
				break;
			case '/':
				ops.push_back({ div,  (cur_number.size() > 0) ? std::stof(cur_number) : 0.0f });
				cur_number.clear();
				break;
			case '(':
				if (cur_number.size() > 0) {
					ops.push_back({ val, std::stof(cur_number) });
					cur_number.clear();
				}
				if ((ops.size() > 0) ? ops[ops.size() - 1].ac != sin : true) {
					ops.push_back({ pl });
				}
				break;
			case ')':
				if (cur_number.size() > 0) {
					ops.push_back({ val, std::stof(cur_number) });
					cur_number.clear();
				}
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
				break;
			}
		}
		if (cur_number.size() > 0) {
			ops.push_back({ val, std::stof(cur_number) });
			cur_number.clear();
		}
	}
	static float eval(float d, std::vector<op>& t, bool* set_cur) {
		float cur = 0.0f;
		for (uint32_t i = 0; i < t.size();) {
			std::vector<op> temp;
			float v;
			switch (t[i].ac) {
			case pl:
				for (i = i + 1; i < t.size() && t[i].ac != pr; i++) {
					temp.push_back(t[i]);
				}
				cur += eval(d, temp, set_cur);
				break;
			case add:
				v = t[i + 1].val;
				cur += v;
				i++;
				break;
			case sub:
				v = t[i + 1].val;
				cur -= v;
				i++;
				break;
			case multi:
				v = t[i + 1].val;
				cur *= v;
				i++;
				break;
			case div:
				v = t[i + 1].val;
				cur /= v;
				i++;
				break;
			case sin:
				for (i = i + 1; i < t.size() && t[i].ac != pr; i++) {
					temp.push_back(t[i]);
				}
				v = eval(d, temp, set_cur);
				cur += sinf(v);
				break;
			case cos:
				for (i = i + 1; i < t.size() && t[i].ac != pr; i++) {
					temp.push_back(t[i]);
				}
				v = eval(d, temp, set_cur);
				cur += cosf(v);
				break;
			case tan:
				for (i = i + 1; i < t.size() && t[i].ac != pr; i++) {
					temp.push_back(t[i]);
				}
				v = eval(d, temp, set_cur);
				cur += tanf(v);
				break;
			case dist:
				if (*set_cur) {
					cur = d;
					*set_cur = false;
				}
				i++;
				break;
			default:
				i++;
				break;
			}
		}
		return cur;
	}

public:
	Equation() {
		std::string def = "(sin(dist * 0.0037 - 0.5) + 1.0) * 30";
		eq = def;
		ev();
		max_distance = 800.0f;
	}

	Equation(std::string path) {
		//path to equation
		std::ifstream fi;
		fi.open(path, std::ifstream::ate | std::ifstream::binary);
		std::string def = "(sin(dist * 0.0037 - 0.5) + 1.0) * 30";
		if (!fi) {
			eq = def;
		}
		else {
			std::string str = "";
			std::streamsize size = fi.tellg();
			fi.close();
			fi.open(path, std::ios::binary);
			std::vector <char> buffer = std::vector<char>(size);
			fi.seekg(0, std::ios::beg);
			if (fi.read(buffer.data(), size)) {
				//parse the string into operators
				eq = "";
				bool mode = false;
				for (auto& i : buffer) {
					if (i != '\n') {
						if (!mode) {
							eq.push_back(i);
						}
						else {
							str.push_back(i);
						}
					}
					else {
						mode = true;
					}
				}
				max_distance = (str.size() > 0) ? std::stof(str) : 800.0f;
			}
		}
		fi.close();
		ev();
	}
	//copy constructor
	Equation(const Equation& e) {
		this->eq = e.eq;
		this->ops = e.ops;
	}

	Equation& operator=(const Equation& e) {
		//self assignment
		if (this == &e) {
			return *this;
		}
		this->eq = e.eq;
		this->ops = e.ops;
		return *this;
	}

	float evaluate(float distance) {
		for (uint32_t i = 0; i < ops.size(); i++) {
			if (ops[i].ac == dist) {
				ops[i].val = distance;
			}
		}
		bool set_cur = true;
		return eval(distance, ops, &set_cur);
	}
	float getMax() {
		return max_distance;
	}
};