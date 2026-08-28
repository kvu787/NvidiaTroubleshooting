# Godot NVIDIA App profile: pre-launch checkpoint

Captured on 2026-08-28 after NVIDIA App was used to add the Godot executable and set **Monitor Technology** to **Fixed Refresh**, after NVIDIA App was closed, and before Godot was launched.

## Result

NVIDIA App successfully created and persisted a user-defined driver profile for the exact Godot executable. The live NVIDIA DRS database contains the intended Fixed Refresh override. This checkpoint isolates NVIDIA App's write from any profile modification that Godot may perform during startup.

Godot and NVIDIA App were both not running when the state was queried.

## Driver profile

- Profile name: `Godot_v4.6.3-stable_win64.exe`
- User-defined: yes (`predefined=0`)
- Application association: `c:/users/k/program/godot_v4.6.3-stable_win64.exe/godot_v4.6.3-stable_win64.exe`
- Association scope: exact full path; a basename-only lookup does not match
- Application count: 1
- Explicit setting count: 7
- Total DRS profile count: 7,958, up from the clean baseline of 7,957

The decisive settings are:

- `0x10A879CF = 4`: G-SYNC application override = Fixed Refresh
- `0x1094F1F7 = 0`: VRR requested state = disabled
- `0x1194F158 = 0`: G-SYNC mode setting = disabled

The other four explicitly stored settings are Vertical Sync Tear Control, Preferred Refresh Rate, Vertical Sync, and Smooth AFR Behavior. The complete decoded output is in `godot-nvapp-prelaunch-query-output.txt`.

## NVIDIA App state and transaction history

`ApplicationStorage.json` now contains a manual application with:

- Local ID: `963528738`
- Display name: `Godot_v4.6.3-stable_win64.exe`
- Exact detected/image path: `C:\Users\k\Program\Godot_v4.6.3-stable_win64.exe\Godot_v4.6.3-stable_win64.exe`
- `IsManuallyAdded=true`
- Initial time: `2026-08-28T20:28:36Z`
- No launch recorded yet: `LastLaunchTimeISO=1601-01-01T00:00:00Z`

The preserved NVIDIA App logs show this sequence:

1. 13:28:36.787: manual application addition succeeded.
2. 13:28:39.130: creation of the new DRS profile succeeded.
3. 13:28:50.222: NVIDIA App requested Monitor Technology value `4` for the new profile.
4. 13:28:50.241: the setting write returned success.
5. 13:28:52.169: the subsequent profile refresh returned success.

This matches the supplied screenshot and the independent DRS query. The screenshot is preserved at `screenshots/godot-profile-created-nvapp-prelaunch.png`, the exact application catalog at `godot-nvapp-prelaunch-application-storage.json`, and the selected log evidence at `godot-nvapp-prelaunch-log-excerpt.txt`.

## File provenance

The NVIDIA App catalog changed at 13:28:36.783, the first DRS database file changed at 13:28:39.126, and the active DRS database plus selector changed at 13:28:50.238. These timestamps align with the catalog add, profile create, and Fixed Refresh write in the logs.

Lengths, timestamps, and SHA-256 hashes are preserved in `godot-nvapp-prelaunch-file-state.txt`. The DRS binary files themselves were not copied because the read-only NVAPI output records the relevant logical state without preserving unrelated driver profiles.

## Next comparison

After Godot is opened, observed, and closed, rerun the same DRS query and compare profile name, application association, and explicit settings against this checkpoint. In particular, check whether Godot retains this exact full-path profile and Fixed Refresh override, changes it, or creates a separate `Godot Engine` profile.
