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

std::string Injector::readConfig() {
    std::cout << "Reading config \n";
    std::string ret = "";
    if (file_exists(exe_name + ".config")) {
        std::ifstream file;
        file.open(exe_name + ".config");
        std::stringstream stream;
        stream << file.rdbuf();
        std::string str = stream.str();
        size_t pos;
        std::string con = "";
        pos = str.find("exe-path:");
        con = readAfterColonWithSpaces(str, pos);
        if (con.compare("local") != 0) {
            ret = con;
        }
        else {
            ret = "DOW2.exe";
        }
        pos = str.find("module:");
        con = readAfterColon(str, pos);
        if (con.compare("none") != 0) {
            ret = ret + " -modname " + con;
        }
        pos = str.find("mod-folder:");
        con.clear();
        con = readAfterColon(str, pos);
        mods_folder = con;
        pos = str.find("dev:");
        con = readAfterColon(str, pos);
        if (con.compare("true") == 0) {
            ret = ret + " -dev";
        }
        pos = str.find("skip-movies:");
        con = readAfterColon(str, pos);
        if (con.compare("true") == 0) {
            ret = ret + " -nomovies";
        }
        pos = str.find("windowed:");
        con = readAfterColon(str, pos);
        if (con.compare("true") == 0) {
            ret = ret + "-window";
        }
        pos = str.find("sleep-after-menu:");
        con = readAfterColon(str, pos);
        sleep_time = std::stoi(con);
        //reading load order
        pos = str.find("load-order:");
        std::string line = readLine(str, &pos);
        while ((line = readLine(str, &pos)).compare("end-load")) {
            load_order.push_back(line);
        }
    }
    else {
        std::ofstream file;
        file.open(exe_name + ".config");
        file << "module: none\n";
        file << "mod-folder: mods\n";
        file << "dev: false\n";
        file << "skip-movies: true\n";
        file << "windowed: false\n";
        file << "sleep-after-menu: 500\n";
        file << "exe-path: local\n";
        file << "load-order:\n"; 
        file << "end-load\n";
        file.close();
        mods_folder = "mods";
        ret = "DOW2.exe";
    }
    return ret;
}

