# TB5/DisplayPort plus HDMI baseline

Captured: 2026-08-28 23:11 PDT

No Unity, Godot, or `VsyncStutterTest.exe` process was opened after the routing change and before this capture.

## Intended state

- MediaSync-on PA remains connected through one TB5/USB-C DisplayPort output.
- MediaSync-off PA moved from the other TB5/USB-C DisplayPort output to the laptop's native HDMI output.
- Both external PAs and the internal panel are active in Windows.
- Global G-SYNC was not toggled.

## Windows active paths

All three paths are owned by the NVIDIA GeForce RTX 5070 Ti Laptop GPU at adapter LUID `00000000:00010346`.

| Target | Display | Route | Mode | Position |
|---|---|---|---|---|
| 8449 | NE180QDM-NZC internal panel | embedded DisplayPort | 2560x1600 at about 240 Hz | `(2560,-155)` |
| 8450 | PA278QGV | external DisplayPort through TB5/USB-C | 2560x1440 at 119.998 Hz | `(0,0)` |
| 8448 | PA278QGV | native HDMI | 2560x1440 at 59.951 Hz | `(5120,0)` |

The HDMI PA is a new Windows/NVIDIA target (`8448`), replacing the previous external-DisplayPort target `8452`. Neither external display is an MST child.

## NVIDIA read-only query

Driver: 596.49, branch `r596_25`.

```text
target 8448, HDMI PA:
  active=1 osVisible=1 connected=1 physicallyConnected=1
  VRR possible=1 displayInVrrMode=1 indicatorEnabled=1
  Adaptive-Sync disabled=0 maxFrameIntervalUs=20583
  isDp=0

target 8449, internal panel:
  active=1 osVisible=1 connected=1 physicallyConnected=1
  VRR possible=1 displayInVrrMode=1 indicatorEnabled=1

target 8450, TB5/DisplayPort PA:
  active=1 osVisible=1 connected=1 physicallyConnected=1
  VRR possible=1 displayInVrrMode=1 indicatorEnabled=1
  Adaptive-Sync disabled=0 maxFrameIntervalUs=20583
  DP 4 lanes at HBR2
```

The HDMI result is unexpected: the user left MediaSync off in that PA's OSD, but NVIDIA reports target 8448 as VRR-possible and in its VRR display mode. In the earlier dual-DisplayPort topology, the MediaSync-off PA target 8452 reported `possible=0`, `displayInVrrMode=0`, and a zero Adaptive-Sync interval. Preserve this as an observed input/route or driver-state difference; do not infer that the user's OSD setting was changed.

The user subsequently rechecked the HDMI PA's OSD and confirmed `MediaSync=off`. The current probes cannot directly read that OSD control. `NvAPI_Disp_GetVRRInfo` reports NVIDIA's assessment of the target, while `NvAPI_DISP_GetAdaptiveSyncData` reports NVIDIA's per-display Adaptive-Sync state. In particular, its `bDisableAdaptiveSync` and `maxFrameInterval` fields are paired with an NVAPI setter and are not documented as a raw monitor-OSD query. Therefore the OSD observation and the NVIDIA state must be recorded as two distinct facts.

This does not invalidate the physical-route A/B. It means the new arm tests TB5/DisplayPort plus HDMI with two NVIDIA-reported VRR-possible external targets, rather than reproducing the earlier mixed-capability state exactly.

## DRS baseline

Cable routing did not rewrite the NVIDIA driver-profile database:

```text
nvdrsdb0.bin  last write 22:50:13  SHA-256 163CE7E0C26D5B3C136AC6514464A52A2E1660951D712D9D58D0F97E20050E85
nvdrsdb1.bin  last write 22:50:13  SHA-256 929E7C1F106022BF0BCC531D5FE4C6786984DB776B9D6894ED149C5F22FB99DC
nvdrssel.bin  last write 22:50:13  SHA-256 4BF5122F344554C53BDE2EBB8CD2B7E3D1600AD631C385A5D7CCE23C7785459A
```

## Next action

Establish a functional pre-Unity baseline by running `VsyncStutterTest.exe` on target 8450, the MediaSync-on TB5/DisplayPort PA. If the G-SYNC indicator is present and animation is smooth, proceed to the same Unity Fixed Refresh transition without changing topology or settings.

## Functional pre-Unity control

The user ran `VsyncStutterTest.exe` on target 8450. It displayed the top-right G-SYNC indicator and ran smoothly.

The 23:18 post-control capture is unchanged:

