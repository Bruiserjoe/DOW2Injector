#include "Injector.h"
#include <bitset>






//https://reverseengineering.stackexchange.com/questions/8120/making-application-load-dll-at-start
//https://www.unknowncheats.me/forum/programming-for-beginners/83148-automatically-inject-dll.html


bool contains(std::string str1, std::string f) {
    unsigned int j = 0;
    for (unsigned int i = 0; i < str1.size(); i++) {
        if (str1[i] == f[j]) {
            j++;
        }
        else {
            if (j >= f.size()) {
                return true;
            }
            j = 0;
        }
    }
    //in case the very last set of characters matched
    if (j >= f.size()) {
        return true;
    }
    return false;
}
std::string readafterequal(std::string str) {
    unsigned int i = 0;
    std::string st = "";
    for (i; i < str.size() && str[i] != '='; i++);
    i++;
    for (i; i < str.size(); i++) {
        st.push_back(str[i]);
    }
    return st;
}


int main(int argc, char* argv[])
{   
    std::vector<std::string> args;
    std::string p = ""; //cfgpath
    for (int i = 0; i < argc; i++) {
        std::string tt(argv[i]);
        std::cout << tt << "\n";
        if (contains(tt, "target=")) {
            p = readafterequal(tt);
        }
    }
    Injector inject;
    inject.start(p);
}
