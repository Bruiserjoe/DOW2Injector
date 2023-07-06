#include "Injector.h"
#include <bitset>






//https://reverseengineering.stackexchange.com/questions/8120/making-application-load-dll-at-start
//https://www.unknowncheats.me/forum/programming-for-beginners/83148-automatically-inject-dll.html





int main()
{
    /*size_t st = 0x600;
    //testing binary
    std::bitset<32> bitst(st);
    std::cout << "bits: " << bitst << "\n";
    */

    Injector inject;
    inject.start();
}
