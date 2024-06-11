#include "Injector.h"

std::string Injector::createcfg(std::string module) {
	exe_name = getExe();
	std::string path = exe_name;

	createcfgpath(path, module);
	return cfg_path;
}


std::string cullexe(std::string path) {
	int i = path.size() - 1;
	for (i; i >= 0 && path[i] != '/'; i--);
	std::string ret = "";
	for (size_t j = 0; j <= (size_t)i; j++) {
		ret.push_back(path[j]);
	}
	return ret;
}

std::string cullPeriod(std::string str) {
	std::string re = "";
	for (uint32_t i = 0; i < str.size() && str[i] != '.'; i++) {
		re.push_back(str[i]);
	}
	return re;
}	

//generates the path to cfg
void Injector::createcfgpath(std::string path, std::string module) {
	for (int i = 0; (size_t)i < path.size(); i++) {
		if (path[i] == '\\') {
			path[i] = '/';
		}
	}
	std::string p = cullexe(path);
	std::string tmod = (module.compare(".config") == 0) ? "player_cfg.lua" : cullPeriod(module);
	p = p + tmod + "_playercfg.lua";
	cfg_path = p;
}