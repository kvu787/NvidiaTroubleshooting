# Resume point: unresolved per-application Godot G-SYNC control

Last updated: 2026-08-28 PDT

## Status

The core user goal is unresolved.

Required end state:

- G-SYNC disabled in the Godot editor.
- G-SYNC enabled in AoE4 and other intended applications.
- No manual global G-SYNC toggle between applications.
- No long monitor blank.
- No sticky loss of G-SYNC after Godot exits.

TB5/DisplayPort plus native HDMI preserves correct Fixed Refresh/G-SYNC transitions only while the internal laptop display remains active. After reboot, Windows-disconnecting the internal panel restores poor behavior; reconnecting it restores smoothness. This defeats the user's ideal two-desktop setup because the hidden internal desktop can capture windows.

Unity now proves the sticky failure is not Godot-specific. In the captured two-external mixed-MediaSync topology, AoE4 worked before Unity. Launching Unity under its explicit Fixed Refresh profile caused a two-to-three-second blink; after Unity closed, both AoE4 and unprofiled `VsyncStutterTest.exe` lacked G-SYNC and were choppy. Target 8450 remained `VRR possible=1` but changed from `displayInVrrMode=1` to `0`.

The DRS databases were last written at 22:11, before the successful 22:36 AoE control and the Unity transition. Unity did not rewrite DRS. Activation of an existing Fixed Refresh profile is sufficient; Godot's DRS save/reload is not required.

The user then performed global G-SYNC off/apply/on/apply. At 22:51, target 8450 returned from `displayInVrrMode=0` to `1`; target 8452 remained non-VRR. This verifies recovery at the exact NVAPI state bit.

The user then ran `VsyncStutterTest.exe` in the recovered two-external state. It showed the G-SYNC indicator and ran smoothly. The 22:55 post-control capture remained correct, validating this unprofiled executable as the routine control.

At 22:58, the user left both PAs physically connected/powered but used Windows `Disconnect this display` for target 8452. Windows now has only internal target 8449 and external target 8450 active. NVAPI still sees target 8452 as connected and physically connected but inactive, so the active-head-count arm is correctly established. However, the VRR query for active target 8450 now returns generic `NVAPI_ERROR` twice; its Adaptive-Sync capability data remains present. A pre-Unity `VsyncStutterTest.exe` run must validate whether G-SYNC is healthy after the topology change.

That pre-Unity control succeeded: `VsyncStutterTest.exe` showed the indicator and ran smoothly. The 23:01 topology and DRS-hash capture is stable. The generic VRR-query error is therefore not evidence that G-SYNC is broken in this topology.

The Unity transition also succeeded cleanly in this topology: there was no monitor blink, Unity ran without G-SYNC as intended, and post-Unity `VsyncStutterTest.exe` still showed the indicator and ran smoothly. The 23:07 topology and DRS hashes remained unchanged. Two active external scanout heads are therefore required; the second PA's physical connection/power/enumeration is not sufficient.

At 23:11, both external PAs were made active again using different routes. MediaSync-on target 8450 remains on TB5/DisplayPort at 119.998 Hz; the second PA is now HDMI target 8448 at 59.951 Hz. The internal panel remains active, all three paths are on the RTX GPU, and DRS is unchanged. Despite MediaSync remaining off in the HDMI PA's OSD, NVAPI reports target 8448 as VRR-possible and in VRR display mode. Establish a healthy `VsyncStutterTest.exe` control on target 8450 before opening Unity.

That control succeeded: `VsyncStutterTest.exe` showed the G-SYNC indicator and ran smoothly on target 8450. At 23:18, topology, NVAPI VRR-mode bits, and DRS hashes remained at baseline. The decisive next action is `Unity Fixed Refresh -> close Unity -> VsyncStutterTest.exe` without changing anything else.

