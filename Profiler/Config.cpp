#include "Injector.h"




std::string readAfterColon(std::string data, size_t start) {
    size_t pos = start;
    for (pos; pos < data.size() && data[pos] != ':'; pos++);
    pos++;
    std::string ret1 = "";
    for (pos; pos < data.size() && data[pos] != '\n'; pos++) {
        if (data[pos] != ' ') {
            ret1.push_back(data[pos]);
        }
    }
    return ret1;
}


std::string readAfterColonInQuotes(std::string data, size_t start) {
    size_t pos = start;
    for (pos; pos < data.size() && data[pos] != ':'; pos++);
    pos++;
    for (pos; pos < data.size() && data[pos] != '\"'; pos++);
    pos++;
    std::string ret1 = "";
    
    for (pos; pos < data.size() && data[pos] != '\n' && data[pos] != '\"'; pos++) {
        ret1.push_back(data[pos]);
    }
    return ret1;
}
std::string readAfterColonWithSpaces(std::string data, size_t start) {
    size_t pos = start;
    for (pos; pos < data.size() && data[pos] != ':'; pos++);
    pos++;
    //now skip spaces before the actual data
    for (pos; pos < data.size() && (data[pos] == ' ' || data[pos] == '\t'); pos++);
    std::string ret1 = "";
    for (pos; pos < data.size() && data[pos] != '\n'; pos++) {
        ret1.push_back(data[pos]);
    }
    return ret1;
}
std::string readLine(std::string data, size_t* start) {
    std::string ret = "";
    for (*start; *start < data.size() && data[*start] != '\n'; (*start)++) {
        ret.push_back(data[*start]);
    }
    (*start)++;
    return ret;
}

std::string convertToLower(std::string str) {
    std::string next = "";
    for (auto& i : str) {
        if (i >= 'A' && i <= 'Z') {
            next.push_back(i + 32);
        }
        else {
            next.push_back(i);
        }
    }
    return next;
}

bool parseToBool (std::string str) {
    return str.compare("true") == 0;
}

std::string readAfterSpace(std::string data, size_t* start) {
    std::string ret = "";
    for (*start; *start < data.size() && data[*start] != ' '; (*start)++);
    (*start)++;
    for (*start; *start < data.size() && data[*start] != ' ' && data[*start] != '-'; (*start)++) {
        ret.push_back(data[*start]);
    }
    (*start)++;
    return ret;
}

bool Injector::readConfig(std::string path) {
    load_order.clear();
    immediate_load.clear();
    std::cout << "Reading config \n";
    bool ret = false;
    std::string cfgname;
    if (path.compare("") != 0) {
        cfgname = path;
    }
    else {
        cfgname = exe_name + ".config";
    }
    if (file_exists(cfgname)) {
        std::ifstream file;
        file.open(cfgname);
        std::stringstream stream;
        stream << file.rdbuf();
        std::string str = stream.str();
        size_t pos;
        std::string con = "";
        pos = str.find("launch-options:");
        con = readAfterColonInQuotes(str, pos);
        launch_options = con;

        pos = str.find("mod-folder:");
        con = readAfterColon(str, pos);
        if (con.compare("none") == 0) {
            return false;
        }
        mods_folder = con;


        pos = str.find("strict-load:");
        con = readAfterColon(str, pos);
        if (parseToBool(convertToLower(con))) {
            strict_load = true;
        }
        //reading load order
        pos = str.find("load-order:");
        std::string line = readLine(str, &pos);
        while ((line = readLine(str, &pos)).compare("end-load")) {
            load_order.push_back(line);
        }
        //reading the immediate loads
        pos = str.find("immediate-load:");
        line = readLine(str, &pos);
        while ((line = readLine(str, &pos)).compare("end-immediate")) {
            immediate_load.push_back(line);
        }
        ret = true;
    }
    else {
        std::ofstream file;
        file.open(path);
        file << "launch-options: \"\"\n";
        file << "mod-folder: none\n";
        file << "strict-load: false\n";
        file << "load-order:\n"; 
        file << "end-load\n";
        file << "immediate-load:\n";
        file << "end-immediate\n";
        file.close();
        mods_folder = "mods";
        strict_load = false;
        ret = false;
    }
    return ret;
}

std::string ripOption(size_t* pos, std::string str) {
    for (; *pos < str.size() && str[*pos] != '-'; (*pos)++);
    std::string ret = "";
    for (; *pos < str.size() && str[*pos] != ' '; (*pos)++) {
        ret.push_back(str[*pos]);
    }
    return ret;
}

std::vector<std::string> splitOnDash(std::string str) {
    std::vector<std::string> additionalValue = {
        "-modname",
        "-exec",
        "-gameendexec",
        "-recover",
        "-testRegion",
        "-Init",
        "-logs",
        "-scar",
        "-connect_lobby",
        "-randSeed",
        "-shadowSize",
        "-FXnear",
        "-FXfar",
        "-FXRenderLimit",
        "-FXObjectLimit",
    };

    std::vector<std::string> ret;
    for (size_t i = 0; i < str.size();) {
        std::string s = ripOption(&i, str);
        if (s.length() > 1) {
            bool additional = false;
            for (auto& j : additionalValue) {
                if (s.compare(j) == 0) {
                    additional = true;
                    break;
                }
            }
            if (additional) {
                std::string add =  readAfterSpace(str, &i);
                s = s + " " +  add;
            }
            ret.push_back(s);
        }
    }
    return ret;
}



bool Injector::testCmdLine() {
    std::string cmdLine = GetCommandLineA();
    std::vector<std::string> commands = splitOnDash(cmdLine);
    std::vector<std::string> launchs = splitOnDash(launch_options);
    for (auto& i : launchs) {
        bool ret = true;
        for (auto& j : commands) {
            if (j.compare(i) == 0) {
                ret = false;
            }
        }
        if (ret) return false;
    }
    return true;
}

std::string Injector::getModuleCmdLine() {
    std::string cmdLine = GetCommandLineA();
    //get module name
    size_t pos = cmdLine.find("-modname");
    if (pos != std::string::npos) {
        std::string ret = readAfterSpace(cmdLine, &pos);
        return ret;
    }
    return "";
}