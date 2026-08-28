# NVIDIA App custom-profile investigation plan

Date: 2026-08-28

## User

I'm observing an issue with creating custom program profiles with the NVIDIA App. I have G-SYNC enabled currently, but it is activating on the Godot editor when it shouldn't be. To fix that, I need to create a custom profile that disables G-SYNC for the Godot editor executable. However, this workaround only seems to work when the custom profile is created in the no-longer-supported NVIDIA Control Panel. Creating a custom profile in NVIDIA App doesn't seem to work.

Explain how you would investigate this. Don't actually do the investigation yet.

## Assistant

I would investigate this as a profile-persistence and profile-matching problem first, then test the runtime behavior. I would not begin by reinstalling drivers or changing many graphics settings, because that would destroy useful evidence and add variables.

First, I would record a precise baseline: Windows build, GPU and display topology, driver and NVIDIA App versions, Godot version and exact editor executable path, whether G-SYNC is enabled for fullscreen only or for windowed and fullscreen applications, and how activation is being observed. I would also note Windows variable-refresh-rate and windowed-game settings, overlays, and whether Godot or NVIDIA software is elevated. The success criterion would be explicit: with global G-SYNC left enabled, the Godot editor must remain fixed-refresh while another known G-SYNC application still activates it.

Next, I would build a small reproduction matrix. Each case would start from a freshly launched Godot process and would change only how the Godot-specific profile was created: no override, an override created in NVIDIA Control Panel, and the purportedly equivalent override created in NVIDIA App. If relevant, I would add restart variants for Godot, NVIDIA App, and Windows, because driver-profile changes can be cached. I would repeat the decisive cases once to rule out a transient result.

For each case, I would verify the actual process behind the editor window rather than trusting a shortcut name. Godot distributions can include differently named binaries, and the editor, a launched game, and helper or console processes must not be conflated. I would capture the owning process ID, image name, full path, and elevation context. This tests whether NVIDIA App associated the profile with the wrong executable or encoded the application match differently from Control Panel.

The most important comparison would be the underlying driver profile, not the two user interfaces. Using a read-only profile export or NVIDIA's NVAPI Driver Settings interface, I would compare:

- The profile name and application association written by each UI.
- The exact executable match entries.
- Which setting identifier and value represent the G-SYNC/monitor-technology override.
- Whether the setting is explicitly stored, inherited, omitted, or written with a different value.
- Whether an NVIDIA predefined profile or another profile takes precedence.

If needed, I would use Process Monitor narrowly around pressing **Apply/Save** to learn which profile database, file, or registry operations each UI performs. This would be supporting evidence; the profile export or NVAPI view would remain the primary evidence.

I would then classify the result:

1. If NVIDIA App does not write the equivalent setting, writes a different value, or loses it after relaunch, this is an NVIDIA App persistence or feature-parity defect.
2. If the setting is correct but the application association differs, this is an executable-matching defect or configuration mistake.
3. If both stored profiles are identical but only the Control Panel-created case works, I would test cache/reload behavior and profile precedence, then look for a driver/App regression.
4. If both cases work only after the same restart, the apparent difference is probably an application or driver-profile reload issue.
5. If the result varies with Windows windowed VRR, fullscreen mode, overlays, elevation, display choice, or Godot's rendering backend, I would isolate that interaction one variable at a time.

Only after the local evidence identifies the failing layer would I research NVIDIA documentation, release notes, and reports for the exact App/driver versions. If it appears version-specific, I would confirm it with one controlled version change rather than broad trial-and-error. I would avoid a clean driver installation until profile exports and logs had been preserved.

The final deliverable would be a compact reproduction table, before-and-after profile exports, the exact failing setting or match entry if identifiable, a safe workaround, and a minimal NVIDIA bug report. No part of that investigation is being performed yet.
