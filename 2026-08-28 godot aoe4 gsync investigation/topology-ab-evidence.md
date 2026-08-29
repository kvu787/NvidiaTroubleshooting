# PA278QGV topology A/B evidence

Date: 2026-08-28 PDT

## User-reported discriminator

Hardware: ASUS ROG Strix G18 `G815LR-IS97`.

```text
Works smoothly:
  one PA278QGV connected to one Thunderbolt 5 port

Reproduces the documented G-SYNC problems:
  two PA278QGVs connected, one to each Thunderbolt 5 port
```

This is interpreted as the same Godot/AoE4 sequence documented in `findings.md`, with external monitor topology as the changed variable.

## Follow-up MediaSync/internal-panel matrix

The user then reported and explicitly clarified three additional configurations. Cases 1 and 2 have two external PAs, one with OSD MediaSync on and one with it off. Case 3 has one external PA with MediaSync on.

| Case | Active external PAs | OSD MediaSync | Internal panel | Godot editor | Unity editor |
|---|---:|---|---|---|---|
| 1 | 2 | one on, one off | connected/active | poor | poor |
| 2 | 2 | one on, one off | disconnected/disabled | poor | poor |
| 3 | 1 | on | connected/active | smooth | smooth |

This is more discriminating than the original A/B:

- The internal panel is not required for the failure: case 2 fails without it.
- Two active displays in total are not sufficient: case 2 has two external displays and fails, while case 3 has one external plus the internal display and succeeds.
- Two OSD-enabled Adaptive-Sync monitors are probably not required: cases 1 and 2 fail with MediaSync off on one PA. This deduction remains provisional until the failing state is captured and NVAPI verifies that the OSD-off monitor no longer reports VRR possible or reports Adaptive-Sync disabled.
- The common tested condition is two active external PA display paths.
- Unity and Godot show the same topology-dependent editor smoothness result. Therefore the poor editor behavior is not specific to Godot's NVIDIA profile writer or rendering engine. Godot's ordinary project-manager profile save/reload remains separately relevant to the three-second display blank and sticky post-Godot VRR failure.

At 19:59 PDT, after case 3, the read-only probes showed the internal target 8449 plus external target 8450/connector instance 0. Both were in VRR-capable display modes; neither had an active VRR request at the idle desktop. The earlier smooth capture used external target 8452/connector instance 1. Thus each external connector has now appeared in a smooth one-external configuration, which makes a single defective port unlikely.

## Earlier one-external-monitor state

This hardware query was taken after target 8450 was removed at 19:05:51 PDT, leaving target 8452 connected. A later good-state capture at 19:59 reversed the external target: 8450 was active and 8452 absent.

### Software and platform

```text
Laptop: ASUS ROG Strix G18 G815LR-IS97
BIOS: G815LR.338, release date 2026-06-03
Windows build: 26200.9168, display version 25H2
NVIDIA GPU: GeForce RTX 5070 Ti Laptop GPU
NVIDIA driver: 596.49, branch r596_25
NVIDIA driver device install time: 2026-08-28 16:30:31 PDT
Intel graphics driver: 32.0.101.8424
USB4 host driver: Microsoft 10.0.26100.8972
```

The original Godot/AoE4 investigation used NVIDIA 616.56. The current driver was installed after the earlier 16:19 pause.

### Active Windows display paths

```text
Active display paths: 2

Internal panel:
  name: NE180QDM-NZC
  NVIDIA adapter LUID: 00000000:0001038F
  Windows target: 8449
  connector: embedded DisplayPort, instance 0
  mode: 2560x1600, 240 Hz
  desktop position: (2560,-155)

External PA278QGV:
  NVIDIA adapter LUID: 00000000:0001038F
  Windows target: 8452
  connector: external DisplayPort, instance 1
  mode: 2560x1440, 119.998 Hz
  desktop position: (0,0)
  target signal: 2560x1440 active, 2720x1525 total, 497.75 MHz pixel rate

Disconnected second PA target:
  Windows target: 8450
  connector: external DisplayPort, instance 0
  current monitor name/path: empty because disconnected
```

