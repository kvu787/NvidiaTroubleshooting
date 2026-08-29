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
- Two NVIDIA-reported VRR-capable external targets are not required: cases 1 and 2 fail with MediaSync off on one PA, and NVAPI independently reported that DisplayPort target as non-VRR in the failing state. NVAPI did not read the OSD setting itself.
- The common tested condition is two active external PA display paths.
- Unity and Godot show the same topology-dependent editor result. Unity's existing Fixed Refresh profile also reproduced the monitor blank and sticky loss of later G-SYNC without writing DRS. Therefore neither the poor editor behavior nor the persistent transition failure is specific to Godot's profile writer or rendering engine.

The requested bad-topology baseline was captured at 22:30 PDT after the user confirmed MediaSync off, reconnected/power-cycled that PA, and before Godot opened. NVAPI identifies target 8450 as VRR-capable (`possible=1`, `displayInVrrMode=1`, maximum interval 20583 us) and target 8452 as non-VRR (`possible=0`, `displayInVrrMode=0`, maximum interval 0). This shows the driver's classification was consistent with the user-observed OSD setting on DisplayPort, but it is not a direct OSD read. Two NVIDIA-reported VRR-capable external targets are conclusively not required for the reported poor editor behavior.

AoE4 was then tested before either editor, without toggling G-SYNC. It showed the G-SYNC indicator and behaved normally. The 22:36 post-AoE probes showed unchanged topology/capabilities. Therefore two active external heads do not globally break G-SYNC; they interact poorly with the tested editor presentation paths and/or later application transitions.

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

Driver-version result:

- the original Godot failure was captured on 616.56, and the Unity Fixed Refresh transition reproduced the same sticky failure on 596.49. The exact Godot sequence need not be repeated to establish that the underlying driver/topology defect crosses these two branches.

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
| E | two PAs | internal + one PA | off on inactive PA | Smooth; proves active external target count, not physical presence |
| F | two PAs | internal + both PAs at 60 Hz | on | Lower-priority bandwidth/link-allocation test |
| G | one PA on TB5/DP, one on HDMI | internal + both PAs | OSD off on HDMI PA; NVAPI still reports VRR possible | Smooth before and after Unity Fixed Refresh; isolates dual-TB5/DP route |

Cases A-E are complete. Case E left both cables connected and monitors powered, but disabled the MediaSync-off PA in Windows. Unity Fixed Refresh caused no blink and the post-Unity `VsyncStutterTest.exe` control retained G-SYNC. This proves two active external scanout heads are required.

## Completed baseline and next step

The active-head-count isolation is complete. Case G remained healthy through `Unity Fixed Refresh -> VsyncStutterTest.exe`, with no blink, no sticky loss, unchanged DRS, and target 8450 still in VRR mode. Two active external heads are not sufficient. Case G changed both the secondary route and its refresh rate (119.998-Hz DisplayPort to 59.951-Hz HDMI), so route alone is not isolated. Validate the original Godot workflow in this working configuration next.

The Godot workflow then preserved correct per-application switching twice. The first project editor opened on the internal panel and caused a two-second blank; the second opened on the primary PA and caused none. Both later neutral controls retained G-SYNC. Godot saved DRS during both launches, so its save is not sufficient for the blank or sticky failure. Case G now needs a controlled internal-panel window-placement repetition, not another uncontrolled Godot launch.

That controlled repetition is complete. Two further Godot launches, including a deliberately persisted internal-panel launch, produced no blank; Godot remained smooth without G-SYNC and the later control retained smooth G-SYNC. Case G is a validated stable-use workaround. The isolated earlier blank is not explained by launch placement or the repeated DRS save and is best classified as an unreproduced first/cold topology transition.

Post-reboot, case G was refined: both external monitors run at 119.998 Hz and remain smooth with internal eDP active, ruling out the earlier 60-Hz secondary as necessary. Windows-disconnecting internal eDP produces poor behavior and reconnecting it restores smoothness; HDMI MediaSync on/off did not change the result. Add an internal-off pre-application capture and neutral-control step before any further editor transition.

The internal-off pre-application capture is complete: only targets 8450 DP and 8448 HDMI are active at 119.998 Hz, internal 8449 remains physically connected/inactive, and DRS is unchanged. The primary DP public VRR query returns generic error, but that error is known to coexist with a healthy functional control. The pre-editor `VsyncStutterTest.exe` result is pending.

The pre-editor control succeeded with the indicator and smooth motion. Internal-panel disconnection alone is not sufficient. The next arm is the same two-external-only topology through `Unity Fixed Refresh -> VsyncStutterTest.exe`, which isolates profile activation without a DRS write.

That Unity arm produced severe repeated display blinking twice, but both post-Unity neutral controls retained smooth G-SYNC. Final topology/state and DRS are unchanged, with no logged driver reset. The internal-off symptom is transient modeset instability rather than sticky failure. Next compare the same internal-off topology with HDMI lowered from 120 Hz to 60 Hz.

The internal-off 120/60 baseline is captured. Lowering only HDMI to 59.951 Hz restores a successful primary DP VRR query with `displayInVrrMode=1`; routes and DRS are unchanged. This directly implicates mode/clock/resource allocation. The neutral functional control and Unity transition are pending.

The neutral control succeeded and left both external targets queryable in VRR mode. The 120/60 Unity transition is ready.

The 120/60 Unity transition then produced sticky failure: after one initial blink, both later neutral controls were choppy without the indicator. Primary target 8450 changed from `displayInVrrMode=1` to `0`; HDMI and DRS remained unchanged. Lower refresh does not solve the two-external-only problem. It changes the manifestation from 120/120 repeated transient blanks with recovery to 120/60 sticky VRR loss. Reconnect internal eDP without a global toggle to test topology-only recovery.

Interpretation:

- Windows-disabling one PA fixed it; active external scanout-head count is confirmed as the trigger rather than cable presence.
- TB5+HDMI at the current 120-Hz/60-Hz split is smooth while two 120-Hz TB5 outputs are poor, isolating a route/mode family but not route alone.
- Later, test HDMI at 120 Hz if exposed, or both TB5/DisplayPort PAs at 60 Hz, to separate connector route from refresh/link load.
- Unity has already separated profile activation from Godot's writer: an existing Fixed Refresh profile is sufficient.

## Authoritative references

- [ASUS G815LR-IS97 specifications](https://rog.asus.com/us/laptops/rog-strix/rog-strix-g18-2025/spec/?config=90NR0LC1-M00460)
- [ASUS PA278QGV product specifications](https://www.asus.com/us/displays-desktops/monitors/proart/proart-display-pa278qv-gen2-pa278qgv/)
- [NVIDIA mixed-monitor VRR limitation](https://nvidia.custhelp.com/app/answers/detail/a_id/4766/~/does-variable-refresh-rate-work-across-mixed-monitor-configurations%3F)
- [NVIDIA G-SYNC setup help](https://www.nvidia.com/content/Control-Panel-Help/vLatest/en-us/mergedProjects/nvdsp/To_use_variable_refresh_rates.htm)
- [Microsoft display target/connector structure](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-displayconfig_target_device_name)
- [NVIDIA open-source NVAPI SDK](https://github.com/NVIDIA/nvapi)
