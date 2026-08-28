# Godot 4.6.3 / Age of Empires IV G-SYNC investigation

Investigation date: 2026-08-28 PDT

## Conclusion

Opening the Godot project is not persistently turning off the NVIDIA global G-SYNC setting, and it is not changing the Age of Empires IV profile.

The failure is a live NVIDIA driver-state problem. The Godot project-manager path starts in native OpenGL and unconditionally saves its NVIDIA DRS profile. When the profile selected for Godot is configured as Fixed Refresh, that save/reload transition blanks both displays for roughly three seconds. On driver 616.56, the VRR presentation path then remains stuck in the fixed-refresh state after Godot exits, even though the stored global setting and AoE4 profile still allow G-SYNC.

This explains the apparently contradictory observations:

- No indicator in the Godot editor is expected from whichever matching Godot profile is configured as Fixed Refresh.
- No indicator in AoE4 afterward is the defect: the driver does not successfully transition back from the Godot fixed-refresh state.
- The settings UI and DRS database still show G-SYNC enabled because the persistent configuration was not turned off.

A subsequent recovery test confirmed this interpretation: disabling global G-SYNC, applying, enabling it again, and applying restored the G-SYNC indicator in AoE4 without any Godot or AoE4 profile edit.

A later one-profile isolation test made the trigger narrower. The exact-path NVIDIA App profile was deleted, and the remaining basename `Godot Engine` profile was set to Fixed Refresh in NVIDIA Control Panel. Godot still produced the same three-second blank, and AoE4 still failed to activate G-SYNC afterward. Therefore duplicate profiles, exact-path matching, and the NVIDIA App-created profile are not required for the failure. The common condition is a Fixed Refresh Godot profile combined with Godot's project-manager DRS save/reload.

The best classification is an NVIDIA 616.56 per-application VRR/profile-transition bug exposed by Godot's unconditional NVAPI profile save. Godot's behavior is an important trigger and an avoidable integration problem, but the failure to restore G-SYNC for a later application is a driver failure.

## Decisive evidence

### Persistent global and display state remained enabled

A read-only NVIDIA App launch at 14:59, after the reported failure, caused NVIDIA's own display APIs to report:

```text
GetGlobalGsyncState: 1
GetGsyncIndicator: 1
Display 1 gsyncState.enabled: true
Display 2 gsyncState.enabled: true
```

The same query reported `vrrMode: 1` (fullscreen only). Dynamic Display Switching reported success with `MuxState: 1`; there was no logged display-mode switch at the reproduction time.

Therefore the missing in-game indicator is not explained by a disabled global toggle, a disabled indicator toggle, or G-SYNC being disabled on the displays.

### AoE4's NVIDIA profile remained G-SYNC-capable

`RelicCardinal.exe` still resolves to NVIDIA's predefined `Age of Empires IV` profile. Its effective relevant settings after the failure are:

| Setting | Value | Source |
|---|---:|---|
| G-SYNC application override (`0x10A879CF`) | `0`, allow | Global profile |
| VRR requested state (`0x1094F1F7`) | `1`, fullscreen only | AoE4 profile |
| G-SYNC mode (`0x1194F158`) | `1`, fullscreen only | AoE4 profile |

AoE4 has no Fixed Refresh override. The driver's fully qualified and basename lookups both select `Age of Empires IV`.

### The failure survives removal of the exact-path profile

The user deleted the exact-path `Godot_v4.6.3-stable_win64.exe` profile in NVIDIA Profile Inspector, then set the remaining `Godot Engine` profile to `Monitor Technology: Fixed Refresh` in NVIDIA Control Panel.

The post-failure DRS query proves that the requested profile deletion took effect:

```text
Total profile count: 7958 (previously 7959)
FindProfileByName("Godot_v4.6.3-stable_win64.exe"): not found
Full-path lookup: Godot Engine
Basename lookup: Godot Engine
Application association: godot_v4.6.3-stable_win64.exe
```

Only one Godot profile now exists. Its relevant state after the reproduction is:

```text
Profile: Godot Engine
VRR requested state: disabled
G-SYNC application override: fixed refresh
G-SYNC mode: fullscreen only
OpenGL threaded optimization: disabled
```

This is a single, basename-associated Fixed Refresh profile. The exact-path profile, split-profile selection, and NVIDIA App-specific profile creation are no longer possible explanations, yet the physical and AoE4 symptoms were identical.

