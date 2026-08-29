# Internal-eDP plus HDMI clone baseline

Captured: 2026-08-29 00:47 PDT

## User action

Starting from the recovered three-display state, the user selected `Duplicate desktop on 1 and 3`, applied it, and kept the changes. No G-SYNC, refresh-rate, OSD, or 3D-application action followed before this capture.

The preceding Windows layout mapped the display numbers as follows:

```text
display 2: primary TB5/DisplayPort PA
display 3: secondary HDMI PA
display 1: internal laptop panel
```

## Windows path result

`QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS)` returns three active physical target paths but only two distinct source IDs:

```text
source id 0, \\.\DISPLAY1, source mode 2560x1440 at (0,0)
  -> target 8450, PA278QGV, external DisplayPort
  -> target signal 2560x1440 at 119.998 Hz

source id 2, \\.\DISPLAY3, source mode 2560x1600 at (2560,0)
  -> target 8449, NE180QDM-NZC, embedded DisplayPort
  -> target signal 2560x1600 at approximately 240 Hz

source id 2, \\.\DISPLAY3, same source mode 2560x1600 at (2560,0)
  -> PA278QGV, HDMI
  -> target signal 2560x1440 at 119.998 Hz
```

The shared source ID, GDI source name, mode, and desktop position prove that internal eDP and the HDMI PA are cloned. The primary TB5/DisplayPort PA remains a separate Windows desktop source.

The DisplayConfig clone path reports target ID `0x04002100` for the HDMI path, while the explicit physical-target lookup and NVAPI continue to identify that PA as target 8448 (`0x2100`). Treat `0x04002100` as a clone-path representation detail, not a newly connected monitor.

Windows selected a 2560x1600 clone source but transmits a 2560x1440 signal to the HDMI PA. This can require scaling, cropping, or letterboxing on the HDMI copy and should be checked for usability separately from the VRR investigation.

## NVIDIA state

Driver: 596.49, branch `r596_25`.

```text
target 8448, HDMI PA278QGV:
  active=1, osVisible=1
  VRR possible=1
  displayInVrrMode=1

target 8449, internal eDP:
  active=1, osVisible=1
  VRR possible=1
  displayInVrrMode=1

target 8450, primary TB5/DisplayPort PA278QGV:
  active=1, osVisible=1
  VRR possible=1
  displayInVrrMode=1
```

All three physical outputs remain active from NVIDIA's perspective. Clone mode reduces the usable Windows desktop to two source spaces without deactivating eDP, which is exactly the intended workaround mechanism.

## Persistent driver settings

The DRS database is unchanged from the pre-clone recovered state:

```text
nvdrsdb0.bin
  last write: 2026-08-29 00:05:51 PDT
  SHA-256: F832B6C5A1B096BD6036B0B3BA80786B428011E004BF4FDFE6A5962B46268B6E

nvdrsdb1.bin
  last write: 2026-08-29 00:04:07 PDT
  SHA-256: 7F46DDF955B81BB5DC2B9BF9DA3C7950523807C008FCB5FA245C0D32281E5AF1

nvdrssel.bin
  last write: 2026-08-29 00:05:51 PDT
  SHA-256: 6E340B9CFFB37A989CA544E6BB780A2C78901D3FB33738768511A30617AFA01D
```

## Interpretation and next test

The requested clone topology exists and its pre-application state is healthy. It preserves the active internal scanout path while eliminating the hidden third desktop source. No persistent NVIDIA setting changed.

Run `VsyncStutterTest.exe` on the separate primary TB5/DisplayPort PA before opening Unity or Godot. If that neutral control remains smooth with the indicator, repeat the Unity Fixed Refresh transition and immediately retest the neutral application.

## Revised native-HDMI clone source

Captured: 2026-08-29 00:50 PDT

The HDMI PA was letterboxed with the automatically selected 2560x1600 clone source. The user changed only the duplicated 1|3 desktop resolution to 2560x1440 and opened no 3D application before this capture.

The clone relationship and paths remain unchanged, but the shared source mode is now native to the HDMI PA:

