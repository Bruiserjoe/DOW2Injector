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

std::string Injector::readConfig(std::string path) {
    std::cout << "Reading config \n";
    std::string ret = "";
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
        pos = str.find("exe-path:");
        con = readAfterColonWithSpaces(str, pos);
        if (con.compare("local") != 0) {
            ret = con;
            local_folder = false;
        }
        else {
            ret = "DOW2.exe";
            local_folder = true;
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
        pos = str.find("launch-options:");
        con.clear();
        con = readAfterColonInQuotes(str, pos);
        ret = ret + con;

        pos = str.find("sleep-after-menu:");
        con = readAfterColon(str, pos);
        sleep_time = std::stoi(con);
        pos = str.find("console:");
        con = readAfterColon(str, pos);
        if (parseToBool(convertToLower(con))) {
            ShowWindow(GetConsoleWindow(), SW_HIDE);
        }
        pos = str.find("window:");
        con = readAfterColon(str, pos);
        if (parseToBool(convertToLower(con))) {
            // window = false;
        }

        pos = str.find("img:");
        con = readAfterColon(str, pos);
        image_path = con;
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
    }
    else {
        std::ofstream file;
        file.open(exe_name + ".config");
        file << "module: none\n";
        file << "mod-folder: mods\n";
        file << "launch-options: \" -dev -nomovies\"\n";
        file << "sleep-after-menu: 500\n";
        file << "exe-path: local\n";
        file << "strict-load: false\n";
        file << "load-order:\n"; 
        file << "end-load\n";
        file << "console: true\n";
        file << "window: true\n";
        file << "img: test.bmp\n";
        file.close();
        mods_folder = "mods";
        strict_load = false;
        ret = "DOW2.exe";
    }
    return ret;
}

