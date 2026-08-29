# Two-external mixed-MediaSync baseline

Capture time: 2026-08-28 22:30:21 PDT

State prepared by the user:

- both external PA278QGVs connected and active;
- one PA OSD MediaSync on;
- the other PA OSD MediaSync off and power-cycled or reconnected after the change;
- internal laptop display connected and active; and
- Godot not opened.

## Result

Windows reports three active paths on the same RTX 5070 Ti Laptop GPU:

| Target | Display | Connector | Mode |
|---:|---|---|---|
| 8450 | PA278QGV | external DP instance 0 | 2560x1440 at 119.998 Hz |
| 8452 | PA278QGV | external DP instance 1 | 2560x1440 at 119.998 Hz |
| 8449 | NE180QDM-NZC internal panel | embedded DP instance 0 | 2560x1600 at 240 Hz |

NVIDIA NVAPI reports:

| Target | NV display ID | VRR possible | Display in VRR mode | Adaptive-Sync disabled override | Maximum frame interval |
|---:|---:|---:|---:|---:|---:|
| 8450 PA | `0x80061081` | 1 | 1 | 0 | 20583 us |
| 8452 PA | `0x80061087` | 0 | 0 | 0 | 0 us |
| 8449 internal | `0x80061082` | 1 | 1 | 0 | 9750 us |

The idle desktop had no active VRR request on any display (`requested=0`, `enabled=0`), which is expected without a presenting VRR application.

The OSD-off PA is target 8452. `VRR possible=0`, `displayInVrrMode=0`, and a zero maximum Adaptive-Sync frame interval prove that the OSD MediaSync change reached the NVIDIA driver. `Adaptive-Sync disabled override=0` does not contradict that result: the panel stopped advertising VRR capability, while NVIDIA did not add a separate driver override.

## Interpretation

This closes the earlier MediaSync-verification caveat. The user's poor Unity/Godot editor behavior with this topology cannot require two VRR-enabled external monitors: only target 8450 is VRR-capable, while target 8452 is a fixed-refresh external display.

Combined with the user's case 2, which remains poor with the internal panel disabled, the strongest common condition is two active external display heads. The remaining immediate question is whether AoE4 is already poor in this pre-Godot/pre-Unity state or becomes poor only after an editor/profile transition.

## AoE4 pre-editor control

The user launched AoE4 on the MediaSync-on PA without opening Godot or Unity and without toggling G-SYNC. AoE4 showed the top-right G-SYNC indicator and G-SYNC appeared to work normally. The user then closed AoE4.

A read-only post-AoE capture at 22:36 PDT showed the topology and capability state unchanged:

- three active paths: PA targets 8450 and 8452 plus internal target 8449;
- target 8450 remained VRR-capable and in a VRR-capable display mode;
- target 8452 remained non-VRR and outside VRR mode; and
- no VRR request remained active at the idle desktop after AoE4 closed.

This proves two active external heads do not inherently prevent G-SYNC from working in AoE4. The poor Unity/Godot editor behavior is presentation-path/application-class dependent. The next separation is Unity editor followed immediately by AoE4: if AoE4 remains good, Unity's problem is confined to its own windowed/editor presentation path; if AoE4 loses G-SYNC, a non-Godot editor can also poison the later driver state.

## Raw Windows topology output

```text
Active display paths: 3

Path 0
  source adapter LUID: 00000000:00010346
  target adapter LUID: 00000000:00010346
  source id: 0
  target id: 8450
  path flags: 0x1
  output technology: DisplayPort external (10)
  target available: true
  refresh: 119998/1000 Hz
  source mode: 2560x1440 at (0,0), pixel format enum 4
  target signal active/total: 2560x1440 / 2720x1525, pixel rate 497750000
  GDI source: \\.\DISPLAY1
  GDI adapter: NVIDIA GeForce RTX 5070 Ti Laptop GPU | PCI\VEN_10DE&DEV_2F58&SUBSYS_3E881043&REV_A1
  monitor friendly name: PA278QGV
  connector instance: 0

Path 1
  source adapter LUID: 00000000:00010346
  target adapter LUID: 00000000:00010346
  source id: 1
  target id: 8452
  path flags: 0x1
  output technology: DisplayPort external (10)
  target available: true
  refresh: 119998/1000 Hz
  source mode: 2560x1440 at (2560,0), pixel format enum 4
  target signal active/total: 2560x1440 / 2720x1525, pixel rate 497750000
  GDI source: \\.\DISPLAY2
  GDI adapter: NVIDIA GeForce RTX 5070 Ti Laptop GPU | PCI\VEN_10DE&DEV_2F58&SUBSYS_3E881043&REV_A1
  monitor friendly name: PA278QGV
  connector instance: 1

Path 2
  source adapter LUID: 00000000:00010346
  target adapter LUID: 00000000:00010346
  source id: 2
  target id: 8449
  path flags: 0x1
  output technology: DisplayPort embedded (11)
  target available: true
  refresh: 240/1 Hz
  source mode: 2560x1600 at (5120,-162), pixel format enum 4
  target signal active/total: 2560x1600 / 2720x1800, pixel rate 1175040000
  GDI source: \\.\DISPLAY3
  GDI adapter: NVIDIA GeForce RTX 5070 Ti Laptop GPU | PCI\VEN_10DE&DEV_2F58&SUBSYS_3E881043&REV_A1
  monitor friendly name: NE180QDM-NZC
  connector instance: 0
```

## Raw NVAPI output

```text
Driver: status=0 version=59649 branch=r596_25
Physical GPUs: 1

GPU 0: NVIDIA GeForce RTX 5070 Ti Laptop GPU
  Connected display IDs: 3

  Display 0
    NV display ID: 0x80061082
    active=1 osVisible=1 connected=1 physicallyConnected=1 dynamicMst=0 mstRoot=0
    display ID info: status=0 adapterLuid=00000000:00010346 targetId=8449 name=NE180QDM-NZC
    VRR info: status=0 possible=1 requested=0 enabled=0 displayInVrrMode=1 indicatorEnabled=1
    Adaptive-Sync data: status=0 disabled=0 frameSplittingDisabled=0 maxFrameIntervalUs=9750 lastFlipRefreshCount=87

  Display 1
    NV display ID: 0x80061081
    active=1 osVisible=1 connected=1 physicallyConnected=1 dynamicMst=0 mstRoot=0
    display ID info: status=0 adapterLuid=00000000:00010346 targetId=8450 name=PA278QGV
    VRR info: status=0 possible=1 requested=0 enabled=0 displayInVrrMode=1 indicatorEnabled=1
    Adaptive-Sync data: status=0 disabled=0 frameSplittingDisabled=0 maxFrameIntervalUs=20583 lastFlipRefreshCount=13

  Display 2
    NV display ID: 0x80061087
    active=1 osVisible=1 connected=1 physicallyConnected=1 dynamicMst=0 mstRoot=0
    display ID info: status=0 adapterLuid=00000000:00010346 targetId=8452 name=PA278QGV
    VRR info: status=0 possible=0 requested=0 enabled=0 displayInVrrMode=0 indicatorEnabled=1
    Adaptive-Sync data: status=0 disabled=0 frameSplittingDisabled=0 maxFrameIntervalUs=0 lastFlipRefreshCount=7
```