```text
source id 0, \\.\DISPLAY1, source mode 2560x1440 at (0,0)
  -> target 8450, primary external DisplayPort PA
  -> target signal 2560x1440 at 119.998 Hz

source id 2, \\.\DISPLAY3, source mode 2560x1440 at (2560,0)
  -> target 8449, internal embedded DisplayPort
  -> target signal 2560x1600 at approximately 240 Hz

source id 2, \\.\DISPLAY3, same source mode 2560x1440 at (2560,0)
  -> target 8448 physical HDMI PA
  -> target signal 2560x1440 at 119.998 Hz
```

The HDMI copy now has a matching 2560x1440 source and target signal. Any aspect-ratio accommodation is moved to the hidden 2560x1600 internal panel, which remains an active scanout target.

NVAPI remains healthy and unchanged in the relevant state:

```text
target 8448 HDMI:       active=1, osVisible=1, possible=1, displayInVrrMode=1
target 8449 internal:   active=1, osVisible=1, possible=1, displayInVrrMode=1
target 8450 primary DP: active=1, osVisible=1, possible=1, displayInVrrMode=1
```

DRS timestamps, sizes, and SHA-256 hashes are identical to the earlier clone and recovered-three-display baselines. The resolution change is purely a Windows source-mode/topology adjustment and did not edit persistent NVIDIA settings.

Use this 2560x1440 clone state as the authoritative pre-application baseline for the neutral control and later Unity transition.

## Neutral functional control

Captured: 2026-08-29 00:54 PDT

The user ran `VsyncStutterTest.exe` on the separate primary TB5/DisplayPort PA. It ran smoothly and displayed the G-SYNC indicator.

The post-control capture is identical in every relevant respect to the 00:50 baseline:

```text
Windows paths: three target paths, two source IDs
clone source 2: 2560x1440, internal eDP plus HDMI
separate source 0: 2560x1440, primary TB5/DisplayPort PA

target 8448 HDMI:       displayInVrrMode=1
target 8449 internal:   displayInVrrMode=1
target 8450 primary DP: displayInVrrMode=1

DRS timestamps and SHA-256 hashes: unchanged
```

The 1440p clone mode therefore preserves functional G-SYNC before an editor transition. The decisive next sequence is Unity Fixed Refresh, followed immediately by the same neutral control.

## Unity Fixed Refresh transition

Captured: 2026-08-29 00:57 PDT

In the unchanged 1440p clone topology, the user opened the same Unity project on the separate primary TB5/DisplayPort PA. Unity opened, ran smoothly without a G-SYNC indicator as intended by its Fixed Refresh profile, and closed with zero monitor blinks. The immediate `VsyncStutterTest.exe` control then ran smoothly and displayed the G-SYNC indicator.

The post-transition capture remains identical to the baseline:

```text
Windows paths: three target paths, two source IDs
clone source 2: 2560x1440, internal eDP plus HDMI
separate source 0: 2560x1440, primary TB5/DisplayPort PA

target 8448 HDMI:       displayInVrrMode=1
target 8449 internal:   displayInVrrMode=1
target 8450 primary DP: displayInVrrMode=1

DRS timestamps and SHA-256 hashes: unchanged
```

This is the decisive workaround result. The internal eDP target does not need its own extended desktop source to stabilize the NVIDIA Fixed Refresh transition; it only needs to remain an active scanout target in the tested configuration. Cloning it with HDMI eliminates the unusable hidden third desktop while retaining correct `Fixed Refresh editor -> G-SYNC application` switching and eliminating transition blanks.

Unity isolates the NVIDIA transition, but the exact original workflow still needs one final validation: ordinary Godot 4.6.3 project-manager launch, project open, close, and immediate neutral control in this unchanged clone topology.

## Ordinary Godot project-manager validation

Captured: 2026-08-29 01:01 PDT

First sequence:

- Godot 4.6.3 was launched normally and the project was opened through the project manager;
- the project editor opened on the cloned HDMI+internal desktop;
- a two-second monitor blink occurred;
- the user moved the editor to the primary TB5/DisplayPort PA and closed Godot; and
- the immediate `VsyncStutterTest.exe` control ran smoothly with the G-SYNC indicator.

Second sequence:

- Godot 4.6.3 was again launched normally and the project was opened;
- the editor opened on the primary PA;
- no G-SYNC indicator appeared in Godot and editor use was smooth;
- no blink was reported; and
- the second immediate neutral control again ran smoothly with the G-SYNC indicator.

The post-state remains healthy:

```text
Windows paths: three target paths, two source IDs
clone source 2: 2560x1440, internal eDP plus HDMI
separate source 0: 2560x1440, primary TB5/DisplayPort PA

target 8448 HDMI:       displayInVrrMode=1
target 8449 internal:   displayInVrrMode=1
target 8450 primary DP: displayInVrrMode=1
```

Godot performed its expected persistent DRS saves:

```text
nvdrsdb1.bin last write: 00:59:06
SHA-256: CE34F421D37411CD98F83251339D14362331EC6BD0502B8013A4958B50B28647

nvdrsdb0.bin last write: 00:59:30
SHA-256: BE3B9B53B568388C366A02136F1DE18D50C5634AE488D4862ACE72665618DDDF

nvdrssel.bin: unchanged
SHA-256: 6E340B9CFFB37A989CA544E6BB780A2C78901D3FB33738768511A30617AFA01D
```

The read-only exhaustive current-driver audit finds one matching `Godot Engine` profile with two application associations:

```text
godot_v4.4.1-stable_win64.exe
godot_v4.6.3-stable_win64.exe
```

Profile enumeration completed without failures. This is the first profile-count audit after moving to driver 596.49, so its total of 7837 profiles must not be compared directly to the earlier 7957-profile 616.56 baseline as evidence of a Godot deletion.

The exact ordinary Godot path therefore preserves correct per-application G-SYNC recovery in clone mode even though Godot saves DRS. The one first-launch blink prevents declaring the no-blank requirement fully solved. A controlled launch-placement repetition is needed: deliberately persist Godot on the cloned HDMI+eDP desktop and relaunch it there to separate clone-source placement from a one-time cold/profile transition.

## Controlled placement repetition

Captured: 2026-08-29 01:06 PDT

The user performed several Godot editor transitions from the primary DP desktop to the cloned HDMI+internal desktop and in the reverse direction. No launch, use, movement, or close reproduced a monitor blink. Godot remained smooth without the G-SYNC indicator in every case.

The later `VsyncStutterTest.exe` control ran smoothly and displayed the G-SYNC indicator.

The final capture remains healthy and structurally unchanged:

```text
Windows paths: three target paths, two source IDs
clone source 2: 2560x1440, internal eDP plus HDMI
separate source 0: 2560x1440, primary TB5/DisplayPort PA

target 8448 HDMI:       displayInVrrMode=1
target 8449 internal:   displayInVrrMode=1
target 8450 primary DP: displayInVrrMode=1
```

Repeated Godot starts produced expected DRS saves:

```text
nvdrsdb1.bin last write: 01:04:51
SHA-256: 4622776696821BFB108C6A90A4F0D044AC2229911203CAAEA9444A3C7E0F9CB4

nvdrsdb0.bin last write: 01:05:02
SHA-256: AA4CAB5DE724169EAF2C40779B0CDE6A15D962D20A7058050BC2DC262CE7751E

nvdrssel.bin: unchanged
SHA-256: 6E340B9CFFB37A989CA544E6BB780A2C78901D3FB33738768511A30617AFA01D
```

The exhaustive audit remains stable at one `Godot Engine` profile associated with the 4.4.1 and 4.6.3 executables, with no profile or application enumeration failures.

This rules out primary-versus-clone editor placement and movement direction as sufficient causes of the earlier isolated blink. The blink is best classified as an unreproduced cold/topology/profile-transition event immediately after constructing the clone state.

Within the current Windows session, the 1440p eDP+HDMI clone is a validated practical solution to the complete user-facing requirement:

- two usable external desktop spaces and no hidden third desktop;
- Godot and Unity run smoothly under Fixed Refresh without a G-SYNC indicator;
- later applications activate smooth G-SYNC automatically;
- no manual global G-SYNC toggle;
- no sticky VRR loss; and
- no recurring transition blank in repeated use.

The remaining durability test is reboot persistence: verify that Windows restores the same clone topology and that the first neutral and Godot transitions after sign-in remain healthy.
