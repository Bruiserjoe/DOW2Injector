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

bool Injector::readConfig(std::string path) {
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
        pos = str.find("mod-folder:");
        con.clear();
        con = readAfterColon(str, pos);
        mods_folder = con;

        pos = str.find("sleep-after-menu:");
        con = readAfterColon(str, pos);
        sleep_time = std::stoi(con);

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
        ret = true;
    }
    else {
        std::ofstream file;
        file.open(path);
        file << "mod-folder: mods\n";
        file << "sleep-after-menu: 500\n";
        file << "strict-load: false\n";
        file << "load-order:\n"; 
        file << "end-load\n";
        file.close();
        mods_folder = "mods";
        strict_load = false;
        ret = false;
    }
    return ret;
}

