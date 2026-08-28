# Current Godot NVIDIA profile state

Date: 2026-08-28

## User

I'm observing an issue with creating custom program profiles with the NVIDIA App. I have G-SYNC enabled currently, but it is activating on the Godot editor when it shouldn't be. To fix that, I need to create a custom profile that disables G-SYNC for the Godot editor executable. However, this workaround only seems to work when the custom profile is created in the no-longer-supported NVIDIA Control Panel. Creating a custom profile in NVIDIA App doesn't seem to work.

We're going to investigate this. Do all of your work in the `2026-08-28 gsync investigation` subfolder.

First, tell me what kind of NVIDIA custom profile stuff is currently configured for `C:\Users\k\Program\Godot_v4.6.3-stable_win64.exe\Godot_v4.6.3-stable_win64.exe` (if any) and tell me how you determined that info.

## Assistant

Currently, none. At the approximately 12:31 PDT snapshot, the target executable has no NVIDIA DRS application association and no active custom program profile. Therefore it inherits the global NVIDIA settings; there is no per-program Fixed Refresh/G-SYNC override stored for it.

The direct checks were:

- `NvAPI_DRS_FindApplicationByName` returned `-166` (`NVAPI_EXECUTABLE_NOT_FOUND`) for both the full path and the normal DRS basename, `Godot_v4.6.3-stable_win64.exe`.
- `NvAPI_DRS_FindProfileByName` returned `-163` (`NVAPI_PROFILE_NOT_FOUND`) for both historically relevant names: `Godot_v4.6.3-stable_win64.exe` and `Godot Engine`.
- Enumerating all 7,957 DRS profiles found no profile name containing `Godot`.
- NVIDIA App's current `NvBackend\ApplicationStorage.json` contains no manual Godot entry.
- NVIDIA-related registry searches found no Godot entry.

I determined this with a small read-only utility that loads `nvapi64.dll` and uses NVIDIA's DRS query functions. It does not load any create, set, save, or delete entry points. I cross-checked the result against NVIDIA App's current application storage, its logs, and NVIDIA registry keys. No NVIDIA setting was changed.

The logs do preserve useful earlier history: NVIDIA App's attempt to create this association failed with `-167` (`NVAPI_EXECUTABLE_ALREADY_IN_USE`), its backend mapped the executable to `Godot Engine`, a later profile-creation attempt failed with `-164` (`NVAPI_PROFILE_NAME_IN_USE`), and the manual Godot entry was deleted successfully at 12:09:47. Those are earlier failed/removed records, not current active configuration.

Godot 4.6.3's own OpenGL initialization code can create a `Godot Engine` DRS profile, associate the executable basename, disable OpenGL threaded optimization, and set G-SYNC mode to fullscreen-only. That is the leading explanation for the earlier NVIDIA App collision, but the profile is absent now.

The evidence and reproducible query are stored in `findings.md`, `query-output.txt`, and `drs-query.cpp` in the investigation folder.