Both PA monitor PnP nodes have this parent:

```text
PCI\VEN_10DE&DEV_2F58&SUBSYS_3E881043&REV_A1\50118ACEA42DB04800
```

That is the RTX 5070 Ti Laptop GPU. Target 8450 has PnP address 274 and target 8452 has address 276. They are separate external targets on one NVIDIA adapter.

### Current NVAPI per-display state

From `nvapi-vrr-query.cpp`, built with NVIDIA's official NVAPI SDK:

```text
Internal NE180QDM-NZC, target 8449:
  VRR possible: 1
  display in VRR mode: 1
  Adaptive-Sync disabled: 0
  maximum adaptive-sync frame interval: 9750 us
  DisplayPort: 4 lanes, current 3.24 Gbit/s eDP link rate, 8 bpc

External PA278QGV, target 8452:
  VRR possible: 1
  display in VRR mode: 1
  Adaptive-Sync disabled: 0
  maximum adaptive-sync frame interval: 20583 us (~48.6 Hz)
  DisplayPort: 4 lanes, current/max 5.4 Gbit/s HBR2, 8 bpc
  MST dynamic/root flags: 0/0
```

At the idle desktop both displays reported `VRR requested=0` and `VRR enabled=0`; those two fields describe current presentation activity, not whether the display mode is VRR-capable.

### Current matching Godot profile

The smooth one-external-monitor state is not a no-profile control. DRS currently reports:

```text
Total profiles: 7836
Profile: Godot Engine
Profile predefined: true
Applications:
  godot_v4.4.1-stable_win64.exe
  godot_v4.6.3-stable_win64.exe

Relevant effective settings:
  VRR requested state: disabled
  G-SYNC application override: fixed refresh
  G-SYNC mode: fullscreen only
  OpenGL threaded optimization: disabled
```

This holds the Godot per-app control constant while the second external PA is absent.

## PnP and Intel display events

The Windows device-management log contains simultaneous surprise-removal events for both PA target IDs during the period of connection/reconfiguration tests, including:

```text
16:23:34  UID8450 and UID8452 reported missing on the bus
16:23:43  UID8450 and UID8452 reported missing on the bus
16:37:48  UID8450 and UID8452 reported missing on the bus
16:37:55  UID8450 and UID8452 reported missing on the bus
16:38:06  UID8450 and UID8452 reported missing on the bus
16:43:38  UID8450 and UID8452 reported missing on the bus
18:11:37  UID8452 reported missing on the bus
18:11:39  UID8450 reported missing on the bus
19:05:51  UID8450 reported missing on the bus; UID8452 remains active
```

These events prove that the two external display target nodes were being removed/re-enumerated. Without a user timestamp for every cable or settings action, the earlier pairs cannot be assigned uniquely to physical unplug, driver installation, global G-SYNC apply, or the Godot-induced blank.

`Intel-Gfx-Display-External/GfxDisplayExEventViewer` also logged event 10 errors throughout the reconfiguration period. The event payload identifies Intel graphics version `32.0.101.8424` but uses undocumented numeric parameters. Do not claim a decoded Intel fault from these records.

## USB4/Thunderbolt interpretation

The present device tree contains:

```text
USB4(TM) Host Router (Microsoft)
  PCI VEN_8086 DEV_5781
  driver 10.0.26100.8972

USB4 Root Router (2.0)
USB4 Virtual power coordination device
```

There is no downstream USB4 device router for the PA278QGV. The monitor itself has DisplayPort 1.4 and HDMI inputs, not Thunderbolt. Current evidence therefore describes a DisplayPort output through a TB5-capable USB-C port, not a Thunderbolt monitor/device tunnel.

## Interpretation

High confidence:

- one external PA and two external PAs are distinct NVIDIA target-count/topology states;
- the current one-PA success retains the matching Fixed Refresh Godot profile;
- both PA ports map to separate DP connector instances on the same NVIDIA GPU;
- the external displays are not MST children; and
- the current smooth one-external arm is on NVIDIA 596.49, whereas the original failure was on 616.56.

