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
- This is still early in development so issues are expected, make a issue if you have a problem
