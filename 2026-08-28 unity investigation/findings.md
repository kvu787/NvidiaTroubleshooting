# NVIDIA settings related to Unity Technologies applications

Snapshot: 2026-08-28 14:28 PDT (2026-08-28T21:28Z)

No NVIDIA, Windows graphics, Unity, display, or application setting was changed during this investigation.

## Scope

The requested company scope was interpreted as Unity Software/Unity Technologies' own installed software: Unity Editor, Unity Hub, and the executables shipped with them. Locally built Unity players found under `C:\Users\k\Repository\Unity` were checked separately because NVIDIA's logs referenced one of them. A full DRS database string scan was also performed to expose profiles that merely contain the word `Unity` or a `UnityPlayer.dll` rule; those unrelated game/content profiles are separated below.

The installed Unity products are:

- Unity Editor 6000.3.21f1
- Unity Editor 6000.3.22f1
- Unity Editor 6000.3.23f1
- Unity Hub 3.21.0

The executable inventory contains 93 Unity-supplied paths and four local player-build paths. The complete inventory and version metadata are in [`unity-executables.tsv`](unity-executables.tsv).

The graphics system is an NVIDIA GeForce RTX 5070 Ti Laptop GPU with driver 616.56, plus Intel integrated graphics. NVIDIA App is 11.0.8.299 and NVIDIA Control Panel is 8.1.969.0. No Unity process was running at the snapshot time.

## Bottom line

There is one live NVIDIA application profile that applies to Unity Technologies software: NVIDIA's predefined `Unity 3D` profile.

- It is not a custom profile.
- Its sole application rule is the basename `unity.exe`.
- The rule has no directory, launcher, or command-line qualifier.
- It therefore matches all three installed Unity Editor executables, Unity Hub's CLI `unity.exe`, and `C:\Users\k\AppData\Local\Unity\bin\unity.exe`.
- It does not match `Unity Hub.exe` or the other Unity helper/build/licensing executables.
- All eight explicit settings in the profile are NVIDIA-predefined values. There is no user override in the Unity profile.
- There is no Unity-specific G-SYNC/VRR, V-Sync, refresh-rate, power, frame-limit, low-latency, DLSS, or GPU-selection override.
- Unity inherits nine user-set values from the global/base NVIDIA profile. The inherited values include Prefer maximum performance, Highest available refresh rate, and the machine's global fullscreen-only G-SYNC configuration.

The read-only query checked 875 known setting IDs collected from NVIDIA Profile Inspector's built-in constants and local reference metadata. It returned 16 known configured values for `Unity 3D`: seven explicit Unity-profile values and nine inherited global values. The eighth explicit Unity-profile value is an undocumented ID not present in the reference metadata. Thus there are 17 stored/effective configured entries in total: eight per-profile plus nine global.

## Exact `Unity 3D` profile

Application rule:

| Field | Current value |
| --- | --- |
| Profile name | `Unity 3D` |
| Profile predefined | Yes |
| Application | `unity.exe` |
| Friendly name | `Unity3D` |
| Application predefined | Yes |
| Folder constraint | None |
| Launcher constraint | None |
| Command-line constraint | None |

The following are all eight explicit values in the profile. For every row, NVAPI reported `current_predefined=1`, `predefined_valid=1`, and a current value equal to the NVIDIA predefined value.

| Setting ID | Setting | Value | Interpretation |
| --- | --- | ---: | --- |
| `0x106D5CFF` | Do not display this profile in Control Panel | `0` | Profile is allowed to be displayed |
| `0x10F9DC81` | Enable application for Optimus | `0x11` | NVIDIA's predefined Optimus allow-list/shim value; it is not a per-path forced-GPU rule |
| `0x205F7E3B` | OpenGL App Claw | `0` | Workstation CLAW path disabled |
| `0x20A3D20D` | Undocumented OpenGL setting | `0` | No public/local description |
| `0x80303A19` | `rxinput.dll` injection | `0` | Disabled |
| `0x80857A28` | Vertical Sync Expr Behaviors | `1` | NVIDIA reference metadata describes this value as usage-statistics collection |
| `0x809D5F60` | NVIDIA App Overlay flags | `1` | Bit 0 set: desktop-capture/overlay allow-list flag |
| `0xB0CC0875` | Undocumented | `0` | Not decoded by the local reference metadata |

None of these eight is a G-SYNC application override.

## Global values inherited by Unity

The `Unity 3D` effective query returned these nine values from `location=global profile`, with `current_predefined=0`. They are global user values, not Unity-specific changes.