The Unity transition also succeeded: no monitor blink, Unity behaved acceptably without G-SYNC, and the immediate `VsyncStutterTest.exe` control retained the indicator and smooth motion. At 23:20, target 8450 remained `displayInVrrMode=1`, topology remained unchanged, and DRS was byte-for-byte identical. This proves two active external heads are not sufficient. The successful arm changed both route and mode: the secondary moved from 119.998-Hz TB5/DisplayPort to 59.951-Hz HDMI. Route versus refresh/link load remains unresolved, but the configuration is a viable workaround candidate.

The current `Godot Engine` DRS profile matches `godot_v4.6.3-stable_win64.exe` and explicitly sets VRR requested state disabled plus G-SYNC Fixed Refresh. The next action is the final exact Godot validation in the working TB5/DisplayPort-plus-HDMI topology.

That Godot validation succeeded on the core switching behavior twice. Both immediate `VsyncStutterTest.exe` controls showed smooth G-SYNC. The first editor opened on the internal panel and caused a two-second blank; the second opened on the primary PA and caused none. At 23:28, primary target 8450 remains `displayInVrrMode=1`; the HDMI target is now outside VRR mode; topology is unchanged. Godot rewrote DRS at 23:24:38 and 23:25:09 but the effective profile is unchanged. The next test should force/persist the editor onto the internal display, then reopen it, to distinguish launch-monitor placement from a one-time cold transition.

The forced-placement repetition produced no blank on either launch. Godot remained smooth without G-SYNC, and the later control remained smooth with G-SYNC. DRS saved again at 23:31:52 and 23:32:20; effective settings remained unchanged. At 23:33, primary target 8450 remains in VRR mode and HDMI target 8448 remains outside it. Window placement and DRS save are not sufficient causes. The earlier blank is a non-reproduced cold/topology-transition event.

After reboot, both external PAs are now 119.998 Hz and HDMI MediaSync is on. With internal eDP active, the user reports smooth behavior and the 00:11 probe shows all three targets at `displayInVrrMode=1`. HDMI at 120 Hz therefore works; the old 60-Hz mode was not required. The user reports poor behavior if internal eDP is Windows-disconnected and smooth behavior when it is reconnected, regardless of HDMI MediaSync on/off. Capture the disconnected state before any application, then run a neutral pre-editor control.

## New leading result: external-monitor-count A/B

Machine: ASUS ROG Strix G18 `G815LR-IS97`.

```text
One PA278QGV on one Thunderbolt 5/USB-C port: smooth; issue absent
Two PA278QGVs, one on each Thunderbolt 5/USB-C port: issue reproduces
```

The current one-external state still contains a matching `Godot Engine` profile with `G-SYNC: Fixed Refresh` and `VRR requested state: disabled`. Therefore the success is not caused by removing the Godot override.

Windows/NVAPI topology:

- the internal panel and current PA are both active and directly owned by the RTX 5070 Ti;
- current PA: target 8452, external DisplayPort connector instance 1, 2560x1440 at 119.998 Hz;
- disconnected second PA: target 8450, external DisplayPort connector instance 0;
- both PA targets are distinct NVIDIA DP connectors, not MST and not different GPUs;
- the active PA is on a four-lane HBR2/8-bpc link; and
- no downstream USB4 device router is present, so the current monitor path is DisplayPort output through the TB5-capable USB-C port rather than a Thunderbolt display tunnel.

Current NVIDIA driver: 596.49 (`r596_25`), installed at 16:30:31 PDT. The original Godot investigation used 616.56. Unity has now reproduced the same two-external Fixed Refresh transition failure on 596.49, so the underlying defect crosses both tested branches.

Follow-up results now point more narrowly to two active external display heads. Two external PAs are poor with the internal panel either active or disabled and with one PA's OSD MediaSync off. One external PA plus the internal panel is smooth. This exact topology-dependent result occurs in both Unity editor and Godot editor. Thus the internal panel, total active-display count, dual-VRR eligibility, and a Godot-specific implementation are not the trigger. After the user confirmed the OSD state, NVAPI independently reported that DisplayPort target as non-VRR; it did not directly read MediaSync from the monitor.

Treat editor smoothness and the Fixed Refresh transition as related driver/topology behavior. Unity establishes both that poor editor behavior exists without Godot and that an already-stored Fixed Refresh profile can cause the same monitor blank and sticky failure to restore later G-SYNC. Godot's DRS writer is an avoidable integration behavior, but it is not necessary for the driver failure.

