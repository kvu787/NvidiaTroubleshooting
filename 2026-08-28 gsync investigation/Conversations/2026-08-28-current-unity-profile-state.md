# Current Unity NVIDIA profile state

Date: 2026-08-28

## User

Unity engine also has the same issue as Godot engine. Check if there's any custom profile stuff for `C:\Program Files\Unity\Hub\Editor\6000.3.22f1\Editor\Unity.exe` currently.

## Assistant

There is an active NVIDIA profile for `Unity.exe`, but it is not custom: the executable is associated with NVIDIA's predefined `Unity 3D` profile.

The important details are:

- The application match is `unity.exe` with no folder, launcher, or command-line qualifier. It is basename-wide rather than specific to Unity 6000.3.22f1, so it can match any executable named `unity.exe`.
- The profile contains eight settings. Every setting has `current_predefined=1`, so all eight are NVIDIA-supplied predefined values; there are no user-modified Unity-profile settings.
- None of the eight profile settings is a G-SYNC override.
- The effective G-SYNC application override is inherited from the global profile and is `0` (`allow`). The global VRR requested state and G-SYNC mode both currently report `1` (`fullscreen only`).
- Therefore, there is no Unity-specific Fixed Refresh, force-off, or other G-SYNC override stored right now.

NVIDIA App's own current application storage has no Unity entry, manual or detected. Its backend log shows that the scanner sees this exact Unity path but rejects it because its file version does not match the scanner's older `fv-2019` or `fv-2018` metadata. I found no NVIDIA App profile-creation or setting-write event for this editor path, and NVIDIA-related registry searches found no Unity entry.

I determined this by querying the live DRS database with the same read-only NVAPI utility, then cross-checking NVIDIA App's storage, logs, and registry keys. No NVIDIA setting was changed.

The full decoded snapshot is in `unity-findings.md`, with raw output in `unity-query-output.txt`.