The apparently contradictory `G-SYNC mode: fullscreen only` line is a setting stored inside this application profile, not proof that Godot turned the base/global toggle on or off. Godot itself writes that value and disables OpenGL threaded optimization. NVIDIA Control Panel supplies the Fixed Refresh/VRR-disabled settings. Together they form the nine-setting `Godot Engine` profile seen after the test.

### The NVIDIA App row is not the deleted profile

After the exact-path DRS profile was deleted, NVIDIA App still displayed a sidebar row named `Godot_v4.6.3-stable_win64.exe`. A fresh comparison shows that this is NVIDIA App's separate manually-added application-catalog record, not a recreated DRS profile.

NVIDIA App's `ApplicationStorage.json` still contains:

```text
LocalId: 963528738
DisplayName: Godot_v4.6.3-stable_win64.exe
DetectedFiles: C:\Users\k\Program\Godot_v4.6.3-stable_win64.exe\Godot_v4.6.3-stable_win64.exe
IsManuallyAdded: true
IsFingerprintDetected: false
DriverProfile: empty
LastLaunchTime: 2026-08-28 15:15:35 PDT
```

At the same time, live DRS still reports:

```text
FindProfileByName("Godot_v4.6.3-stable_win64.exe"): not found
Full-path executable lookup: Godot Engine
Basename executable lookup: Godot Engine
Total profiles: 7958
```

These are different identity stores. NVIDIA Profile Inspector deleted the driver profile but does not edit NVIDIA App's private application catalog. The NVIDIA App `2/2 Programs` count refers to accepted application-catalog rows, not to DRS profiles. Merely seeing the row therefore does not show that the exact-path profile returned.

The remaining `Godot Engine` DRS profile is also intentional in the latest test: it was the "other" profile that the user retained and changed to Fixed Refresh. NVIDIA's backend resolved the running executable to `Godot Engine` at 15:15:38 and 15:16:38, agreeing with the direct DRS query.

The screenshot has AoE4 selected, so it provides no evidence that NVIDIA App successfully loaded or recreated a Godot profile. Selecting or editing the stale/manual Godot row could cause NVIDIA App to run its profile resolution/creation path again; preserve the current state and query DRS afterward if that behavior is tested.

### Godot saved DRS again at the failure transition

The active NVIDIA DRS files changed at exactly the project-open transition:

```text
C:\ProgramData\NVIDIA Corporation\Drs\nvdrsdb1.bin  2026-08-28 14:49:19 PDT
C:\ProgramData\NVIDIA Corporation\Drs\nvdrssel.bin  2026-08-28 14:49:19 PDT
```

This was not one-time profile creation. Both Godot profiles and all their settings already existed before 14:49, and the before/after semantic DRS query has no changed line. Godot rewrote the database even though its desired settings were already present.

The one-profile repetition shows the same behavior more directly. NVIDIA's active DRS files were written at 15:15:04 when the Control Panel profile change was applied and again at 15:15:32 when the first Godot process started:

```text
15:15:32.762  Godot project-manager process observed by NVIDIA
15:15:32.915  nvdrsdb1.bin and nvdrssel.bin rewritten
15:15:35.244  spawned Godot editor process observed by NVIDIA
```

The two settings that Godot's source writes were already present in `Godot Engine` from the earlier profile creation. The current nine-setting profile is consistent with NVIDIA Control Panel adding Fixed Refresh to those existing Godot settings, followed by Godot re-saving them. There was no prelaunch raw DRS snapshot in this test, so exact line-for-line semantic identity cannot be claimed for the 15:15:32 write; the source call and coincident database rewrite are established.

### Godot's source contains the triggering save

Godot 4.6.3's native Windows OpenGL manager calls `_nvapi_setup_profile()` during `GLManagerNative_Windows::initialize()`.

That routine:

1. loads NVIDIA DRS;
2. finds or creates a profile using the project/application name;
3. finds or creates a basename application association;
4. sets OpenGL threaded optimization;
5. writes NVIDIA setting `0x1194F158` to fullscreen-only; and
6. calls `NvAPI_DRS_SaveSettings()` unconditionally.

The project manager defaults to native `opengl3` / `gl_compatibility`, so this runs even though the project itself specifies D3D12. The D3D12 editor is spawned only after the OpenGL project-manager process has initialized.