The 22:30 pre-Godot baseline closes the NVIDIA-capability caveat for the DisplayPort topology: target 8450 is VRR-capable, while user-confirmed MediaSync-off target 8452 reports `VRR possible=0`, `displayInVrrMode=0`, and a zero Adaptive-Sync interval. Both remain active external heads. Dual external NVIDIA-reported VRR capability is therefore ruled out as a necessary condition.

AoE4 works normally in this exact state before either editor opens: the indicator appears and G-SYNC behaves as expected. A 22:36 post-AoE capture is unchanged. Therefore dual external heads do not globally break G-SYNC. The current separation is editor/windowed presentation behavior versus persistent state poisoning after an editor exits.

The current smooth capture uses external target 8450/connector 0; the earlier smooth capture used target 8452/connector 1. Each external port works individually, making a single bad port unlikely.

Superseded test plan, retained for history:

1. connect and enable both PAs;
2. turn Adaptive-Sync off in the secondary PA's OSD only;
3. recover global G-SYNC once and confirm via `nvapi-vrr-query.cpp` that only the intended PA is VRR-possible;
4. repeat the ordinary Fixed Refresh Godot → AoE4 sequence;
5. if it still fails, leave both connected but disable the secondary in Windows and repeat; and
6. separately verify that each physical TB5/USB-C port works smoothly with exactly one PA.

If step 4 succeeds, leaving Adaptive-Sync disabled on the secondary PA is the first plausible stable workaround that keeps both monitors connected and avoids per-session global G-SYNC toggling.

The user completed the MediaSync test, pre-editor NVAPI capture, working AoE4 control, Unity-to-control transition, driver-level recovery, and clean `VsyncStutterTest.exe` baseline. Current next test, in order:

1. keep the MediaSync-on PA on one TB5/USB-C DisplayPort output;
2. move the second PA from the other TB5/USB-C output to the laptop HDMI output and make it active in Windows;
3. topology, a healthy `VsyncStutterTest.exe` baseline, and the clean Unity transition are complete;
4. the original Godot workflow and launch-placement isolation are complete; and
5. the internal-panel dependency supersedes the earlier stopping point: capture `internal disconnected + TB5/DP 120 Hz + HDMI 120 Hz` before opening an editor, then run `VsyncStutterTest.exe` as the neutral control.

The refresh/load question is resolved for the HDMI route: both external PAs work at 119.998 Hz when internal eDP is active. The immediate investigation is now the internal-panel active-path dependency and a possible way to keep eDP active without exposing a third usable desktop. Dual-VRR eligibility on the earlier DisplayPort topology, driver-branch specificity, and the need or sufficiency of Godot's DRS write remain resolved. Direct OSD-state detection has not been implemented.

## Per-display software possibility

NVCP cannot independently enable only one of the two identical PA278QGVs. NVIDIA documents its display checkbox as applying to every connected display of the selected model.

NVIDIA's public NVAPI does expose a lower-level per-display setter: `NvAPI_DISP_SetAdaptiveSyncData(displayId, ...)`, with a `bDisableAdaptiveSync` input documented as applying to the display. Because the two PAs have distinct display IDs, a guarded custom utility may be able to disable Adaptive-Sync on only the secondary PA. This path has not been tested and is not documented as persistent across reboot, hotplug, driver restart, or modeset; it may also blank a display while applying. The monitor OSD remains the safest persistent control.

If this is pursued, do not extend the existing read-only probe in a way that makes accidental writes easy. Build a separate tool that defaults to status-only, requires an exact display target plus an explicit enable/disable verb, captures the prior state, verifies the post-state, and supports rollback. No NVAPI setter has been called so far.

## What is established

### Unsafe/failed path

Ordinary Godot project-manager launch while a matching Godot profile is Fixed Refresh:

