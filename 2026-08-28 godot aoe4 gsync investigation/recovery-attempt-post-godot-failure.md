# Recovery attempt: post-Godot sticky G-SYNC failure

Captured: 2026-08-29 03:49-03:51 PDT

## User observation

The user followed the last-known-good recovery guide far enough to pass the neutral pre-editor control. During the full acceptance test, the immediate post-Godot `VsyncStutterTest.exe` run lost G-SYNC.

No relevant 3D application was running during the read-only capture. The failed live state was preserved: global G-SYNC was not toggled and Godot was not reopened before the probes.

## Windows topology is correct

`QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS)` reported three active target paths and two source IDs:

```text
source 0 at (0,0), 2560x1440
  -> target 8452, PA278QGV, external DisplayPort connector instance 1
  -> target signal 2560x1440 at 119.998 Hz

source 2 at (2560,0), 2560x1440
  -> target 8449, NE180QDM-NZC, embedded DisplayPort
  -> target signal 2560x1600 at 240 Hz
  -> target 8448 clone representation, PA278QGV, native HDMI
  -> target signal 2560x1440 at 119.998 Hz
```

This restores the two deviations seen before the recovery attempt: eDP is no longer at 60 Hz, and the intended internal-eDP-plus-HDMI clone is intact. All three paths are directly owned by the RTX 5070 Ti Laptop GPU.

## DRS profiles are correct

The matching profile is the single predefined `Godot Engine` profile. It contains the 4.4.1 and 4.6.3 basename application associations and nine explicit settings, including:

```text
0x1094F1F7 = 0  VRR requested state: disabled
0x10A879CF = 4  G-SYNC application override: fixed refresh
0x1194F158 = 1  G-SYNC mode: fullscreen only
0x20C1221E = 2  threaded optimization: disabled
```

Both the full-path and basename lookups resolve the 4.6.3 executable to this profile. `VsyncStutterTest.exe` has no DRS association or target-related profile name.

Godot wrote the DRS databases at 03:45:08 and 03:45:37, as expected from its native-OpenGL profile setup. The effective profile remained correct after those writes.

## Live NVAPI state isolates the failure

Driver: 596.49, branch `r596_25`.

```text
target 8448, HDMI PA:
  active=1, possible=1, displayInVrrMode=1

target 8449, internal eDP:
  active=1, possible=1, displayInVrrMode=1

target 8452, primary external DP PA:
  active=1, possible=1, displayInVrrMode=0
```

The primary target remains VRR-capable, Adaptive-Sync is not disabled, and the indicator feature remains enabled. Only its live `displayInVrrMode` bit is cleared. This exactly matches the earlier sticky failure mechanism and agrees with the user's missing indicator and choppy post-Godot control.

No matching display-driver reset event was found in the queried System log window.

## New implication

The visible last-known-good state is not sufficient to select the stable private NVIDIA allocation deterministically. The failed recovery restored:

- mixed external routing (one TB5/DP plus one native HDMI);
- active eDP cloned with HDMI;
- the 1440p clone source;
- DP and HDMI at 120 Hz;
- eDP at 240 Hz;
- Godot Fixed Refresh; and
- a healthy neutral control before Godot.

Godot then still caused the primary target to leave VRR mode permanently until recovery. This rules out the two previously detected configuration drifts as the complete explanation.

One observable difference from the rebooted working capture is source/head assignment. The rebooted working state had target 8452 on Windows source ID 2 and the clone on source ID 0. The failed recovery has target 8452 on source ID 0 and the clone on source ID 2. Source IDs are not normally semantic configuration, and an earlier working pre-reboot capture used a DP target on source ID 0, so this does not prove causality. It does identify the next controlled variable: connector/source allocation, rather than another profile edit.

## Next controlled recovery/test

1. Re-arm live G-SYNC with the verified global off/apply/on/apply cycle.
2. Before any editor, confirm the neutral control works and capture target 8452 returning to `displayInVrrMode=1`.
3. Move the TB5/DP PA to the other TB5 port, leaving the other PA on native HDMI and eDP active/cloned with HDMI.
4. Re-establish the same modes and clone relationship if Windows changes them.
5. Confirm the new external DP target/connector and run the neutral control.
6. Run one Godot Fixed Refresh transition followed by the neutral control.

This tests whether the current connector-1/source-0 allocation is the remaining selector. Do not change profiles, refresh rates, OSD MediaSync, or other variables during this A/B.

