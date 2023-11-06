#include "Injector.h"


std::string cullexe(std::string path) {
	int i = path.size() - 1;
	for (i; i >= 0 && path[i] != '/'; i--);
	std::string ret = "";
	for (size_t j = 0; j <= i; j++) {
		ret.push_back(path[j]);
	}
	return ret;
}
//generates the path to cfg
void Injector::createcfgpath(std::string path) {
	for (int i = 0; i < path.size(); i++) {
		if (path[i] == '\\') {
			path[i] = '/';
		}
	}
	std::string p = cullexe(path);
	p = p + exe_name + "_playercfg.lua";
	cfg_path = p;
}
bool checkClipboard(std::string comp) {
	bool ret = false;
	if (OpenClipboard(NULL)) {
		char* buffer = (char*)GetClipboardData(CF_TEXT);
		if (buffer) {
			std::string str(buffer);
			if (str.compare(comp) == 0) {
				ret = true;
			}
		}
	}
	CloseClipboard();
	return ret;
}

//send the cfg path to setupdll
void Injector::communicatecfgpath() {
	//now block until we've got confirmation that setupdll has read it
	const char* msg = cfg_path.c_str();
	OpenClipboard(NULL);
	HGLOBAL glob = GlobalAlloc(GMEM_FIXED, sizeof(char) * cfg_path.size() + 1);
	char* buffer = (char*)GlobalLock(glob);
	if (buffer != NULL) {
		strcpy_s(buffer, sizeof(char) * cfg_path.size() + 1, msg);
	}
	GlobalUnlock(glob);

	SetClipboardData(CF_TEXT, glob);
	CloseClipboard();
	while (true) {
		Sleep(10);
		if (checkClipboard("Finish CFG")) {
			break;
		}
	}
}