- suppresses G-SYNC in the editor;
- runs the native-OpenGL project manager's `_nvapi_setup_profile()`;
- unconditionally saves/reloads NVIDIA DRS;
- coincides with a roughly three-second blank on both monitors; and
- leaves AoE4 unable to activate G-SYNC until global G-SYNC is cycled off/on.

This survives removal of duplicate/exact-path profiles. A single basename `Godot Engine` Fixed Refresh profile is sufficient for the reproduced combination.

### Safe but incomplete path

Direct D3D12 editor launch with Vulkan and OpenGL fallback disabled, from a state with no Godot NVIDIA profile:

- creates no profile or application association;
- changes no DRS byte;
- causes no monitor blank;
- leaves G-SYNC active in the editor;
- produces choppy pointer movement; and
- preserves working AoE4 G-SYNC afterward.

Command:

```text
"C:\Users\k\Program\Godot_v4.6.3-stable_win64.exe\Godot_v4.6.3-stable_win64.exe" --editor --path "C:\Users\k\Repository\Godot\VsyncStutterTest\Godot" --rendering-driver d3d12
```

Required project settings already present at the last test:

```ini
rendering_device/driver.windows="d3d12"
rendering_device/fallback_to_opengl3=false
rendering_device/fallback_to_vulkan=false
```

This is a reliable bypass of Godot's NVIDIA profile writer, not a solution for disabling editor G-SYNC.

### Recovery that is not an acceptable workflow

Global G-SYNC off/on in NVIDIA App was verified to restore AoE4 G-SYNC after the sticky failure. It is cumbersome and causes a long monitor blank, so it does not satisfy the per-app requirement.

## State at the last clean test

Before and after the direct D3D12 editor session:

```text
DRS profile count: 7957
Godot DRS profiles: none
Godot DRS application associations: none
NVIDIA App Godot catalog entries: none
```

AoE4 activated G-SYNC normally after that editor session.

The Godot project worktree contains a semantic-neutral editor rewrite of `project.godot`: CRLF became LF and the two fallback keys changed textual order. Preserve or review this user/project-owned change when resuming.

## Resolved historical experiment

The earlier proposed test was:

1. capture a fresh clean DRS/catalog/hash baseline;
2. create exactly one Fixed Refresh profile associated with the Godot executable, without using the stale NVIDIA App manual row;
3. launch only through the verified direct D3D12/no-fallback command;
4. check for a monitor blank, editor G-SYNC indicator, DRS writes, and process exit;
5. close Godot and immediately test the AoE4 G-SYNC indicator; and
6. compare every DRS hash and association with the baseline.

Do not run this experiment. Unity has already supplied the cleaner result: its existing Fixed Refresh profile reproduced the blank and sticky failure without a DRS write. Profile activation alone is not safe in the two-external topology.

## Separate unresolved observation

During the direct D3D12 test, closing the editor window did not return the command prompt within ten seconds. The Godot process was gone later, and no crash, WER, Application Hang, TDR, or display-driver event was recorded. Investigate this separately with verbose logging and live process capture if needed.

## Primary references in this folder

- `findings.md`: full diagnosis and confidence assessment.
- `evidence.md`: timestamped evidence record.
- `pre-d3d12-clean-nvidia-baseline.txt`: clean pre-launch state.
- `direct-d3d12-postlaunch-state.txt`: byte-for-byte post-launch comparison and AoE4 validation.
- `nvidia-app-row-selection-post-state.txt`: NVIDIA App orphan-profile creation behavior.
- `drs-substring-audit.cpp`: read-only exhaustive DRS audit source.
- `display-topology-query.cpp`: read-only Windows display-path/connector query.
- `nvapi-vrr-query.cpp`: read-only NVIDIA per-display VRR and DisplayPort query.
- `topology-ab-evidence.md`: new physical-topology evidence and revised hypothesis.
- `two-external-mixed-mediasync-baseline.md`: pre-editor capability capture and working AoE4 control.
- `unity-fixed-refresh-transition.md`: decisive Unity Fixed Refresh transition, post-state, and DRS timestamp evidence.
- `windows-disconnected-second-pa-baseline.md`: one-active-external/two-physically-connected topology and VRR-query anomaly.