Leading but not yet proven:

- the failure requires two active external scanout/display heads, not two VRR-enabled monitors and not merely two active displays in total; and
- the sticky state occurs during simultaneous reprogramming/hotplug churn across the two external DP targets.

Remaining version confounder:

- rerun and capture the failing two-external arm while 596.49 remains installed. The A/B report arrived after the driver installation and later PnP events show both targets active/reconfiguring, but the existing records do not timestamp the exact user-visible Godot failure tightly enough to prove which driver was loaded for every arm.

Lower-probability alternatives:

- aggregate link allocation or a Type-C port-mux/Intel display-driver interaction; or
- Windows primary/secondary assignment, rather than external target count, is the hidden variable.

A single defective TB5/USB-C port is now unlikely because target 8452 was smooth as the lone external display in the earlier capture and target 8450 is smooth as the lone external display in the current capture.

## Minimal next A/B matrix

| Case | Physical connections | Windows active displays | Secondary PA Adaptive-Sync | Purpose |
|---|---|---|---|---|
| A | one PA on port 1 | internal + PA | on | Existing smooth control |
| B | one PA on port 2 | internal + PA | on | Rule out a bad individual port |
| C | two PAs | internal + both PAs | on | Existing failure control |
| D | two PAs | internal + both PAs | off | Isolate dual-VRR from dual-display topology |
| E | two PAs | internal + one PA | either | Isolate active target count from physical connection |
| F | two PAs | internal + both PAs at 60 Hz | on | Lower-priority bandwidth/link-allocation test |

For each case capture `display-topology-query.cpp` and `nvapi-vrr-query.cpp` before Godot, while Godot is open, and after Godot closes; then test the AoE4 indicator. Do not change DRS between cases.

## Revised next step

The next capture should be the failing two-external state with one PA's MediaSync off. After changing the OSD setting, power-cycle or reconnect that PA so the display capability is re-enumerated, but do not launch Godot yet. Then:

1. run both read-only probes and verify whether the OSD-off display reports `VRR possible=0` or `Adaptive-Sync disabled=1`;
2. recover G-SYNC once, launch AoE4 before Godot, and record indicator plus subjective/monitor-refresh behavior;
3. disable one PA in Windows while leaving both physically connected, rerun the probes, and repeat the baseline; and
4. if two active external PAs alone are bad, connect one PA by the laptop's HDMI output and the other by one TB5/USB-C DisplayPort output.

Interpretation:

- If Windows-disabling one PA fixes it, active external scanout-head count is the trigger rather than cable presence.
- If TB5+HDMI is smooth while two TB5 outputs are poor, the fault is specific to the dual USB-C/DisplayPort routing path.
- If TB5+HDMI is also poor, the fault is the NVIDIA/Windows two-external-head path more generally.
- Only after the pre-Godot AoE4 baseline is known should the Fixed Refresh Godot transition be repeated; this separates a topology-only G-SYNC quality problem from Godot's known sticky post-transition problem.

## Authoritative references

- [ASUS G815LR-IS97 specifications](https://rog.asus.com/us/laptops/rog-strix/rog-strix-g18-2025/spec/?config=90NR0LC1-M00460)
- [ASUS PA278QGV product specifications](https://www.asus.com/us/displays-desktops/monitors/proart/proart-display-pa278qv-gen2-pa278qgv/)
- [NVIDIA mixed-monitor VRR limitation](https://nvidia.custhelp.com/app/answers/detail/a_id/4766/~/does-variable-refresh-rate-work-across-mixed-monitor-configurations%3F)
- [NVIDIA G-SYNC setup help](https://www.nvidia.com/content/Control-Panel-Help/vLatest/en-us/mergedProjects/nvdsp/To_use_variable_refresh_rates.htm)
- [Microsoft display target/connector structure](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-displayconfig_target_device_name)
- [NVIDIA open-source NVAPI SDK](https://github.com/NVIDIA/nvapi)
