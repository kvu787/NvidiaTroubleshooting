# Current NVIDIA profile state for Godot 4.6.3

Snapshot taken on 2026-08-28 at approximately 12:31 PDT.

Target:

`C:\Users\k\Program\Godot_v4.6.3-stable_win64.exe\Godot_v4.6.3-stable_win64.exe`

## Result

There is no current NVIDIA custom program-profile configuration associated with the target executable.

More specifically:

- The live NVIDIA Driver Settings (DRS) database has no application association for either the full executable path or the executable basename.
- Neither of the two historically relevant profile names, `Godot_v4.6.3-stable_win64.exe` and `Godot Engine`, currently exists.
- A scan of all 7,957 profile records found no profile whose name contains `Godot`.
- NVIDIA App's current `NvBackend\ApplicationStorage.json` has no manual Godot application entry. It currently contains only Age of Empires IV and Steam.
- NVIDIA-related registry searches found no Godot entry.

Consequently, the target currently inherits global NVIDIA driver settings. There is no per-program Fixed Refresh/G-SYNC override stored for it.

## Direct DRS query

The read-only utility in `drs-query.cpp` loads `nvapi64.dll` and calls only NVIDIA query functions. It does not resolve or call any create, set, save, or delete function.

The decisive calls and current results were:

| Query | Status | Meaning |
| --- | ---: | --- |
| `NvAPI_DRS_FindApplicationByName` with full path | -166 | `NVAPI_EXECUTABLE_NOT_FOUND` |
| `NvAPI_DRS_FindApplicationByName` with basename | -166 | `NVAPI_EXECUTABLE_NOT_FOUND` |
| `NvAPI_DRS_FindProfileByName("Godot_v4.6.3-stable_win64.exe")` | -163 | `NVAPI_PROFILE_NOT_FOUND` |
| `NvAPI_DRS_FindProfileByName("Godot Engine")` | -163 | `NVAPI_PROFILE_NOT_FOUND` |
| Profile enumeration | 0 | Success; no profile name contained `Godot` |

The raw output is preserved in `query-output.txt`. The status meanings were cross-checked against the local NVAPI header at `C:\Users\k\Repository\External\PresentMon_2-5-1\IntelPresentMon\ControlLib\nvapi.h`, lines 1130-1134.

## Relevant history in NVIDIA App logs

This is history, not the current configuration, but it exposes an important conflict:

- At 12:09:33, NVIDIA App tried to create a DRS application association for the exact target path. `NvAPI_DRS_CreateApplication` failed with `-167`, meaning `NVAPI_EXECUTABLE_ALREADY_IN_USE`.
- During the same period, NVIDIA App's backend repeatedly resolved the target path to a profile named `Godot Engine`.
- At 12:09:44, NVIDIA App tried to create a profile named `Godot_v4.6.3-stable_win64.exe`. `NvAPI_DRS_CreateProfile` failed with `-164`, meaning `NVAPI_PROFILE_NAME_IN_USE`.
- At 12:09:47, NVIDIA App logged successful deletion of its manual Godot application entry, ID `963528738`.
- The current DRS query returns `-163` for both historical profile names and the current application storage no longer contains Godot.

The relevant source log locations are:

- `C:\Users\k\AppData\Local\NVIDIA Corporation\NVIDIA App\CxNative_NVIDIA App.1.log`, lines 486-487, 661-662, and 703-704.
- `C:\Users\k\AppData\Local\NVIDIA Corporation\NVIDIA App\NvBackend\backend.log`, including lines 8375, 8496, 8710, 8997, 9495, 9819, 10306, and 10853.

## Godot itself can create the conflicting profile

The local Godot 4.6.3 source contains its own NVIDIA DRS setup in `C:\Users\k\Repository\External\Godot_4-6-3\platform\windows\gl_manager_windows_native.cpp`:

- The OpenGL manager calls `_nvapi_setup_profile()` during initialization at line 506.
- The profile name comes from `application/config/name`, falling back to the engine name, at lines 168-174.
- The application association uses only the executable basename at lines 168 and 224-234.
- It explicitly sets OpenGL threaded optimization (`0x20C1221E`) to disabled and G-SYNC mode (`0x1194F158`) to fullscreen-only at lines 239-264.

This makes a Godot-created `Godot Engine` profile the leading explanation for NVIDIA App's earlier `-167` executable conflict. It is not present in the current snapshot, so the explanation remains about the earlier failed creation attempt rather than a current active profile.