| Setting ID | Setting | Raw value | Decoded current state |
| --- | --- | ---: | --- |
| `0x005A375C` | Vertical Sync Tear Control | `0x96861077` | Disabled/standard behavior |
| `0x0064B541` | Preferred refresh rate | `1` | Highest available |
| `0x00A879CF` | Vertical Sync | `0x60925292` | Passive / use the 3D application's setting |
| `0x101AE763` | Smooth AFR behavior | `0` | Off |
| `0x1057EB71` | Power management mode | `1` | Prefer maximum performance |
| `0x1094F157` | Toggle the VRR global feature | `1` | Enabled |
| `0x1094F1F7` | VRR requested state | `1` | Fullscreen only |
| `0x10A879CF` | G-SYNC application override | `0` | Allow; not force-off, disallow, or fixed refresh |
| `0x1194F158` | Enable G-SYNC globally | `1` | Fullscreen only |

The practical G-SYNC result is that Unity has no application override and inherits the global fullscreen-only mode. There is no current Unity-specific Fixed Refresh rule.

The base profile reports `numOfSettings=10`, while the public `NvAPI_DRS_EnumSettings` call returns nine. The report preserves that driver/API discrepancy rather than claiming knowledge of an entry the public enumeration did not expose.

## Coverage of installed executables

Five full paths matched `Unity 3D` through its basename rule:

- `C:\Program Files\Unity\Hub\Editor\6000.3.21f1\Editor\Unity.exe`
- `C:\Program Files\Unity\Hub\Editor\6000.3.22f1\Editor\Unity.exe`
- `C:\Program Files\Unity\Hub\Editor\6000.3.23f1\Editor\Unity.exe`
- `C:\Program Files\Unity Hub\resources\cli\unity.exe`
- `C:\Users\k\AppData\Local\Unity\bin\unity.exe`

The remaining 92 inventoried executable paths returned `NVAPI_EXECUTABLE_NOT_FOUND` (`-166`) for both full-path and basename lookup. This includes `Unity Hub.exe`, licensing clients, package managers, crash handlers, shader/compiler/build helpers, playback templates, and both local `ZoomTracks.exe` builds.

Exact profile-name lookups derived from every inventoried filename also found no custom/orphan profile. In particular:

- `Unity`, `Unity.exe`, `Unity Hub`, and `Unity Hub.exe`: `NVAPI_PROFILE_NOT_FOUND` (`-163`)
- `ZoomTracks` and `ZoomTracks.exe`: `NVAPI_PROFILE_NOT_FOUND` (`-163`)

The current state therefore contains no Unity Technologies custom profile and no current local `ZoomTracks` profile.

## NVIDIA App state

NVIDIA App's current [`ApplicationStorage.json`](nvidia-app-application-storage.json) contains three entries: Age of Empires IV, Godot, and Steam. It contains no Unity Editor, Unity Hub, Unity helper, or `ZoomTracks` entry. Therefore no NVIDIA App manual/detected application record or app-scoped NVIDIA App setting is current for Unity.

NVIDIA App does ship a `unity_editor` fingerprint. The local metadata is preserved in [`nvidia-app-unity-editor-fingerprint.xml`](nvidia-app-unity-editor-fingerprint.xml). It:

- identifies `unity.exe` as a creative application and uses the driver profile `unity.exe`;
- supports only versions matching `2019.x.x.x` and `2018.x.x.x`;
- marks Frame Generation, Super Resolution, Ray Reconstruction, and RR/SR model overrides disabled for those supported catalog versions.

Those metadata flags are not active settings for the installed Unity 6 editors because none of them is in current `ApplicationStorage.json`. NVIDIA App's scanner log shows all three installed 6000.3 editor versions being considered for `unity_editor` and rejected because their file versions do not match the `fv-2019` or `fv-2018` rules. At runtime, the backend falls back to DRS classification `Unity 3D` for telemetry.

The same log previously queried the local `ZoomTracks.exe` path and reported `useGlobal=true` for NVIDIA App DLSS/Frame Generation information, but it repeatedly said the application was absent from both detected and manual apps. Direct current DRS lookup now confirms there is no executable association or profile named `ZoomTracks`/`ZoomTracks.exe`.

No Unity/ZoomTracks `NvAPI_DRS_Create*`, `Set*`, `Save*`, or `Delete*` mutation event appears in the current backend log.

## Overlay / ShadowPlay state

The current DRS profile's overlay flag is predefined value `1`, whose documented bit 0 allows desktop capture/overlay treatment.

The most recent ShadowPlay runtime records for `Unity.exe` compute:

- `isGame=1`
- `isLauncher=1`
- `isAppBlackListed=0`
- `isEnableHook=0`
- `isFSDEnabled=0`
- `isMouseClipEnabled=0`
- `isDx9FOEnabled=0`
- `isCAEnabled=0`
- `m_bDX12ScreenshotWAR=0`
- `m_bShowHotkeyMessageDisabled=0`

These are historical runtime records from the latest Unity launches; no Unity process was running at the final snapshot. Unity Hub generated ShadowPlay status queries, but no corresponding custom settings record was found. The relevant lines are preserved in [`shadowplay-unity-log-lines.txt`](shadowplay-unity-log-lines.txt).

## Windows and registry cross-checks

- Windows `HKCU\Software\Microsoft\DirectX\UserGpuPreferences` has no Unity or `ZoomTracks` entry, so Windows does not currently force an integrated/high-performance GPU preference for these paths.
- Recursive Unity/ZoomTracks searches under NVIDIA's HKCU, HKLM, and WOW6432Node registry trees returned no match.
- The machine has no explicit `HwSchMode` registry value. NVIDIA App's runtime log separately reports hardware scheduling as detected when it evaluates Frame Generation support.

## Broad DRS scan: matches that are not Unity Technologies applications

The all-profile scan examined 7,959 live DRS profiles and matched 16 profile records by `unity` text in a profile/application field. Only `Unity 3D` targets Unity Technologies software installed here. The other 15 are NVIDIA-predefined game, Unity Web Player/content, or DLL-association profiles:

- Assassin's Creed: Unity
- `island.unity3d`
- Super Hero Squad
- `3rdpersonshooter.unity3d`
- F1 2022
- F1 2012
- Invoke AI
- `webplayer.unity3d`
- `flylikeabird3.unity3d`
- `AngryBots.unity3d`
- Mechdyne getReal3D
- XIII - Remake
- F1 25
- F1 24
- F1 23

All 15 are predefined; none is evidence of an active Unity Editor/Hub custom setting. Their full application fields and settings remain in [`drs-audit.txt`](drs-audit.txt) so the broad scan is auditable.

## Method and reproducibility

[`unity-drs-audit.cpp`](unity-drs-audit.cpp) loads only the following NVIDIA query/session functions:

- `NvAPI_Initialize` / `NvAPI_Unload`
- `NvAPI_DRS_CreateSession` / `NvAPI_DRS_DestroySession` (an in-memory read session)
- `NvAPI_DRS_LoadSettings`
- `NvAPI_DRS_GetBaseProfile`
- `NvAPI_DRS_FindApplicationByName`
- `NvAPI_DRS_FindProfileByName`
- `NvAPI_DRS_GetNumProfiles`
- `NvAPI_DRS_EnumProfiles`
- `NvAPI_DRS_GetProfileInfo`
- `NvAPI_DRS_EnumApplications`
- `NvAPI_DRS_EnumSettings`
- `NvAPI_DRS_GetSetting`

It does not resolve or call any create-profile, create-application, set, save, delete, restore, or import function. [`collect-machine-state.ps1`](collect-machine-state.ps1) collects file metadata, installed/running state, read-only registry output, NVIDIA App catalog/log excerpts, ShadowPlay log excerpts, and invokes the query utility. [`Build.cmd`](Build.cmd) reproduces the local build.

Primary evidence:

- [`drs-audit.txt`](drs-audit.txt): raw DRS target lookups, exact profile-name lookups, all broad matches, all explicit Unity values, and effective known settings
- [`machine-state.txt`](machine-state.txt): hardware, driver/application versions, installed Unity products, process state, DRS file metadata, and Windows GPU preferences
- [`nvidia-app-application-storage.json`](nvidia-app-application-storage.json): exact current NVIDIA App application catalog
- [`nvidia-app-unity-log-lines.txt`](nvidia-app-unity-log-lines.txt): Unity/ZoomTracks NVIDIA App backend lines
- [`nvidia-app-unity-editor-fingerprint.xml`](nvidia-app-unity-editor-fingerprint.xml): local catalog definition for Unity Editor
- [`shadowplay-unity-log-lines.txt`](shadowplay-unity-log-lines.txt): Unity/Hub overlay runtime classification
- [`registry-unity-search.txt`](registry-unity-search.txt): negative registry cross-check
- [`setting-reference.tsv`](setting-reference.tsv) and [`npi-assembly-settings.txt`](npi-assembly-settings.txt): setting ID/value decoding inputs
- [`artifact-manifest.tsv`](artifact-manifest.tsv): sizes, timestamps, and SHA-256 hashes for investigation files
