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

## Global-cycle recovery baseline

Captured after the user performed the global G-SYNC off/apply/on/apply cycle and then confirmed that `VsyncStutterTest.exe` was smooth with the indicator. No editor was opened.

The live primary target recovered exactly as predicted:

```text
target 8452, primary external DP PA:
  before global cycle: possible=1, displayInVrrMode=0
  after global cycle:  possible=1, displayInVrrMode=1
```

Targets 8448 and 8449 remained at `displayInVrrMode=1`.

The Windows topology is byte-for-byte equivalent at the semantic level:

```text
source 0 -> target 8452, external DP connector instance 1, 1440p/119.998 Hz
source 2 -> internal target 8449 at 1600p/240 Hz plus HDMI PA at 1440p/119.998 Hz
```

The `Godot Engine` profile still contains the same nine explicit settings, including VRR disabled and G-SYNC Fixed Refresh. The neutral executable remains unprofiled.

Therefore the global cycle repaired only volatile NVIDIA state on the primary DP target. It did not recover by changing the Windows topology, target timing, Godot policy, application association, or DRS profile contents. This is the strongest same-state before/after proof of the live-state diagnosis so far.

The next step is now narrower than originally phrased: physically move only the primary PA's DP-over-USB-C cable to the other TB5 port, leave native HDMI and every software/OSD setting untouched, and capture the raw post-hotplug topology before normalizing anything Windows may change.

## Other-TB5-port raw capture

The user moved only the primary PA's USB-C/DisplayPort cable to the laptop's other TB5 port. Native HDMI, eDP clone membership, G-SYNC, MediaSync, profiles, refresh rates, and Windows display settings were untouched. No 3D application ran before capture.

The public topology did not change at all:

```text
primary DP PA: target 8452, connector instance 1, source ID 0
  2560x1440 at 119.998 Hz

internal eDP: target 8449, source ID 2
  shared 2560x1440 source, target 2560x1600 at 240 Hz

native-HDMI PA: target 8448 clone path, source ID 2
  2560x1440 at 119.998 Hz
```

All targets remained `displayInVrrMode=1`, and the nine-setting Godot profile remained unchanged.

This means the two physical TB5 jacks do not map one-to-one to persistent NVIDIA connector IDs when only one external DP target is active. The driver assigned the moved display back to target 8452/connector instance 1/source 0. Earlier simultaneous dual-TB5 operation still enumerated two logical connectors, but a single-DP hotplug does not let us select connector 0 merely by moving the cable.

The move nevertheless changed the physical jack while preserving every public state field, making the next runtime transition a clean physical-jack A/B. Run a neutral pre-control, one ordinary Godot Fixed Refresh transition, and an immediate neutral post-control without any intervening change. If the failure repeats, physical jack alone is ruled out. If it succeeds, a hidden route difference exists below the public connector/source representation.

## Other-TB5-port runtime result

The neutral pre-control was smooth and showed the G-SYNC indicator.

Godot then opened on the secondary native-HDMI-plus-eDP clone smoothly, without a monitor blank. When the user moved the running editor window onto the primary DP PA, both monitors underwent a roughly three-second blank. Godot remained smooth without the G-SYNC indicator afterward and was closed. The immediate neutral control was choppy and did not show the indicator.

The preserved failed-state capture again reports:

```text
target 8448, HDMI PA:     possible=1, displayInVrrMode=1
target 8449, internal:    possible=1, displayInVrrMode=1
target 8452, primary DP:  possible=1, displayInVrrMode=0
```

Topology, timings, clone membership, and the nine-setting Godot Fixed Refresh profile are unchanged. No matching display-driver reset event was found.

This result rules out the physical TB5 jack as a sufficient discriminator under the current single-DP logical assignment. More importantly, it localizes the runtime transition more narrowly than process launch or exit: the editor was initially harmless on the HDMI clone, and the disruptive transition occurred when the Fixed Refresh Godot window entered the G-SYNC-capable primary DP target.

The strongest next A/B is placement-only:

1. recover global G-SYNC and verify the neutral control;
2. launch Godot on the HDMI clone;
3. keep the editor entirely on that clone and close it there; and
4. immediately retest the neutral application on primary DP.

If later G-SYNC remains healthy, crossing a Fixed Refresh editor surface onto the VRR primary is necessary in this current allocation. If failure still occurs, launch/exit processing can poison the DP target even without presenting the editor on it.

## Placement-only success

The user saved Godot's editor placement on the HDMI clone while the system was already failed, recovered global G-SYNC, verified a healthy neutral pre-control, and then reopened Godot. The editor remained entirely on the HDMI clone and was closed there. The immediate primary-DP neutral control was smooth and showed the indicator.

The read-only post-state confirms:

```text
target 8448, HDMI PA:     possible=1, displayInVrrMode=1
target 8449, internal:    possible=1, displayInVrrMode=1
target 8452, primary DP:  possible=1, displayInVrrMode=1
```

Topology, 120/120/240-Hz target timings, clone membership, and the nine-setting Godot profile are semantically identical to both failure arms. Godot again wrote the DRS databases, this time at 04:06:08 and 04:06:40, yet the effective settings remained unchanged and primary VRR survived.

This is a decisive within-allocation placement A/B:

| Godot presentation | Transition | Later primary G-SYNC |
| --- | --- | --- |
| Opens on HDMI clone, then moves onto primary DP | Three-second blank on crossing | Fails; target 8452 mode `0` |
| Opens, remains, and closes on HDMI clone | No damaging DP transition | Works; target 8452 mode `1` |

In the current connector-1/source-0 allocation, presenting the Fixed Refresh editor on the G-SYNC-capable primary DP target is necessary for the reproduced sticky failure. Process launch, project open, DRS save/reload, use on the HDMI clone, process exit, physical TB5 jack, and the visible topology are each insufficient without that DP presentation step.

Operational workaround for this allocation: persist Godot on the HDMI clone and do not move any part of the editor onto the primary DP display. A second same-placement repetition and a reboot-persistence test would establish durability; the current single success establishes mechanism and immediate viability.
