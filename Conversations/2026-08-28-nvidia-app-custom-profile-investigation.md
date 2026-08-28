# NVIDIA App custom-profile and G-SYNC investigation

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

---

## User

I'm observing an issue with creating custom program profiles with the NVIDIA App. I have G-SYNC enabled currently, but it is activating on the Godot editor when it shouldn't be. To fix that, I need to create a custom profile that disables G-SYNC for the Godot editor executable. However, this workaround only seems to work when the custom profile is created in the no-longer-supported NVIDIA Control Panel. Creating a custom profile in NVIDIA App doesn't seem to work.

We're going to investigate this. Do all of your work in the `2026-08-28 gsync investigation` subfolder.

First, tell me what kind of NVIDIA custom profile stuff is currently configured for `C:\Users\k\Program\Godot_v4.6.3-stable_win64.exe\Godot_v4.6.3-stable_win64.exe` (if any) and tell me how you determined that info.

## Assistant

Currently, none. At the approximately 12:31 PDT snapshot, the target executable has no NVIDIA DRS application association and no active custom program profile. Therefore it inherits the global NVIDIA settings; there is no per-program Fixed Refresh/G-SYNC override stored for it.

The direct checks were:

- `NvAPI_DRS_FindApplicationByName` returned `-166` (`NVAPI_EXECUTABLE_NOT_FOUND`) for both the full path and the normal DRS basename, `Godot_v4.6.3-stable_win64.exe`.
- `NvAPI_DRS_FindProfileByName` returned `-163` (`NVAPI_PROFILE_NOT_FOUND`) for both historically relevant names: `Godot_v4.6.3-stable_win64.exe` and `Godot Engine`.
- Enumerating all 7,957 DRS profiles found no profile name containing `Godot`.
- NVIDIA App's current `NvBackend\ApplicationStorage.json` contains no manual Godot entry.
- NVIDIA-related registry searches found no Godot entry.

I determined this with a small read-only utility that loads `nvapi64.dll` and uses NVIDIA's DRS query functions. It does not load any create, set, save, or delete entry points. I cross-checked the result against NVIDIA App's current application storage, its logs, and NVIDIA registry keys. No NVIDIA setting was changed.

The logs do preserve useful earlier history: NVIDIA App's attempt to create this association failed with `-167` (`NVAPI_EXECUTABLE_ALREADY_IN_USE`), its backend mapped the executable to `Godot Engine`, a later profile-creation attempt failed with `-164` (`NVAPI_PROFILE_NAME_IN_USE`), and the manual Godot entry was deleted successfully at 12:09:47. Those are earlier failed/removed records, not current active configuration.

Godot 4.6.3's own OpenGL initialization code can create a `Godot Engine` DRS profile, associate the executable basename, disable OpenGL threaded optimization, and set G-SYNC mode to fullscreen-only. That is the leading explanation for the earlier NVIDIA App collision, but the profile is absent now.

The evidence and reproducible query are stored in `findings.md`, `query-output.txt`, and `drs-query.cpp` in the investigation folder.

---

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

---

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

- `0x1194F158 = 1`: G-SYNC mode - fullscreen only
- `0x20C1221E = 2`: OpenGL threaded optimization - disabled

The critical detail is what is missing: the profile has no per-application G-SYNC override (`0x10A879CF`). That value is inherited from the global profile as `0` (`allow`). Therefore the current Godot-created profile is not equivalent to a Control Panel **Fixed Refresh** override. Your active G-SYNC indicator shows that its two settings are not preventing G-SYNC in this windowed editor session.

This profile was created by Godot during the recent launch, not by NVIDIA App:

- The total DRS profile count increased from 7,957 to 7,958.
- The DRS database changed at 12:49:02.
- NVIDIA App's application storage is unchanged since 12:09:47 and still has no Godot entry.
- NVIDIA App began classifying the executable through the new `Godot Engine` DRS profile at 12:49:23.
- Godot 4.6.3's OpenGL code writes exactly this profile name, application association, and these two settings.

The full snapshot is in `godot-after-project-launch-findings.md`; the raw DRS output is in `godot-after-project-launch-query-output.txt`. No NVIDIA setting was changed.

---

## User

With the supplied screenshots, we have our first major issue: the custom Godot profile is visible in NVIDIA Control Panel and NVIDIA Profile Inspector, but not in NVIDIA App. NVIDIA has officially retired NVIDIA Control Panel for GeForce Game Ready and Studio drivers, while NVIDIA Profile Inspector is a third-party tool. Investigate why the official replacement does not show the profile.

Attached images:

![Godot profile in NVIDIA Control Panel](<../2026-08-28 gsync investigation/screenshots/godot-profile-nvcp.png>)

![Godot profile in NVIDIA Profile Inspector](<../2026-08-28 gsync investigation/screenshots/godot-profile-npi.png>)

![Godot profile missing from NVIDIA App](<../2026-08-28 gsync investigation/screenshots/godot-profile-missing-nvidia-app.png>)

## Assistant

Confirmed: this is an NVIDIA App profile-discovery/adoption defect, not a missing DRS profile or an NVIDIA Profile Inspector artifact.

The live NVIDIA driver database contains the user-defined `Godot Engine` profile and associates it with `godot_v4.6.3-stable_win64.exe`. NVIDIA Control Panel and NVIDIA Profile Inspector enumerate that database, so both show it. NVIDIA App does not build Program Settings by enumerating driver profiles. Its installed code starts from a separate scanned/manual application catalog and only resolves DRS profiles for entries that pass that catalog's filter.

The current NVIDIA App catalog contains Age of Empires IV and Steam, but no Godot. Age of Empires passes the UI filter and Steam does not, exactly explaining the screenshot's `1/1 Programs` display.

NVIDIA App's backend still recognizes the running executable through DRS and explicitly logs that its application-catalog record is null while the DRS profile name is `Godot Engine`. This proves the app has enough information to find the profile but does not use that information to populate Program Settings.

The manual-add path has a second defect. For a manual executable with no cached profile name, NVIDIA App first attempts to create a profile named after the executable, then resolves the executable's profile only if creation succeeds. Because Godot is already associated with the differently named `Godot Engine` profile, the create-first path collides instead of adopting it. The earlier log captured `NVAPI_EXECUTABLE_ALREADY_IN_USE` (`-167`) and `NVAPI_PROFILE_NAME_IN_USE` (`-164`), matching the installed code and NVIDIA's NVAPI definitions.

NVIDIA's current official statement confirms the impact: classic NVIDIA Control Panel is retiring for Game Ready and Studio drivers, receives no further features or fixes, and NVIDIA App's Graphics > Program Settings is its stated replacement. The replacement currently cannot expose this legitimate custom driver profile.

This is separate from the remaining G-SYNC problem. The current Godot profile contains `fullscreen only` mode and disabled threaded optimization, but no Fixed Refresh/application-level G-SYNC override. It therefore does not yet disable G-SYNC.

The full evidence, implementation trace, official sources, and a minimal NVIDIA bug report are recorded in `nvidia-app-profile-visibility-findings.md`. No NVIDIA settings were changed.

---

## User

I updated `AGENTS.md`. Fix the conversations organization accordingly.

## Assistant

Reorganized the records to follow the updated instructions:

- Consolidated this entire thread into this one conversation file.
- Located the file in the repository-root `Conversations` folder.
- Removed the four per-turn files from the investigation subfolder.
- Embedded links to the three attached screenshots in the applicable user turn; the image files remain preserved in the repository.
- Kept the reorganization in its own `[record]` commit.
