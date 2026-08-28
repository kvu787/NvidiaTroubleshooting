# Godot / AoE4 G-SYNC session-state investigation

Date: 2026-08-28

## User

i've observed some strange behavior. it seems like opening a godot 4.6.3 project is turning off gsync globally, not just disabling it for godot editor.

these are my exact steps:
- open and play aoe4 and observe the g-sync indicator at the top right
- close aoe4
- open godot 4.6.3 and open this project: "C:\Users\k\Repository\Godot\VsyncStutterTest\Godot"
- as the godot project is opening, observe a 3 second long blink on both attached monitors
- when the godot project is fully opened, observe no g-sync indicator at the top right
- do some general usage of the godot editor and observe no gsync behavior
- close godot editor
- open aoe4 and observe no g-sync indicator at the top right

what's going on?

do your all work in "C:\Users\k\Repository\NvidiaTroubleshooting\2026-08-28 godot aoe4 gsync investigation"

## Assistant

Godot is not actually saving the global G-SYNC setting as off. It is leaving NVIDIA driver 616.56's live VRR path stuck in Fixed Refresh, which has the same practical effect for the rest of the Windows session.

The decisive evidence after your reproduction is:

- NVIDIA's live API reports `globalGsyncState=1`, indicator enabled, and G-SYNC enabled on both ASUS displays.
- AoE4 still resolves to its normal profile with G-SYNC allowed; no AoE4 setting changed.
- Both Godot profiles are line-for-line unchanged from the earlier working snapshot.
- The active NVIDIA DRS database was nevertheless rewritten at 14:49:19, exactly at the monitor blink.
- No TDR, `nvlddmkm`, display-reset event, LiveKernelReport, or WER crash was recorded.

The trigger is the interaction between two Godot profiles. Your exact-path NVIDIA App profile forces Fixed Refresh. Separately, Godot's project manager starts in native OpenGL even though this project uses D3D12. Native OpenGL initialization finds/creates the basename `Godot Engine` profile, writes fullscreen-only G-SYNC plus threaded optimization, and calls `NvAPI_DRS_SaveSettings()` unconditionally—even when nothing changed. Godot intentionally added this path to suppress unstable windowed G-SYNC ([origin commit](https://github.com/godotengine/godot/commit/b8edc64379b3c4b5f2e7334468be65fd44a4980c)); it remains in the [4.6.3 source](https://github.com/godotengine/godot/blob/4.6.3-stable/platform/windows/gl_manager_windows_native.cpp).

That save reloads/reapplies profile state while the exact-path Fixed Refresh profile is active. The driver reconfigures the display pipeline—hence the three-second blank—but then fails to transition VRR back after Godot exits. AoE4 receives the correct G-SYNC-allowed profile, yet G-SYNC does not engage because the live path is wedged. So “globally off” accurately describes the symptom, but not the saved configuration.

For immediate recovery, toggle global G-SYNC off and back on in NVIDIA App, or reboot. The best low-risk isolation test is to bypass the OpenGL project manager and launch this D3D12 editor directly:

```text
"C:\Users\k\Program\Godot_v4.6.3-stable_win64.exe\Godot_v4.6.3-stable_win64.exe" --editor --path "C:\Users\k\Repository\Godot\VsyncStutterTest\Godot" --rendering-driver d3d12
```

If that avoids the blink and AoE4 G-SYNC works afterward, Godot's unconditional DRS save is the decisive trigger. If it still fails, merely selecting the exact-path Fixed Refresh profile is sufficient to trigger the NVIDIA bug.

The complete analysis is in `2026-08-28 godot aoe4 gsync investigation/findings.md`; the timestamped snapshot is in `evidence.md`. Both the investigation and conversation record are committed, and the working tree is clean.
