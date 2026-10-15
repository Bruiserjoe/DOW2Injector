# DOW2Injector

DOW2Injector replaces `Profiler.dll` with a compatible DLL injector for *Dawn of War II: Retribution*. It preserves the exports expected by the game, loads configured patch DLLs, and provides a shared patching and signature-scanning API.

## Changes since release 1.8.3

- Replaced the original injector path with the Profiler-based injector.
- Preserved the original seven `Profiler.dll` exports and ordinals required by the game.
- Added shared Profiler exports for `MemPatch`, `NopPatch`, `JmpPatch`, and `SigScan`.
- Converted patch DLLs to use the shared Profiler exports instead of maintaining separate patch implementations.
- Added `SigScan|x86` builds for version-tolerant runtime address resolution.
- Added signature-scanning support to:
  - 8p Replay Fix
  - Cullsphere Patch
  - Eight Player FFA Patch
  - Gamemode Patch
  - Misc Fixes
  - Shell Patch
  - UI Expansion
  - Window Patch
- Reworked the Window Patch to use borderless fullscreen without changing the monitor's display resolution.
- Fixed Gamemode Patch map-list handling across resolution changes.
- Reworked the Eight Player FFA and Cullsphere patches around assembly hooks.
- Improved Shell Patch stability and shell-selection behavior.
- Reworked the Network Patch.

## Installation

1. Download and extract the release into the main Retribution installation directory.
2. Create a `mods` folder in that directory if one does not already exist.
3. Put the desired patch DLLs and their configuration files in `mods`.
4. Set the correct launch options for your mod.
5. Start the game and review the configuration generated on the first run.

If the game fails during startup, check the configured DLL load order. DLLs listed as immediate loads are loaded before the remaining patches.

## Build configurations

The solution targets 32-bit Retribution and provides two relevant configurations:

- `Release|x86` uses the fixed offsets for the supported game build.
- `SigScan|x86` locates active patch sites at runtime with byte signatures.

SigScan DLLs are written to the `SigScan` output directory. A SigScan plugin aborts its initialization if a required signature is missing, ambiguous, or fails its original-byte validation instead of patching an uncertain address.

The current signatures were developed against this `DOW2.exe`:

```text
SHA-256: 4CE6423D3D97528A784EDE2A16BB9B671AAC221FDDE1FDDEE5BD060CFD38A50E
```

Build from Visual Studio or with MSBuild:

```powershell
msbuild DOW2DLLInjector.sln /p:Configuration=Release /p:Platform=x86
msbuild DOW2DLLInjector.sln /p:Configuration=SigScan /p:Platform=x86
```

## Profiler plugin API

Injected patch DLLs import the shared helpers from `Profiler.dll`:

| Ordinal | Export | Purpose |
|---:|---|---|
| 100 | `MemPatch` | Write bytes into a loaded module image |
| 101 | `NopPatch` | Replace a region with x86 NOP instructions |
| 102 | `JmpPatch` | Install a relative x86 jump |
| 103 | `SigScan` | Search a module image using a byte pattern and mask |

The patch helpers validate that the complete destination range belongs to a committed, loaded module image before changing memory protection or writing bytes.

## Gamemode Patch

- The release includes an example configuration.
- `GamemodePatch.dll` must be listed as an immediate load.
- The configuration file must be named after the module and stored in `mods`.
- Example member layout:

  ```text
  (index): ffa: false; tffa: true; list: default;
  (index2): ffa: true; tffa: false; list: glorb;
  ```

- `ffa` and `tffa` cannot both be enabled for one entry.
- Entries `0` through `3` in the example configuration are built-in game modes and should normally remain unchanged.
- `list` selects the map list used by that game mode. `default` uses the normal PvP or FFA list.
- Custom map-list folders belong under `Data/maps/<folder>`.
- Incorrect map-list names can cause missing maps or crashes.

The current patch expands the game's map-list storage and redirects the original loading, cleanup, and finalization flow into the expanded allocation.

## Cullsphere Patch

- Put the DLL in `mods`.
- The example `.cullsphere` file contains the distance cutoff on the first line and the growth rate on the second line.
- The patch dynamically increases the cullsphere up to zoom level 800, then reduces it because fog prevents useful visibility beyond that distance.
- Large cullsphere values can reduce performance.

## Shell Patch

1. Put the DLL in `mods`.
2. Edit `selection_panel.gfx` and `selection_panel.lua` to expose the added shells.
3. Add the shell images to your module:

   ![Shell image location 1](https://github.com/Bruiserjoe/DOW2Injector/blob/main/Capture1.PNG?raw=true)
   ![Shell image location 2](https://github.com/Bruiserjoe/DOW2Injector/blob/main/Capture2.PNG?raw=true)

4. Add an image import for each shell. Cloning and editing an existing import is usually easiest:

   ![Shell image import](https://github.com/Bruiserjoe/DOW2Injector/blob/main/Capture3.PNG?raw=true)

5. Add the corresponding `DefineShape` and `DefineSprite` entries:

   ![Shell shape and sprite](https://github.com/Bruiserjoe/DOW2Injector/blob/main/Capture4.PNG?raw=true)

6. Add the shell's `PlaceObject` to `waaagh_mc`. The expected ID is normally `366`; the example image uses `437`:

   ![Shell PlaceObject](https://github.com/Bruiserjoe/DOW2Injector/blob/main/Capture5.PNG?raw=true)

7. Edit `<module_name>.shells` in the main Retribution directory using the example format. Do not include spaces.

## Eight Player FFA Patch

- Enables eight-player maps in FFA game modes.
- Eight-player solo FFA slots can be inconsistent. Create the slots in Team FFA, then return to Solo FFA if they are not populated correctly.

## 8p Replay Fix

Adjusts replay player-slot indexing so replays with more than six players can be processed correctly.

## UI Expansion

Expands active UI limits for abilities, global abilities, mini portraits, the build queue, and observer-panel player entries.

## Window Patch
- `WindowPatch.dll` must be listed as an immediate load.
- Implements borderless fullscreen without changing the monitor's display mode.
- Keeps the Platform and renderer client dimensions synchronized.
- Prevents the windowed renderer path from forcing incompatible fullscreen presentation values.

## Misc Fixes

- Removes the race statistics dropdown in skirmish games to prevent crashes with custom races.
- Includes the scaling fix originally written by [RipleyTom](https://github.com/RipleyTom/rustpatch).
