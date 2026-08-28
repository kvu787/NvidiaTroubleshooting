# Godot profile state after deletion in NVIDIA Profile Inspector

Snapshot taken on 2026-08-28 at 13:25 PDT.

Target:

`C:\Users\k\Program\Godot_v4.6.3-stable_win64.exe\Godot_v4.6.3-stable_win64.exe`

## Result

There is no remaining custom NVIDIA DRS profile or application association for the Godot editor executable.

The read-only NVAPI checks returned:

- Full-path application lookup: `-166` (`NVAPI_EXECUTABLE_NOT_FOUND`)
- Basename application lookup: `-166` (`NVAPI_EXECUTABLE_NOT_FOUND`)
- Profile lookup for `Godot_v4.6.3-stable_win64.exe`: `-163` (`NVAPI_PROFILE_NOT_FOUND`)
- Profile lookup for `Godot_v4.6.3-stable_win64`: `-163` (`NVAPI_PROFILE_NOT_FOUND`)
- Profile scan: no target-related profile
- Total DRS profile count: 7,957

The prior state contained the user-defined `Godot Engine` profile and 7,958 total profiles. The one-profile decrease, failed application lookups, failed profile-name lookups, and full scan agree that the profile and executable association were deleted.

## NVIDIA App cross-check

`C:\Users\k\AppData\Local\NVIDIA Corporation\NVIDIA App\NvBackend\ApplicationStorage.json` contains no Godot entry. It remains 2,766 bytes and was last modified at 12:09:47 PDT.

Therefore there is currently neither:

- a Godot application profile in the live NVIDIA DRS database, nor
- a separate Godot program entry in NVIDIA App's application catalog.

## Change timestamp

The active DRS database selection and `nvdrsdb1.bin` changed at 13:24:27 PDT, consistent with the NPI deletion being applied.

## Safety

Verification was read-only. No NVIDIA setting was changed during this snapshot.

Raw output: [`godot-after-npi-delete-query-output.txt`](godot-after-npi-delete-query-output.txt)
