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

Godot is not persistently switching off the global G-SYNC control. It is triggering a driver-state failure that has the same practical effect for the rest of the Windows session.

The decisive facts are:

- After the failure, NVIDIA's own live API still reports global G-SYNC enabled, the indicator enabled, fullscreen VRR mode selected, and G-SYNC enabled on both ASUS displays.
- AoE4 still resolves to its normal NVIDIA profile with G-SYNC allowed. Nothing in its profile changed.
- Both Godot profiles are also line-for-line unchanged from the earlier working snapshot.
- Nevertheless, the active NVIDIA DRS database was rewritten at 14:49:19, exactly when the monitors blinked.

That rewrite comes from Godot itself. Godot's project manager defaults to native OpenGL even though this project uses D3D12. During native OpenGL initialization, Godot 4.6.3 finds/creates its `Godot Engine` NVIDIA profile, writes the fullscreen-only G-SYNC mode and threaded-optimization values, and calls `NvAPI_DRS_SaveSettings()` unconditionally. This happens even when the profile already contains the requested values.

At the same time, this Godot executable matches the separate exact-path NVIDIA App profile that forces Fixed Refresh. The DRS save makes the driver reload/reapply profile state while that fixed-refresh profile is active. Driver 616.56 reconfigures the display pipeline—hence the three-second blank on both monitors—but then fails to transition the VRR presentation path back when Godot exits. AoE4 later gets the correct G-SYNC-allowed profile, yet G-SYNC does not engage because the live driver state is stuck.

So “globally off” accurately describes the symptom, but not the saved configuration. The configuration is still on; runtime VRR activation is wedged.

For immediate recovery, toggle global G-SYNC off and back on in NVIDIA App, or reboot. The best low-risk isolation/workaround is to bypass Godot's OpenGL project manager and start the D3D12 editor directly:

```text
"C:\Users\k\Program\Godot_v4.6.3-stable_win64.exe\Godot_v4.6.3-stable_win64.exe" --editor --path "C:\Users\k\Repository\Godot\VsyncStutterTest\Godot" --rendering-driver d3d12
```

If that avoids the blink and AoE4 G-SYNC works afterward, Godot's unconditional DRS save is the decisive trigger. If it still fails, merely selecting the exact-path Fixed Refresh profile is sufficient to trigger the NVIDIA bug.

The full findings and timestamped evidence are in `2026-08-28 godot aoe4 gsync investigation/findings.md` and `evidence.md`.
