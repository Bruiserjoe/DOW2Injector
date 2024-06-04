#include "framework.h"

DWORD base;

BYTE newValue1[] = { 0x15 }; //value for number 21
BYTE newValue2[] = { 0x08 }; //value for number 8
BYTE newValue3[] = { 0x06 }; //value for number 6
BYTE newValue4[] = { 0x09 }; //value for number 9
BYTE newValue5[] = { 0x12 }; //value for number 18
BYTE newValue6[] = { 0x36 }; //value for number 54
BYTE newValue7[] = { 0x14 }; //value for 20
BYTE newValue8[] = { 0xFF }; //value for 255
BYTE newValue9[] = { 0x17, 0x58 }; //value for 5976
BYTE newValue10[] = { 0x01 }; //value for 1
BYTE newValueTest[] = { 0xFF }; //value for testing


void InitPatch()
{
    base = (DWORD)GetModuleHandleA("DOW2.exe");
    if (base)
    {
        //ABILITIES (MOSTLY FUNCTIONAL)
        //sub_B39850
        MemPatch(reinterpret_cast<BYTE*>((base + 0x00739FA6)), newValue1, sizeof(newValue1)); //(NECESSARY) compare, expands number of available abilities, though the new ability slots do not respond to icon indexes beyond 6 in the rbf files - not a priority or major issue though


        //GLOBAL ABILITIES (MOSTLY FUNCTIONAL)
        //sub_B39850
        MemPatch(reinterpret_cast<BYTE*>((base + 0x0073A70D)), newValue1, sizeof(newValue1)); //(NECESSARY) compare, expands number of available global abilities, same as with regular abilities they dont respond to icon indexes beyond original - not a priority or major issue though


        //MINI PORTRAITS (FUNCTIONAL)
        //sub_B39850
        MemPatch(reinterpret_cast<BYTE*>((base + 0x0073B486)), newValue1, sizeof(newValue1)); //(NECESSARY) compare, expands number of available mini portraits

        //sub_B53E50
        MemPatch(reinterpret_cast<BYTE*>((base + 0x00753EDF)), newValue1, sizeof(newValue1)); //(NECESSARY) compare, calls miniportrait address above in a loop to enable functionality of the mini portraits


        //SQUAD TABS (NOT FUNCTIONAL)
        //sub_B76D70
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00776DB9)), newValue5, sizeof(newValue5)); //(NECESSARY) push, constructor for squad tabs
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00776E88)), newValue5, sizeof(newValue5)); //(NECESSARY) compare, iterating over each squad tab
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00776DBB)), newValueTest, sizeof(newValueTest)); //push, size of constructed element (changing this one and the one below to be lower lets us do more loops, but we crash elsewhere)
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00776E82)), newValueTest, sizeof(newValueTest)); //add, size of constructed element
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00776E13)), newValue8, sizeof(newValue8)); //push, size of constructed element 2
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00776E11)), newValue4, sizeof(newValue4)); //push, constructed element 2

        //sub_B77100
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x0077721E)), newValue5, sizeof(newValue5)); //(NECESSARY) push, deconstructor for squad tabs
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00777193)), newValue5, sizeof(newValue5)); //(NECESSARY) compare, iterating over each squad tab

        //sub_B48720
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x007487DE)), newValue5, sizeof(newValue5)); //(NECESSARY) mov, expands number of available squad tabs

        //sub_B77290
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x0077737A)), newValue5, sizeof(newValue5)); //(NECESSARY) mov, enables the functionality of squad tabs, changes amount of available tabs
        //NopPatch(reinterpret_cast<BYTE*>((base + 0x0077750B)), 0x41); //just to see if we can make the squad tabs smaller so we can fit more
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x007773E1)), newValue10, sizeof(newValue10)); //(NECESSARY) mov, forcing minion tabs to spawn in skirmish?

        //sub_B77730
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00777802)), newValue5, sizeof(newValue5)); //(NECESSARY) mov, populates available squad tabs
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x007778BF)), newValue5, sizeof(newValue5)); //(NECESSARY)^^

        //sub_B79A00
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00779F9B)), newValue5, sizeof(newValue5)); //compare, seems potentially relevant to squad tabs, crashes when original squad count of 9 value is reached (exception access violation at B79FC2)
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00779F2A)), newValue5, sizeof(newValue5)); //compare, doesnt seem to do anything on its own 
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00779F73)), newValue5, sizeof(newValue5)); //^^
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00779FAD)), newValue5, sizeof(newValue5)); //mov, straightup crashes when edited, unlikely to be relevant

        //sub_B7A120
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x0077A12C)), newValue5, sizeof(newValue5)); //compare, no idea - main function called by some squad construction function but unlikely to do with squad tabs

        //sub_B7A160
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x0077A2AF)), newValue5, sizeof(newValue5)); //compare, no idea - main function called by sub_B77730, so likely relevant to squad tabs, but not sure as it for some reason compares  as greater/equal to 9 rather than less than 9 which doesnt make sense given the context

        //sub_B48C00
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x007499CF)), newValue5, sizeof(newValue5)); //compare, appears to be in a per-squad-tab function, for health? unlikely to do anything useful, doesnt appear to do anything

        //sub_B78770
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x007787B3)), newValue5, sizeof(newValue5)); //mov, potentially relevant, looks extremely similar to the setup of the other two fuctions for squad tabs (B77730 and B48720)

        //sub_B775C0
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x007775E7)), newValue5, sizeof(newValue5)); //mov, potentially relevant, looks extremely similar to the setup of the other two fuctions for squad tabs (B77730 and B48720)

        //sub_B78460
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x007784A4)), newValue5, sizeof(newValue5)); //mov, potentially relevant, looks extremely similar to the setup of the other two fuctions for squad tabs (B77730 and B48720)

        //sub_B78E80
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00778EA8)), newValue5, sizeof(newValue5)); //mov, potentially relevant, looks extremely similar to the setup of the other two fuctions for squad tabs (B77730 and B48720)

        //sub_B78BD0
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00778C3B)), newValue5, sizeof(newValue5)); //compare, no clue, maybe to do with squads, unlikely of consequence

        //sub_B76D70
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00776E4F)), newValue6, sizeof(newValue6)); //mov, oh fuck we're desperate

        //sub_AD9500
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x006D9648)), newValue5, sizeof(newValue5)); //mov, oh fuck we're desperate part 2, and this probably has nothing to do with it

        //sub_B76F20
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00776F2A)), newValue7, sizeof(newValue7)); //push, maybe a memory allocation issue? doesn't seem to help tho
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00776F65)), newValue8, sizeof(newValue8)); //^^

        //sub_B7A550
        //NopPatch(reinterpret_cast<BYTE*>((base + 0x0077AB98)), 0x13F); //just to see if we can make the squad tabs smaller so we can fit more
        //NopPatch(reinterpret_cast<BYTE*>((base + 0x0077A669)), 0xB4); //^^

        //sub_AD3D80
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x006D5359)), newValue9, sizeof(newValue9)); //push, maybe a memory allocation issue? doesn't seem to help tho


        //BUILD QUEUE (NOT FUNCTIONAL)
        //sub_B39850
        MemPatch(reinterpret_cast<BYTE*>((base + 0x0073B6F5)), newValue1, sizeof(newValue1)); //(NECESSARY) compare, number of available build queue elements

        //sub_B714D0
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x007715A0)), newValue3, sizeof(newValue3)); //compare, either UI or a production ystem compare, might help expand max build queue


        //OBSERVER PANEL PLAYERS/TEAMS (NOT FUNCTIONAL)
        //sub_B3C780
        MemPatch(reinterpret_cast<BYTE*>((base + 0x0073D937)), newValue2, sizeof(newValue2)); //(NECESSARY) compare, expands number of available players/teams in observer
        MemPatch(reinterpret_cast<BYTE*>((base + 0x0073D94B)), newValue2, sizeof(newValue2)); //(NECESSARY)^^

        //sub_B63BE0
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00764A53)), newValue2, sizeof(newValue2)); //compare, appears to be for partly enabling the UI elements, though just these three don't fully work on their own yet either - game also crashes when loading *out* of the replay, and crashes if observing a live game outright
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00764A6F)), newValue2, sizeof(newValue2)); //^^
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00764951)), newValue4, sizeof(newValue4)); //^^ - for some reason compares against 7 and not 6? now using 9 (assuming 1 above max player count)

        //sub_B68B10
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00768B3F)), newValue2, sizeof(newValue2)); //compare, compares player count, initially at 6, crashes with or without 8p_replay_fix, probably shouldnt touch it though
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00768BAB)), newValue2, sizeof(newValue2)); //^^
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00768BC7)), newValue2, sizeof(newValue2)); //^^
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00768BDF)), newValue2, sizeof(newValue2)); //^^
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00768B7F)), newValue2, sizeof(newValue2)); //^^

        //sub_B63A20
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00763B7A)), newValue2, sizeof(newValue2)); //mov, May be related to max players
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x00763B9B)), newValue2, sizeof(newValue2)); //compare, May be related to max players

        //sub_B65020
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x0076525B)), newValue2, sizeof(newValue2)); //compare, May be related to max players

        //sub_B6C940
        //MemPatch(reinterpret_cast<BYTE*>((base + 0x0076CBE2)), newValue2, sizeof(newValue2)); //compare, may be related to max players
    }
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        InitPatch();
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}