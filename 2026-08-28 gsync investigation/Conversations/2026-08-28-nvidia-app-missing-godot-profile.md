# NVIDIA App missing the Godot profile

Date: 2026-08-28

## User

With the supplied screenshots, we have our first major issue: the custom Godot profile is visible in NVIDIA Control Panel and NVIDIA Profile Inspector, but not in NVIDIA App. NVIDIA has officially retired NVIDIA Control Panel for GeForce Game Ready and Studio drivers, while NVIDIA Profile Inspector is a third-party tool. Investigate why the official replacement does not show the profile.

## Assistant

Confirmed: this is an NVIDIA App profile-discovery/adoption defect, not a missing DRS profile or an NVIDIA Profile Inspector artifact.

The live NVIDIA driver database contains the user-defined `Godot Engine` profile and associates it with `godot_v4.6.3-stable_win64.exe`. NVIDIA Control Panel and NVIDIA Profile Inspector enumerate that database, so both show it. NVIDIA App does not build Program Settings by enumerating driver profiles. Its installed code starts from a separate scanned/manual application catalog and only resolves DRS profiles for entries that pass that catalog's filter.

The current NVIDIA App catalog contains Age of Empires IV and Steam, but no Godot. Age of Empires passes the UI filter and Steam does not, exactly explaining the screenshot's `1/1 Programs` display.

NVIDIA App's backend still recognizes the running executable through DRS and explicitly logs that its application-catalog record is null while the DRS profile name is `Godot Engine`. This proves the app has enough information to find the profile but does not use that information to populate Program Settings.

The manual-add path has a second defect. For a manual executable with no cached profile name, NVIDIA App first attempts to create a profile named after the executable, then resolves the executable's profile only if creation succeeds. Because Godot is already associated with the differently named `Godot Engine` profile, the create-first path collides instead of adopting it. The earlier log captured `NVAPI_EXECUTABLE_ALREADY_IN_USE` (`-167`) and `NVAPI_PROFILE_NAME_IN_USE` (`-164`), matching the installed code and NVIDIA's NVAPI definitions.

NVIDIA's current official statement confirms the impact: classic NVIDIA Control Panel is retiring for Game Ready and Studio drivers, receives no further features or fixes, and NVIDIA App's Graphics > Program Settings is its stated replacement. The replacement currently cannot expose this legitimate custom driver profile.

This is separate from the remaining G-SYNC problem. The current Godot profile contains `fullscreen only` mode and disabled threaded optimization, but no Fixed Refresh/application-level G-SYNC override. It therefore does not yet disable G-SYNC.

The full evidence, implementation trace, official sources, and a minimal NVIDIA bug report are recorded in `nvidia-app-profile-visibility-findings.md`. No NVIDIA settings were changed.
