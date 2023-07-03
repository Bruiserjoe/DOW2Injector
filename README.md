# DOW2Injector
DLL injector which injects DLLs into DOW2 and starts the game to make sure the dlls load correctly

Setup guide
- Download and extract
- Put the exe and SetupDLL.dll in main Retribution folder and create a folder called mods
- put BaseDLL.dll and CullspherePatch.dll in mods folder
- Run the exe to play the game with the patches
- Take a look at the config file generated after first run

Known Issues
- Crashes are possible when navigating menus

Fixes
- If you experience crashes on startup or first menu button pressed try increasing the sleep-after-menu time, it's in milliseconds
- Otherwise try changing the load order
- This is still early in development so issues are expected


Gamemode Patch
- Release comes packaged with an example config
- The config file must be named gmd.cfg and be stored in the mods folder
- Example member layout:
  - (index): ffa: false; tffa: true;
  - (index2): ffa: true; tffa: false
- ffa and tffa can not both be true so make sure not to mix them
- 0-3 in example config are the built in gamemodes so I recommend leaving them as is
