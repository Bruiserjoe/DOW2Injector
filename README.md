# DOW2Injector
DLL injector which injects DLLs into DOW2 and starts the game to make sure the dlls load correctly

Setup guide
- Download and extract
- Put the exe and SetupDLL.dll in main Retribution folder and create a folder called mods
- put BaseDLL.dll and CullspherePatch.dll in mods folder
- Run the exe to play the game with the patches
- Take a look at the config file generated after first run

Fixes
- If you experience crashes on startup or first menu button pressed try increasing the sleep-after-menu time, it's in milliseconds
- Otherwise try changing the load order
- This is still early in development so issues are expected


Gamemode Patch
- Release comes packaged with an example config
- The config file must be named gmd.cfg and be stored in the mods folder
- Example member layout:
  - (index): ffa: false; tffa: true; list: default;
  - (index2): ffa: true; tffa: false; list: glorb;
- ffa and tffa can not both be true so make sure not to mix them
- 0-3 in example config are the built in gamemodes so I recommend leaving them as is
- List section in line corresponds to the map list you want to use. Default is just default pvp or ffa for each gamemode. If you want a custom map list put it in Data/maps/(your folder)
- Make sure the name is correct in list or else could cause crashes or other errors

Cullsphere Patch
- Simple as putting in mods folder
- Will dynamically change cullsphere size up to zoom level 800 at which point it will set it to a small level to cull basically everything
- Past 800 you can't really see much do to fog so this is not such a big deal
- This can have a hit on performance but through testing it isn't so bad on 100-200 levels

Shell Patch
- Put in your designated mods folder
- Edit selection_panel.gfx and selection_panel.lua to add your shells to UI
    1. Add new shell images your module, see images below for where <br />
       ![alt text](https://github.com/Bruiserjoe/DOW2Injector/blob/main/Capture1.PNG?raw=true)
       ![alt text](https://github.com/Bruiserjoe/DOW2Injector/blob/main/Capture2.PNG?raw=true)
    3. Edit the gfx file.
         1. Start with adding a new image import for each new shell, easiest way is to just clone and edit an existing one <br />
            ![alt text](https://github.com/Bruiserjoe/DOW2Injector/blob/main/Capture3.PNG?raw=true)
         2. Add your DefineShape and DefineSprites for the individual shell, see image below for more detail <br />
            ![alt text](https://github.com/Bruiserjoe/DOW2Injector/blob/main/Capture4.PNG?raw=true)
         3. Add your PlaceObject for the shell in your waaagh_mc object, yours should be 366, in the image it is 437 <br />
            ![alt text](https://github.com/Bruiserjoe/DOW2Injector/blob/main/Capture5.PNG?raw=true)
    4. Edit your {module_name}.shells file in main DOW2 Retribution folder, follow example files outline (MAKE SURE THERE ARE NO SPACES PRESENT!)

Eight Player FFA Patch
- Allows you to use eight player maps in ffa gamemodes
- Player slots are a bit janky on eight player solo ffa, go to team ffa and create them and go back to solo ffa and they should be filled