Godot added this behavior in commit [`b8edc643`](https://github.com/godotengine/godot/commit/b8edc64379b3c4b5f2e7334468be65fd44a4980c), explicitly to disable windowed G-SYNC because of unstable editor refresh rates. The current 4.6.3 implementation is in [`gl_manager_windows_native.cpp`](https://github.com/godotengine/godot/blob/4.6.3-stable/platform/windows/gl_manager_windows_native.cpp). The original editor/VRR problem is tracked in [Godot issue #38219](https://github.com/godotengine/godot/issues/38219).

### This was not a TDR or GPU crash

For 14:40 through 15:00 there were:

- no `Display`, `nvlddmkm`, `DxgKrnl`, or display-related `Kernel-PnP` System events;
- no System event 4101;
- no new `LiveKernelReports` dump; and
- no WER report around the reproduction.

The only warning/error in the narrow interval was an unrelated Game Bar DCOM timeout at 14:50:09.

The three-second blank is therefore best understood as display-pipeline/profile reconfiguration rather than Windows recovering from a driver timeout.

## Trigger sequence

1. AoE4 runs with its profile and activates G-SYNC.
2. AoE4 exits.
3. Godot starts through the project manager.
4. The process matches a Godot profile configured as Fixed Refresh. In the latest test this is the sole basename `Godot Engine` profile.
5. The project manager uses native OpenGL and invokes `_nvapi_setup_profile()`.
6. Godot saves DRS, causing a profile reload/reapplication.
7. NVIDIA reconfigures the display/VRR path; both monitors blank.
8. Godot correctly remains Fixed Refresh, but driver 616.56 fails to restore usable VRR activation after Godot exits.
9. AoE4 later matches the correct G-SYNC-allowed profile, but the G-SYNC indicator never activates because the live VRR path is still stuck.

Steps 4 through 6 are directly established. Step 7 is established by the user's physical observation at the same timestamp. Step 8 is an inference from the enabled stored configuration, the verified global-toggle recovery, and the failed runtime activation; it is the explanation consistent with all recorded state.

## Recovery

Global G-SYNC off/on in NVIDIA App is now a verified recovery.

The user performed no other issue-related action between the failed AoE4 test and this sequence:

1. opened NVIDIA App;
2. disabled G-SYNC and applied;
3. enabled G-SYNC and applied;
4. closed NVIDIA App;
5. opened AoE4 and observed the top-right G-SYNC indicator; and
6. closed AoE4.

NVIDIA's logs independently corroborate the control changes:

```text
15:09:34.521  Set global GsyncState=0, globalVRRMode=0
15:09:37.814  SetGlobalGsyncState returned success (3528.9 ms)
15:09:43.082  Set global GsyncState=1, globalVRRMode=1
15:09:43.086  SetGlobalGsyncState returned success (233.4 ms)
15:09:54      NVIDIA App records the subsequent AoE4 launch
15:11:37.470 NVIDIA backend detects the AoE4 session ending
```

The DRS database writes at 15:09:34 and 15:09:43 align with the off and on applies. After the test, AoE4 still selected `Age of Empires IV` with G-SYNC allowed and fullscreen-only VRR; no AoE4-specific repair or profile change was needed.

This recovery sharply strengthens the live-state diagnosis. Cycling the global setting forces the display/VRR path to be programmed through an actual disabled state and back to enabled, clearing the stale Fixed Refresh condition that merely launching AoE4 could not clear.

Rebooting should also rebuild the display session but was not tested. A Windows graphics-driver reset may recover it, but that has not been verified in this investigation.

Merely restarting AoE4 is not sufficient in the reported reproduction. Re-saving its existing profile is also unlikely to help because its stored settings are already correct.

## Workarounds and isolation tests

### Best low-risk isolation test

Bypass the OpenGL project manager and start this D3D12 editor directly:

```text
"C:\Users\k\Program\Godot_v4.6.3-stable_win64.exe\Godot_v4.6.3-stable_win64.exe" --editor --path "C:\Users\k\Repository\Godot\VsyncStutterTest\Godot" --rendering-driver d3d12
```

If D3D12 initializes successfully, this bypasses the project manager's native-OpenGL initialization and therefore avoids Godot's `_nvapi_setup_profile()`/`NvAPI_DRS_SaveSettings()` call. It does not remove the basename Fixed Refresh profile, so NVIDIA's driver can still apply that profile to the process and the editor should remain fixed-refresh. This workaround follows directly from the source path but has not yet been runtime-verified.

The command alone is not a fail-closed guarantee. Godot 4.6.3 defines `rendering/rendering_device/fallback_to_vulkan`, `fallback_to_d3d12`, and `fallback_to_opengl3` as `true`. On Windows, a requested D3D12 startup tries D3D12, may try Vulkan, and, if both RenderingDevice backends fail, may switch to native OpenGL. That native OpenGL fallback constructs `GLManagerNative_Windows`, whose `initialize()` calls `_nvapi_setup_profile()`.

For a source-level guarantee that a failed D3D12 initialization cannot reach Godot's NVAPI DRS writer, set this project setting before the test:

```ini
[rendering]
rendering_device/fallback_to_opengl3=false
```

Optionally also set `rendering_device/fallback_to_vulkan=false` if the requirement is specifically D3D12-or-fail rather than merely no native-OpenGL fallback. With OpenGL fallback disabled, a D3D12/Vulkan failure aborts display-server initialization instead of entering the only Godot 4.6.3 Windows code path that references NVAPI or writes DRS.

This guarantee is narrowly about Godot's explicit NVIDIA settings mutation. A successful D3D12 editor necessarily loads `D3D12.dll` and `DXGI.dll`, creates a D3D12 device, and uses the NVIDIA display driver. NVIDIA may read and apply the executable's existing DRS profile as part of normal process/device initialization; that is not Godot editing the profile.

Interpretation:

- no monitor blank and AoE4 G-SYNC works afterward: the unconditional Godot DRS save is the decisive trigger;
- blank/sticky failure still occurs: selecting any Fixed Refresh Godot profile is sufficient to trigger the NVIDIA bug even without the OpenGL profile save.

### Other practical options

- After each affected Godot session, use the now-verified global G-SYNC off/on recovery before gaming.
- Test another NVIDIA driver branch/version. The sticky failure was observed on Game Ready 616.56; no other version was tested here.
- For a durable Godot-side fix, build Godot with the NVAPI profile setup removed or changed so it does not save DRS when the profile already has the desired values.
- Removing only the exact-path profile is now proven insufficient if the remaining basename profile is also set to Fixed Refresh.
- Removing Fixed Refresh from every profile that can match the Godot executable should avoid this particular transition, but it also restores the previously observed unwanted G-SYNC activation/choppy pointer behavior in the editor. It is therefore a tradeoff, not a complete fix.

## Upstream bug split

### NVIDIA

Primary defect: after a Fixed Refresh application and a DRS reload/profile transition, a later G-SYNC-allowed application does not activate G-SYNC even though NVIDIA's persistent global and application-profile state remain enabled. The initial reproduction additionally confirmed the live API and both display flags remained enabled.

Minimal environment data:

- GeForce RTX 5070 Ti Laptop GPU
- Game Ready driver 616.56
- NVIDIA App 11.0.8.299
- two ASUS PA278QGV displays reported by NVIDIA App
- Windows build 26200

### Godot

Integration defect: the native OpenGL manager writes and saves NVIDIA DRS on every initialization, including the OpenGL project manager, even when the desired profile already exists and no setting changes. The resulting driver-wide profile reload is disproportionate for an editor startup and can expose display/VRR transition bugs.

Godot's basename-wide profile creation can conflict or combine with per-application settings created in NVIDIA tools. The split exact-path/basename arrangement documented earlier is confusing but, as the one-profile test proves, it is not necessary for this defect.

## Confidence and remaining uncertainty

- High confidence: global G-SYNC was not persistently disabled.
- High confidence: AoE4's profile was not changed and still allows G-SYNC.
- High confidence: Godot rewrote DRS at the blink despite no semantic setting change.
- High confidence: no TDR/display-driver crash was recorded.
- High confidence: the failed state is recoverable by reprogramming global G-SYNC off/on without repairing either application profile.
- High confidence: the failure is a sticky NVIDIA runtime VRR state rather than persistent global or application-profile damage.
- High confidence: duplicate profiles, exact-path matching, and NVIDIA App profile creation are not required.
- Medium-high confidence: Godot's DRS reload while any matching Godot profile is Fixed Refresh creates that sticky state.
- Remaining isolation: direct D3D12 launch is needed to separate the unconditional DRS save from Fixed Refresh profile activation alone.
