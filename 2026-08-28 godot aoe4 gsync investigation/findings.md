# Godot 4.6.3 / Age of Empires IV G-SYNC investigation

Investigation date: 2026-08-28 PDT

## Conclusion

Opening the Godot project is not persistently turning off the NVIDIA global G-SYNC setting, and it is not changing the Age of Empires IV profile.

The failure is a live NVIDIA driver-state problem. The Godot project-manager path starts in native OpenGL, rewrites and saves a Godot NVIDIA DRS profile, and causes the driver to reload/reapply profile state while the exact-path Godot profile is forcing Fixed Refresh. Both displays blank for roughly three seconds at that transition. On driver 616.56, the VRR presentation path then remains stuck in the fixed-refresh state after Godot exits, even though the stored global setting, display settings, indicator setting, and AoE4 profile all still say G-SYNC is enabled/allowed.

This explains the apparently contradictory observations:

- No indicator in the Godot editor is expected from the exact-path Fixed Refresh workaround.
- No indicator in AoE4 afterward is the defect: the driver does not successfully transition back from the Godot fixed-refresh state.
- The settings UI and DRS database still show G-SYNC enabled because the persistent configuration was not turned off.

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

### The two Godot profiles did not change

The live DRS state after the failure was line-for-line identical to the earlier working snapshot taken after both Godot profiles had already been created.

The exact-path NVIDIA App profile is selected for the tested installation path:

```text
Profile: Godot_v4.6.3-stable_win64.exe
Application: c:/users/k/program/godot_v4.6.3-stable_win64.exe/godot_v4.6.3-stable_win64.exe
VRR requested state: disabled
G-SYNC application override: fixed refresh
"Enable G-SYNC globally": disabled, stored at current-profile location
```

The broader Godot-created profile also remains present:

```text
Profile: Godot Engine
Application: godot_v4.6.3-stable_win64.exe
"Enable G-SYNC globally": fullscreen only, stored at current-profile location
OpenGL threaded optimization: disabled
```

The confusing setting name `Enable G-SYNC globally` does not mean that Godot edited the base/global profile in this snapshot. NVAPI reports both copies at `current profile` location. Nevertheless, applying the exact-path profile's disabled/fixed-refresh bundle affects the live display pipeline, and that live state is what fails to recover.

### Godot saved DRS again at the failure transition

The active NVIDIA DRS files changed at exactly the project-open transition:

```text
C:\ProgramData\NVIDIA Corporation\Drs\nvdrsdb1.bin  2026-08-28 14:49:19 PDT
C:\ProgramData\NVIDIA Corporation\Drs\nvdrssel.bin  2026-08-28 14:49:19 PDT
```

This was not one-time profile creation. Both Godot profiles and all their settings already existed before 14:49, and the before/after semantic DRS query has no changed line. Godot rewrote the database even though its desired settings were already present.

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
4. The project manager uses native OpenGL and invokes `_nvapi_setup_profile()`.
5. The process itself matches the exact-path Fixed Refresh profile.
6. Godot saves the separate basename `Godot Engine` profile, causing a DRS reload/reapplication.
7. NVIDIA reconfigures the display/VRR path; both monitors blank.
8. Godot correctly remains Fixed Refresh, but driver 616.56 fails to restore usable VRR activation after Godot exits.
9. AoE4 later matches the correct G-SYNC-allowed profile, but the G-SYNC indicator never activates because the live VRR path is still stuck.

Steps 4 through 6 are directly established. Step 7 is established by the user's physical observation at the same timestamp. Step 8 is an inference from the unchanged enabled configuration plus failed runtime activation; it is the only explanation consistent with all recorded state.

## Recovery

The least ambiguous recovery is to toggle global G-SYNC off and back on in NVIDIA App, which reprograms the display state. Rebooting also rebuilds the display session. A Windows graphics-driver reset may recover it, but that has not been verified in this investigation.

Merely restarting AoE4 is not sufficient in the reported reproduction. Re-saving its existing profile is also unlikely to help because its stored settings are already correct.

## Workarounds and isolation tests

### Best low-risk isolation test

Bypass the OpenGL project manager and start this D3D12 editor directly:

```text
"C:\Users\k\Program\Godot_v4.6.3-stable_win64.exe\Godot_v4.6.3-stable_win64.exe" --editor --path "C:\Users\k\Repository\Godot\VsyncStutterTest\Godot" --rendering-driver d3d12
```

This avoids the project manager's native-OpenGL initialization and therefore should avoid Godot's `_nvapi_setup_profile()`/`NvAPI_DRS_SaveSettings()` call. It does not remove the exact-path Fixed Refresh profile, so the editor should remain fixed-refresh. This workaround follows directly from the source path but has not yet been runtime-verified.

Interpretation:

- no monitor blank and AoE4 G-SYNC works afterward: the unconditional Godot DRS save is the decisive trigger;
- blank/sticky failure still occurs: selecting the exact-path Fixed Refresh profile alone is sufficient to trigger the NVIDIA bug.

### Other practical options

- After each affected Godot session, toggle global G-SYNC off/on before gaming.
- Test another NVIDIA driver branch/version. The sticky failure was observed on Game Ready 616.56; no other version was tested here.
- For a durable Godot-side fix, build Godot with the NVAPI profile setup removed or changed so it does not save DRS when the profile already has the desired values.
- Removing the exact-path Fixed Refresh profile avoids this particular fixed-refresh transition, but it also restores the previously observed unwanted G-SYNC activation/choppy pointer behavior in the editor. It is therefore a tradeoff, not a complete fix.

## Upstream bug split

### NVIDIA

Primary defect: after a Fixed Refresh application and a DRS reload/profile transition, a later G-SYNC-allowed application does not activate G-SYNC even though NVIDIA's global, display, indicator, and application-profile state all remain enabled.

Minimal environment data:

- GeForce RTX 5070 Ti Laptop GPU
- Game Ready driver 616.56
- NVIDIA App 11.0.8.299
- two ASUS PA278QGV displays reported by NVIDIA App
- Windows build 26200

### Godot

Integration defect: the native OpenGL manager writes and saves NVIDIA DRS on every initialization, including the OpenGL project manager, even when the desired profile already exists and no setting changes. The resulting driver-wide profile reload is disproportionate for an editor startup and can expose display/VRR transition bugs.

The basename-wide profile creation and the separate NVIDIA App exact-path profile also produce a split-profile configuration, as documented in the earlier investigation.

## Confidence and remaining uncertainty

- High confidence: global G-SYNC was not persistently disabled.
- High confidence: AoE4's profile was not changed and still allows G-SYNC.
- High confidence: Godot rewrote DRS at the blink despite no semantic setting change.
- High confidence: no TDR/display-driver crash was recorded.
- Medium-high confidence: the failure is a sticky NVIDIA runtime VRR state after Godot's DRS reload while Fixed Refresh is active.
- Remaining isolation: direct D3D12 launch is needed to separate the unconditional DRS save from exact-profile activation alone.
