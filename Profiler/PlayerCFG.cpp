#include "Injector.h"

std::string Injector::createcfg(std::string module) {
	TCHAR szExeFileName[MAX_PATH];
	GetModuleFileName(NULL, szExeFileName, MAX_PATH);
	exe_name = std::string(szExeFileName);
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
//generates the path to cfg
void Injector::createcfgpath(std::string path, std::string module) {
	for (int i = 0; (size_t)i < path.size(); i++) {
		if (path[i] == '\\') {
			path[i] = '/';
		}
	}
	std::string p = cullexe(path);
	p = p + module + "_playercfg.lua";
	cfg_path = p;
}