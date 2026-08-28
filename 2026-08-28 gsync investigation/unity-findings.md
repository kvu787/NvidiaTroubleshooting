# Current NVIDIA profile state for Unity 6000.3.22f1

Snapshot taken on 2026-08-28 at approximately 12:43 PDT.

Target:

`C:\Program Files\Unity\Hub\Editor\6000.3.22f1\Editor\Unity.exe`

## Result

The target is currently associated with NVIDIA's predefined `Unity 3D` driver profile. It does not have a custom user-created profile or any user-modified per-program setting.

The application entry is:

- `appName`: `unity.exe`
- Friendly name: `Unity3D`
- Predefined: yes
- `fileInFolder`: empty
- Launcher and command-line qualifiers: empty

This is a basename-only association. It is not specific to Unity 6000.3.22f1 or to the requested installation folder; it can match any executable named `unity.exe`.

## Profile settings

The profile has eight settings. Every one is marked `location=current profile`, `current_predefined=1`, and `predefined_valid=1`. That combination shows they are NVIDIA-supplied settings in the predefined profile, not user overrides.

| Setting ID | NVIDIA/Profile Inspector metadata | Value |
| --- | --- | ---: |
| `0x106D5CFF` | Do not display this profile in Control Panel | `0` (disabled) |
| `0x10F9DC81` | Enable application for Optimus | `0x11` |
| `0x205F7E3B` | OpenGL App CLAW/workstation performance code | `0` (disabled) |
| `0x20A3D20D` | Unnamed OpenGL setting | `0` |
| `0x80303A19` | `rxinput.dll` injection | `0` (disabled) |
| `0x80857A28` | Vertical Sync Expr Behaviors | `1` (Mode 1) |
| `0x809D5F60` | NVIDIA App Overlay flags | `1` |
| `0xB0CC0875` | Undocumented by the local reference metadata | `0` |

None of these eight is the per-application G-SYNC override.

## Effective G-SYNC-related settings

The relevant G-SYNC settings returned for `Unity 3D` all come from `location=global profile`, not from the Unity profile:

| Setting | ID | Effective value | Source |
| --- | --- | --- | --- |
| VRR requested state | `0x1094F1F7` | `1` — fullscreen only | Global profile |
| G-SYNC application override | `0x10A879CF` | `0` — allow | Global profile |
| G-SYNC mode | `0x1194F158` | `1` — fullscreen only | Global profile |

There is therefore no Unity-specific Fixed Refresh, force-off, or other G-SYNC override currently stored.

## NVIDIA App cross-check

NVIDIA App's current `NvBackend\ApplicationStorage.json` contains no Unity entry, manual or detected. It currently contains only Age of Empires IV and Steam.

The backend log shows that NVIDIA App's scanner does see the exact Unity 6000.3.22f1 path, but repeatedly rejects it because its file version differs from the scanner metadata labeled `fv-2019` and `fv-2018`. Relevant current log entries include lines 11620-11637 in:

`C:\Users\k\AppData\Local\NVIDIA Corporation\NVIDIA App\NvBackend\backend.log`

No NVIDIA App DRS profile-creation or setting-write event for this Unity editor path was found. NVIDIA-related registry searches also found no Unity entry.

## Method

The `drs-query.cpp` utility loads `nvapi64.dll` and calls only read operations:

- `NvAPI_DRS_FindApplicationByName`
- `NvAPI_DRS_GetProfileInfo`
- `NvAPI_DRS_EnumApplications`
- `NvAPI_DRS_EnumSettings`
- `NvAPI_DRS_GetSetting`
- Profile-name enumeration for cross-checking

The raw result is preserved in `unity-query-output.txt`. No NVIDIA setting was changed.
