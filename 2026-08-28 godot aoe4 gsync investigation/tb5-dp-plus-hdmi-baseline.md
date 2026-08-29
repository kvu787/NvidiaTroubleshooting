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