```text
Windows active paths: targets 8449 internal, 8450 TB5/DisplayPort PA, 8448 HDMI PA
target 8450: VRR possible=1, displayInVrrMode=1
target 8448: VRR possible=1, displayInVrrMode=1
DRS timestamps and SHA-256 hashes: identical to the 23:11 baseline
```

This proves the TB5/DisplayPort-plus-HDMI topology is healthy before Unity. The decisive next transition is `Unity Fixed Refresh -> close Unity -> VsyncStutterTest.exe` on target 8450 without any intervening settings or topology change.

## Unity transition result

Unity produced no monitor blink, behaved acceptably, and did not show G-SYNC as intended by its Fixed Refresh profile. After Unity closed, `VsyncStutterTest.exe` still displayed the G-SYNC indicator and ran smoothly on target 8450.

The 23:20 post-transition capture remained healthy and unchanged:

```text
Windows active paths: targets 8449 internal, 8450 TB5/DisplayPort PA, 8448 HDMI PA
target 8450: VRR possible=1, displayInVrrMode=1
target 8448: VRR possible=1, displayInVrrMode=1
DRS timestamps and SHA-256 hashes: identical to the 23:11 and 23:18 baselines
```

This is decisive on one point: two active external NVIDIA scanout heads are necessary in the earlier failure arms but are not sufficient. Substituting native HDMI for the second TB5/USB-C DisplayPort route prevents both the Fixed Refresh launch blank and the sticky loss of later G-SYNC.

It does not yet prove that connector route alone is causal. The move also changed the secondary PA from 119.998 Hz over DisplayPort to 59.951 Hz over HDMI, created a new target ID, and changed NVIDIA's VRR classification despite the unchanged OSD setting. The successful configuration may therefore depend on the HDMI driver path, lower secondary refresh/link load, or another route-associated mode-state difference. A future HDMI-at-120-Hz test, if available, or dual-TB5/DisplayPort-at-60-Hz test can separate those factors.

## Godot pre-validation state

A read-only DRS audit after the successful Unity transition finds exactly one matching profile:

```text
profile: Godot Engine
application: godot_v4.6.3-stable_win64.exe
VRR requested state: disabled
G-SYNC application override: fixed refresh
G-SYNC mode: fullscreen only
OpenGL threaded optimization: disabled
```

This ensures the final Godot test will exercise the intended Fixed Refresh profile. Run the original Godot workflow without the explicit D3D12 bypass, close Godot, and immediately validate `VsyncStutterTest.exe` on target 8450.

## Godot validation result

The user performed the original Godot workflow twice without changing G-SYNC, topology, refresh rates, or OSD settings.

First sequence:

- Godot 4.6.3 opened the project editor on the internal laptop display rather than the primary TB5/DisplayPort PA;
- the monitors blanked for about two seconds;
- the user moved the editor to the primary PA and closed Godot; and
- the immediate `VsyncStutterTest.exe` control on the primary PA displayed the G-SYNC indicator and ran smoothly.

Second sequence:

- the project editor opened on the primary TB5/DisplayPort PA;
- there was no reported monitor blank;
- Godot showed no G-SYNC indicator and editor usage was smooth;
- after Godot closed, the immediate `VsyncStutterTest.exe` control again displayed the indicator and ran smoothly.

The 23:28 post-test capture shows:

```text
target 8450, primary TB5/DisplayPort PA:
  VRR possible=1, displayInVrrMode=1

target 8448, MediaSync-off HDMI PA:
  VRR possible=1, displayInVrrMode=0

target 8449, internal panel:
  VRR possible=1, displayInVrrMode=1

all three Windows paths unchanged
```

The primary target therefore remains healthy after both Godot Fixed Refresh transitions. The HDMI target changed from `displayInVrrMode=1` in the pre-Godot baseline to `0`, which is consistent with its user-confirmed OSD MediaSync-off state but does not alter the working primary control.

Godot did rewrite DRS during both launches:

```text
nvdrsdb0.bin last write: 23:24:38
nvdrsdb1.bin last write: 23:25:09
```

The two database hashes changed, while a read-only query shows the effective Godot profile is semantically unchanged: VRR requested state disabled, G-SYNC Fixed Refresh, fullscreen-only G-SYNC mode, and threaded optimization disabled. The two distinct timestamps closely match the two launches and are consistent with Godot 4.6.3's unconditional `NvAPI_DRS_SaveSettings()` path.

This proves the DRS save alone is not sufficient for either outcome: the second save caused no reported blank, and neither save caused sticky loss of later G-SYNC. The remaining first-launch blank is confounded with launch-display placement and cold/warm transition state. Deliberately persisting the editor on the internal display and reopening it is the next direct isolation.
