# Godot profile after opening a project

Date: 2026-08-28

## User

Let's ignore Unity and just focus on Godot for now.

I've done the following:

- Open a project in Godot 4.6.3 editor.
- Observed that the G-SYNC indicator in the top right was active.
- Observed that mouse movement was choppy while moving in the Godot editor, but when I pressed the Windows key and moved the mouse around in the Start menu, the mouse movement was smooth.

What is the current state of the custom profile for Godot, if any?

## Assistant

A custom profile now exists. Opening the project changed the live DRS state from the earlier no-profile baseline.

Current profile:

- Name: `Godot Engine`
- User-defined: yes (`predefined=0`)
- Application: `godot_v4.6.3-stable_win64.exe`
- Match scope: executable basename only; no folder, launcher, or command-line qualifier
- Explicit settings: two

The settings are:

- `0x1194F158 = 1`: G-SYNC mode — fullscreen only
- `0x20C1221E = 2`: OpenGL threaded optimization — disabled

The critical detail is what is missing: the profile has no per-application G-SYNC override (`0x10A879CF`). That value is inherited from the global profile as `0` (`allow`). Therefore the current Godot-created profile is not equivalent to a Control Panel **Fixed Refresh** override. Your active G-SYNC indicator shows that its two settings are not preventing G-SYNC in this windowed editor session.

This profile was created by Godot during the recent launch, not by NVIDIA App:

- The total DRS profile count increased from 7,957 to 7,958.
- The DRS database changed at 12:49:02.
- NVIDIA App's application storage is unchanged since 12:09:47 and still has no Godot entry.
- NVIDIA App began classifying the executable through the new `Godot Engine` DRS profile at 12:49:23.
- Godot 4.6.3's OpenGL code writes exactly this profile name, application association, and these two settings.

The full snapshot is in `godot-after-project-launch-findings.md`; the raw DRS output is in `godot-after-project-launch-query-output.txt`. No NVIDIA setting was changed.
