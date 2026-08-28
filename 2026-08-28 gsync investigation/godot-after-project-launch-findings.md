# Godot profile state after opening a project

Snapshot taken on 2026-08-28 at approximately 12:52 PDT.

Target:

`C:\Users\k\Program\Godot_v4.6.3-stable_win64.exe\Godot_v4.6.3-stable_win64.exe`

## Runtime observation supplied by the user

- A project was opened in the Godot 4.6.3 editor.
- The NVIDIA G-SYNC indicator was active in the editor.
- Mouse movement in the editor was choppy.
- Pressing the Windows key and moving the mouse over the Start menu produced smooth movement.

## Current profile state

A user-defined NVIDIA DRS profile now exists:

- Profile name: `Godot Engine`
- Profile type: user-defined (`predefined=0`)
- Application: `godot_v4.6.3-stable_win64.exe`
- Application type: user-defined (`predefined=0`)
- Folder qualifier: none
- Launcher or command-line qualifier: none

The application match is based on the executable basename, not the requested full path.

The profile contains exactly two explicit settings:

| Setting | ID | Value |
| --- | --- | --- |
| G-SYNC mode | `0x1194F158` | `1` — fullscreen only |
| OpenGL threaded optimization | `0x20C1221E` | `2` — disabled |

Both are current-profile, non-predefined values.

## Important G-SYNC distinction

The `Godot Engine` profile does not contain the per-application G-SYNC override (`0x10A879CF`). Its effective value is inherited from the global profile as `0` (`allow`).

Therefore this profile is not equivalent to setting **Monitor Technology / G-SYNC = Fixed Refresh** for Godot. It requests the `fullscreen only` G-SYNC mode through `0x1194F158`, but it leaves the application override at global `allow`. The user's active G-SYNC indicator demonstrates that this two-setting profile is not preventing G-SYNC in the current windowed editor session.

## Change from the earlier baseline

The earlier snapshot had:

- No application association for the Godot executable.
- No `Godot Engine` profile.
- 7,957 total DRS profiles.

The new snapshot has:

- A successful application match to `Godot Engine`.
- The two explicit settings listed above.
- 7,958 total DRS profiles.

## Attribution

The profile was created by Godot during the recent project launch:

- The active DRS database file changed at 12:49:02.
- NVIDIA App's `ApplicationStorage.json` has been unchanged since 12:09:47 and still has no Godot entry.
- At 12:49:23, NVIDIA App's backend began classifying the executable through the DRS profile `Godot Engine`.
- The local Godot 4.6.3 OpenGL code creates a profile using the application name, associates the executable basename, and writes exactly these two setting IDs and values.

This is direct agreement between the before/after DRS state, timestamps, NVIDIA App logs, and Godot source code.

## Method

The snapshot was taken through read-only NVAPI DRS calls using `drs-query.cpp`. No NVIDIA setting was changed. The raw result is in `godot-after-project-launch-query-output.txt`.
