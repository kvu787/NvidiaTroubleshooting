# Unity NVIDIA settings investigation

Date: 2026-08-28

## User

Investigate the current status of any and all NVIDIA settings related to Unity Technologies applications on this machine. Do not change any settings.

Do all work in `C:\Users\k\Repository\NvidiaTroubleshooting\2026-08-28 unity investigation`.

## Assistant

Completed. The current machine state is:

- The only live NVIDIA application profile that targets Unity Technologies software is NVIDIA's predefined `Unity 3D` profile. It matches the basename `unity.exe`, so it applies to all three installed Unity 6 editors and both Unity CLI copies. It is not path-specific.
- The profile has eight explicit settings, and every one is NVIDIA-predefined with its current value equal to its predefined value. There is no Unity-profile user override and no Unity-specific G-SYNC, V-Sync, refresh, power, frame-limit, low-latency, DLSS, or GPU-selection override.
- Unity inherits nine user-set global values. The important ones are Highest available refresh rate, V-Sync controlled by the application, Prefer maximum performance, VRR/G-SYNC enabled in fullscreen-only mode, and G-SYNC application override `allow`. There is no Fixed Refresh rule for Unity.
- The audit inventoried 97 relevant executable paths. Five `unity.exe` paths match `Unity 3D`; the other 92 paths, including Unity Hub, helpers, licensing/build tools, and two local `ZoomTracks.exe` builds, have no DRS application association. Exact profile-name lookups found no custom/orphan Unity or ZoomTracks profile.
- NVIDIA App's current catalog contains no Unity or ZoomTracks entry. Its bundled Unity Editor fingerprint supports only 2018/2019 version patterns, so logs show all three installed Unity 6 versions being rejected on version mismatch. Its DLSS/Frame Generation disable flags are therefore inactive metadata, not current per-app settings.
- The current Unity DRS profile carries NVIDIA's predefined overlay flag `1`. The latest ShadowPlay runtime record classified `Unity.exe` as a game and launcher, not blacklisted, with the hook disabled and its other recorded capture flags off.
- Windows has no per-app GPU preference for Unity/ZoomTracks, and recursive NVIDIA registry searches found no Unity/ZoomTracks entry.
- A broad scan found 15 additional predefined profiles containing `Unity` text or Unity-player metadata, but they belong to unrelated games/legacy Unity content and do not configure the installed Unity Editor or Hub.

No setting was changed. The full report is `2026-08-28 unity investigation/findings.md`; raw DRS, NVIDIA App, ShadowPlay, registry, executable, and setting-reference evidence is in the same folder. The investigation artifacts were committed as `fb2098e` (`Document NVIDIA settings for Unity applications`).
