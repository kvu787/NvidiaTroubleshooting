# Godot 4.6.3 / Age of Empires IV G-SYNC investigation

Investigation date: 2026-08-28 PDT

## Conclusion

Opening the Godot project is not persistently turning off the NVIDIA global G-SYNC setting, and it is not changing the later control application's profile.

The failure is a live NVIDIA driver-state problem caused by activation/deactivation of a Fixed Refresh application profile in the tested dual-TB5/USB-C DisplayPort, dual-119.998-Hz topology. Both Godot and Unity reproduce a two-to-three-second monitor blank in that topology, after which the VRR-capable PA remains stuck outside VRR mode even though persistent global G-SYNC remains enabled.

Unity is the decisive control. Its current `Unity 3D` profile explicitly requests VRR disabled and Fixed Refresh. Unity reproduced the blank and left both AoE4 and the unprofiled `VsyncStutterTest.exe` without G-SYNC. The NVIDIA DRS databases were last written about 25 minutes before the Unity test, proving no application-side DRS save/reload was required. Merely activating the already-stored Fixed Refresh profile is sufficient.

The subsequent global G-SYNC off/on recovery restored target 8450 from `displayInVrrMode=0` to `1` while leaving its VRR capability and the display topology unchanged. The UI recovery therefore directly reprograms the exact live state bit that becomes stuck after Fixed Refresh profile activation.

After recovery, unprofiled `VsyncStutterTest.exe` displayed the G-SYNC indicator and ran smoothly; the post-control API state remained correct. It is now a validated replacement for AoE4 in the remaining tests.

The active-head-count and route/mode isolations are decisive. With both PA cables connected and both monitors powered, but MediaSync-off target 8452 disabled in Windows, Unity Fixed Refresh caused no blink or sticky failure. With both external PAs active but the second moved from 119.998-Hz TB5/DisplayPort to 59.951-Hz native HDMI, Unity likewise caused no blink or sticky failure. Therefore two active external heads alone are not sufficient. Route alone is not yet isolated because the HDMI move also reduced the secondary refresh/link load and changed its NVIDIA target/mode state.

The exact Godot workflow is now validated twice in the working route/mode family. Godot runs smoothly without G-SYNC, and the later neutral application restores smooth G-SYNC without a global toggle. The first project-editor launch opened on the internal panel and produced a two-second blank; the second opened on the primary PA and produced no blank. Neither caused sticky failure. This meets the central per-application switching requirement but leaves the no-blank requirement conditional until launch-display placement versus cold-start state is isolated.

That placement isolation is now complete. Two additional Godot launches, including one deliberately persisted to the internal panel, produced no blank. Godot saved DRS on both and remained smooth without G-SYNC; the later neutral control retained G-SYNC. Window placement and `NvAPI_DRS_SaveSettings()` are therefore each ruled out as sufficient for the isolated first blank. In stable use, the working route/mode family meets the full practical goal. A first transition after reboot, hotplug, or topology reconstruction may still blank and has not been tested.

Post-reboot usage reveals that the workaround depends on the internal panel remaining active. TB5/DisplayPort plus HDMI is smooth with the internal eDP panel active, but poor when Windows disconnects the internal panel; reconnecting it restores smoothness. Both external PAs now run at 119.998 Hz and the HDMI PA's MediaSync on/off state did not change the A/B. Thus the prior 60-Hz secondary was not necessary. The leading condition is a compound active-path topology: HDMI routing avoids the dual-TB5 failure only while the internal eDP scanout path remains active.

The clean internal-off pre-application capture confirms only the two external 120-Hz paths are active and DRS is unchanged. HDMI remains in NVIDIA VRR display mode, while the primary DP target's public VRR query returns a generic error. That error is not diagnostic by itself because it occurred in an earlier working Windows-disconnected topology. The neutral control, not the query error, must determine whether disconnect alone is sufficient.

The neutral control is healthy: `VsyncStutterTest.exe` shows the indicator and runs smoothly before any editor in the internal-off DP+HDMI topology. The 00:16 state and DRS hashes are unchanged. Therefore internal eDP disconnection alone is not sufficient; the user-observed poor state requires a later presentation/profile transition or another intervening event.

Unity supplies that transition but refines the symptom. With internal eDP off, Unity Fixed Refresh causes severe repeated monitor blanks on open, during use, and on close. Nevertheless, the neutral G-SYNC control restores correctly afterward twice. DRS and final target state remain unchanged, and Windows logs contain no relevant display-driver reset. The internal panel is therefore stabilizing Fixed Refresh modeset transitions; it is not required for eventual G-SYNC recovery in the mixed-route topology.

Lowering only the HDMI secondary from 119.998 Hz to 59.951 Hz while keeping internal eDP off immediately changes the read-only driver state: the primary DP VRR-info query returns successfully with `displayInVrrMode=1` instead of generic error. DRS and physical routes are unchanged. This makes external scanout mode/clock/resource allocation the leading mechanism, pending the functional Unity transition.

The 120/60 neutral control is also functionally healthy and leaves both external targets queryable in VRR display mode. The Unity transition can now be compared without a preexisting failure.

That Unity transition produces the damaging sticky failure. After one initial blink, `VsyncStutterTest.exe` loses its indicator and becomes choppy; a second Unity/control sequence remains failed. Primary target 8450 changes from `displayInVrrMode=1` to `0`, HDMI remains at `1`, and DRS is unchanged. Lowering HDMI therefore changes the failure from repeated transient blanking at 120/120 to persistent VRR loss at 120/60. It is not a workaround and disproves a simple high-load threshold model.

Reconnecting/extending internal eDP recovers target 8450 from mode 0 to 1 without a global G-SYNC toggle or DRS change. Windows simultaneously returns HDMI from 60 Hz to 120 Hz, so this is a topology/mode-reconstruction recovery rather than a pure eDP-only proof. It nevertheless provides a faster recovery path and prepares the clone-mode workaround test.

The subsequent neutral control confirms that this is a complete functional recovery, not merely an API-bit change. `VsyncStutterTest.exe` runs smoothly with the indicator, target 8450 remains in VRR mode 1, all three active targets report VRR mode 1, and DRS remains unchanged. The next workaround experiment is to duplicate the hidden internal panel with the HDMI PA while leaving the primary TB5/DisplayPort PA extended separately. That should retain an active eDP path without exposing a third Windows desktop, although it does not literally disconnect or power down the internal panel.

The clone baseline realizes that topology exactly. Internal target 8449 and HDMI target 8448 share one Windows source ID and desktop position, while primary DP target 8450 remains a separate source. NVIDIA still sees all three physical targets active and in VRR mode 1, and DRS is unchanged. Thus clone mode removes the hidden third desktop without removing the eDP scanout path that appears to stabilize Fixed Refresh transitions. Functional G-SYNC and Unity transition tests remain pending. Windows uses a 2560x1600 clone source while sending 2560x1440 to HDMI, so visual scaling usability must also be evaluated.

The initial 2560x1600 clone source letterboxed the HDMI PA. Changing only the clone source to 2560x1440 gives HDMI a matching native 1440p source and target signal while leaving scaling to the hidden 1600p internal panel. All paths, clone membership, VRR-mode bits, and DRS state remain otherwise unchanged. This 1440p clone is the usable pre-application baseline.

The neutral control in that 1440p clone state succeeds with smooth G-SYNC and the indicator on the separate primary DP PA. The post-control topology, all three VRR-mode bits, and DRS remain unchanged. Clone mode is therefore a viable neutral-use topology; whether active internal eDP still stabilizes a Fixed Refresh editor transition when it shares a source with HDMI remains the decisive test.

This explains the apparently contradictory observations:

- No indicator in the Godot editor is expected from whichever matching Godot profile is configured as Fixed Refresh.
- No indicator in a later G-SYNC-allowed application is the defect: the driver does not successfully transition back from the preceding Fixed Refresh application state.
- The settings UI and DRS database still show G-SYNC enabled because the persistent configuration was not turned off.

A subsequent recovery test confirmed this interpretation: disabling global G-SYNC, applying, enabling it again, and applying restored the G-SYNC indicator in AoE4 without any Godot or AoE4 profile edit.

A later one-profile isolation test made the trigger narrower. The exact-path NVIDIA App profile was deleted, and the remaining basename `Godot Engine` profile was set to Fixed Refresh in NVIDIA Control Panel. Godot still produced the same three-second blank, and AoE4 still failed to activate G-SYNC afterward. Therefore duplicate profiles, exact-path matching, and the NVIDIA App-created profile are not required. The subsequent Unity control narrowed this further: an existing Fixed Refresh profile can trigger the failure without any DRS write.

The best classification is an NVIDIA multi-display VRR/profile-transition bug specific to the tested dual external DisplayPort-over-USB-C, high-refresh state. Godot's unconditional NVAPI profile writer exposed the problem but is not necessary for it. The original reproduction used driver 616.56; Unity reproduced the failure on driver 596.49. The failure to restore G-SYNC for a later application is therefore a driver/display-path failure across both tested driver branches.

The direct D3D12 bypass has now been runtime-verified from a completely clean Godot/NVIDIA baseline. With both rendering fallbacks disabled, the editor opened without a monitor blank, showed the G-SYNC indicator, and exhibited the user's choppy pointer movement. Afterward, every DRS file remained byte-for-byte identical to the pre-launch baseline, the profile count remained 7957, the exhaustive Godot audit remained clean, and NVIDIA App's private catalog remained free of Godot. The user then launched AoE4 and observed its G-SYNC indicator normally. This proves the direct D3D12 path avoids both Godot's profile writer and Fixed Refresh profile activation; the Unity control shows the latter is the necessary distinction for the blank/sticky failure. The choppy G-SYNC editor behavior is the tradeoff that Godot's profile workaround was designed to suppress.

### Unresolved primary user goal

The investigation has not produced a usable configuration that satisfies the original requirement in full:

1. G-SYNC disabled for the Godot editor;
2. G-SYNC enabled for AoE4 and other intended applications;
3. no manual global G-SYNC off/on cycle between applications;
4. no long monitor blank; and
5. no sticky live-VRR failure after Godot exits.

The tested approaches split into incompatible partial outcomes:

| Approach | Godot editor | AoE4 afterward | Side effect | Status |
|---|---|---|---|---|
| Ordinary project-manager launch with matching Fixed Refresh profile | G-SYNC disabled | G-SYNC fails to activate | Godot saves/reloads DRS; monitors blank; live VRR becomes stuck | Not acceptable |
| Unity launch with matching Fixed Refresh profile | G-SYNC disabled | AoE4 and `VsyncStutterTest.exe` fail to activate | No DRS write; monitors blank; live VRR becomes stuck | Proves profile activation alone is unsafe |
| Clean direct D3D12/no-fallback launch with no Godot profile | G-SYNC active; pointer is choppy | G-SYNC activates normally | No DRS mutation or monitor blank | Safe partial workaround, but does not meet editor requirement |
| Explicit global G-SYNC off/on in NVIDIA App or NVIDIA Control Panel | Can force desired global state manually | Works after re-enable | Cumbersome and causes a long monitor blank | Recovery/manual toggle, not a per-app solution |

Therefore the direct D3D12 procedure is a verified workaround for profile mutation and the sticky NVIDIA state, but it is not a solution to the core per-application requirement. Calling it a complete workaround would overstate the result.

The previously proposed combination—an existing Fixed Refresh profile with a launch path that does not save DRS—is no longer a plausible safe workaround. Unity has now demonstrated that activation of an existing Fixed Refresh profile alone reproduces the blank and sticky failure.

## Topology A/B update

The user found a clean physical-topology discriminator on an ASUS ROG Strix G18 `G815LR-IS97`:

| Active external PA278QGV topology | Result |
|---|---|
| One PA278QGV connected to one Thunderbolt 5/USB-C port | Smooth; the G-SYNC issue does not reproduce |
| Two PA278QGVs, one connected to each Thunderbolt 5/USB-C port | The documented G-SYNC issue reproduces |

The current one-external-monitor arm is particularly strong because it still has the same relevant per-application condition. A live DRS query reports a matching `Godot Engine` profile associated with `godot_v4.6.3-stable_win64.exe` and containing:

```text
VRR requested state: disabled
G-SYNC application override: fixed refresh
G-SYNC mode: fullscreen only
OpenGL threaded optimization: disabled
```

Therefore the smooth arm is not explained by the Fixed Refresh profile being absent. The second external display path is a necessary condition in the user's A/B.

A follow-up matrix narrows this further. Two external PAs behave poorly both with the internal panel active and with it disabled, even when OSD MediaSync is off on one external PA. One MediaSync-enabled external PA plus the internal panel behaves smoothly. The same result occurs in both the Godot editor and Unity editor. Therefore the leading condition is **two active external display heads**, not the internal panel, not merely two active displays in total, not two NVIDIA-reported VRR-capable external targets, and not a Godot-specific editor implementation. After the user confirmed the OSD setting, NVAPI independently reported that DisplayPort target as non-VRR; it did not directly read MediaSync from the monitor.

Unity's matching behavior resolves what was previously entangled. Editor motion/smoothness is broadly topology-dependent, and launching either editor under a Fixed Refresh profile can produce the monitor blank and sticky loss of later G-SYNC. Godot is not the root cause; its profile writer is one way the Fixed Refresh configuration is established, not a required trigger.

The bad-topology baseline subsequently verified the MediaSync distinction in NVAPI before Godot opened. External target 8450 reports VRR possible and a 20583-us maximum frame interval; external target 8452 reports VRR impossible, not in VRR mode, and a zero maximum frame interval. Both external heads are nevertheless active at 2560x1440/119.998 Hz. This proves the poor editor behavior does not require two VRR-enabled external monitors. It requires the second active external head under the tested configurations.

AoE4 then provided a clean pre-editor control in that exact state: without opening Unity or Godot and without toggling G-SYNC, AoE4 displayed the G-SYNC indicator and behaved normally. The post-AoE read-only state was unchanged. Thus two active external heads do not inherently disable or degrade G-SYNC for all applications. The failure is presentation-path/application-class dependent.

The earlier 14:28 Unity NVIDIA-settings audit correctly showed no user G-SYNC/refresh override at that timestamp. The current profile was configured afterward and now explicitly contains `VRR requested state: disabled`, `G-SYNC: fixed refresh`, and `G-SYNC mode: disabled`.

The latest good-state capture contains internal target 8449 and external target 8450/connector instance 0. The previous good-state capture contained the internal target and external target 8452/connector instance 1. Both external ports therefore work individually; a defective single port is unlikely.

### What Windows and NVAPI show in the one-external arm

The current active paths are:

| Display | Windows target | Connector | Mode | Owner |
|---|---:|---:|---|---|
| Internal `NE180QDM-NZC` | 8449 | embedded DP instance 0 | 2560x1600 at 240 Hz | RTX 5070 Ti Laptop GPU |
| Connected `PA278QGV` | 8452 | external DP instance 1 | 2560x1440 at 119.998 Hz | RTX 5070 Ti Laptop GPU |

The other PA278QGV is the recently disconnected Windows target 8450, external DP connector instance 0. Both monitor device nodes have the RTX 5070 Ti as their parent. Microsoft documents `connectorInstance` as a connector identity unique within an adapter, so 8450 and 8452 are two distinct external DP targets on the same NVIDIA adapter, not one MST branch and not displays owned by different GPUs. The NVAPI result for the active PA also reports `dynamicMst=0` and `mstRoot=0`.

The active PA link is DisplayPort 1.4/HBR2, four lanes, 8 bits per component. The internal panel and the active PA both report VRR possible and their display modes are VRR-capable. The PA's adaptive-sync minimum corresponds to about 48.6 Hz, matching ASUS's published 48–120 Hz range.

No downstream USB4 device router is present in the current Plug-and-Play tree. The laptop has one Intel USB4 host/root router, while the monitor enumerates as an NVIDIA DisplayPort target. Thus “connected to a Thunderbolt 5 port” describes the physical USB-C receptacle; the current monitor path behaves as DisplayPort output/Alt Mode rather than as a monitor behind a Thunderbolt device router. Thunderbolt tunnel bandwidth is not the leading explanation.

### Revised causal model

The original trigger sequence needs one new condition:

1. both external PA278QGV DisplayPort targets are active;
2. an application matched to a Fixed Refresh profile starts;
3. NVIDIA reprograms multiple external display/VRR targets and both monitor device nodes may transiently disappear/re-enumerate; and
4. the driver leaves the VRR-capable external target outside VRR mode after the application exits.

With only one external PA target active, the same Fixed Refresh editor profiles do not produce the user-visible failure. Fixed Refresh profile activation is therefore necessary in the tested transition, but it is not independently sufficient without the two-external-head topology.

Windows' device-management log contains repeated simultaneous `surprise removed ... missing on the bus` events for targets 8450 and 8452 during the dual-monitor testing period. At 19:05:51 only target 8450 was removed; target 8452 remained and is the current working one-external state. These events corroborate real display-target churn. They do not, by themselves, distinguish physical unplugging from a driver modeset/hotplug cycle for every earlier timestamp.

The Intel external-display event channel also logged repeated errors during the connection/reconfiguration period. Because Windows attributes scanout to the NVIDIA adapter while the physical Type-C ports are provided through the Intel platform display/USB4 complex, a cross-driver port-mux/hotplug interaction remains plausible. The Intel event parameters are undocumented, so the error records are supporting evidence, not a decoded root cause.

### Superseded test plan: secondary MediaSync off

Keep both PA278QGVs physically connected and active, but turn Adaptive-Sync off in the OSD of the secondary PA only. Recover global G-SYNC once, verify that NVAPI reports VRR possible on only the intended PA, and repeat the ordinary Godot → AoE4 sequence.

- If this is smooth, the necessary condition is two external Adaptive-Sync targets. This also yields a practical two-monitor workaround: leave Adaptive-Sync disabled on the secondary display.
- If it still fails, disable the secondary only in Windows while leaving it physically connected. A smooth result would identify the number of active external scanout paths rather than VRR capability itself.
- Test each Thunderbolt 5/USB-C port with exactly one PA. If both single-port cases are smooth and only the dual-port case fails, neither port is individually defective.

This order is more discriminating than lowering resolution or refresh rate. The current PA already uses its own four-lane HBR2 link, both laptop ports are explicitly advertised by ASUS as supporting DisplayPort and G-SYNC, and the two monitors are separate NVIDIA targets. A bandwidth test is still useful later: run both PAs at 60 Hz. It is lower priority.

The user completed the central part of that plan: two external PAs still behaved poorly with MediaSync off on one PA, both with the internal panel active and with it disabled. The old plan is retained above as investigation history, not as the current recommendation.

### Current highest-value next test

The Unity-to-control transition, driver-level recovery, replacement-control validation, and active-head-count isolation are complete. The highest-value remaining test is two active external PAs with different physical routes: MediaSync-on PA via TB5/DisplayPort and the second PA via HDMI. This distinguishes the dual-TB5/DP route from any-two-active-external behavior.

That routing baseline is now established. The TB5/DP PA is target 8450 at 119.998 Hz and the HDMI PA is new target 8448 at 59.951 Hz; both plus the internal panel are active directly on the RTX GPU. No DRS data changed. NVAPI unexpectedly advertises the OSD-MediaSync-off HDMI target as VRR-possible and in VRR display mode, unlike the same OSD-off PA on DisplayPort. The physical-route A/B remains valid, but the current arm is not a perfect mixed-VRR-capability match.

The pre-Unity functional control in this topology is healthy: `VsyncStutterTest.exe` shows the G-SYNC indicator and runs smoothly on target 8450. A 23:18 capture confirms all display paths, VRR-mode bits, and DRS hashes remain at baseline. The Unity-to-control transition can now distinguish the dual-TB5/DP route from any two-active-external topology without a preexisting G-SYNC failure.

That transition remained healthy. Unity caused no blink and ran without G-SYNC; the immediate neutral control retained G-SYNC and smooth motion. The 23:20 post-state is unchanged and target 8450 remains in VRR display mode. Therefore two active external heads are not sufficient. One PA at 119.998 Hz on TB5/DisplayPort plus the second at 59.951 Hz on native HDMI is the first two-external-monitor workaround candidate that preserves the desired profile transition in Unity. Exact Godot validation remains. Because both route and refresh changed, route alone is not yet proven causal.

Godot validation substantially upgrades that candidate. Two successive Godot-to-control transitions preserved correct per-application behavior and later G-SYNC. Godot rewrote DRS on both launches but retained the same effective profile; the second launch did not blank. Thus Godot's unconditional save is neither sufficient for the blank nor for sticky G-SYNC loss. The remaining two-second first blank occurred only when the project editor opened on the internal laptop panel, while the no-blank second launch opened on the primary PA. A controlled window-placement repetition is required before calling the no-blank requirement fully solved.

The controlled repetition then produced no blank with the editor persisted to the internal panel. This rules out window placement as sufficient and validates repeated no-blank operation in the stable configuration. The earlier single blank remains best treated as a transient cold transition after the route/topology change, subject to later reboot/hotplug testing.

- TB5+HDMI with the HDMI PA at 59.951 Hz is smooth and isolates a route/mode family; and
- HDMI at 120 Hz, if available, or both TB5/DisplayPort PAs at 60 Hz is the later test that can separate route from refresh/link load.

NVIDIA's public support article for mixed-monitor VRR says multiple monitors may be connected but no more than one should have G-SYNC enabled. NVIDIA's setup help also describes its display enablement as applying to every connected display of the selected model. Both PAs have the same model/EDID, and the earlier NVIDIA App capture reported G-SYNC enabled on both. Those documents originally made dual enabled PA278QGVs the leading hypothesis. The MediaSync-off test now points more broadly to two active external heads. NVAPI measures NVIDIA's per-target state, not the monitor's OSD setting; the older support article is not proof of the exact 2026 driver defect.

### Per-display software control

NVIDIA Control Panel cannot express the desired split for the two identical PA278QGVs. Its own setup help says the checkbox applies G-SYNC settings to **all connected displays of a particular model**, so selecting either PA278QGV addresses the model rather than one serial-number/connector instance. The NVIDIA App presents the same practical limitation.

There is nevertheless a lower-level, untested software path. NVIDIA's public NVAPI exposes `NvAPI_DISP_SetAdaptiveSyncData(displayId, ...)`; its input structure contains `bDisableAdaptiveSync`, documented as indicating whether Adaptive Sync is disabled **on the display**. The two PAs have distinct Windows targets and NVAPI display IDs, so a purpose-built tool should be able to request Adaptive-Sync disabled for one PA while leaving the other enabled.

Important limits:

- This is a per-display Adaptive-Sync runtime API, not a documented persistent replacement for the NVIDIA Control Panel G-SYNC configuration.
- Persistence across a reboot, driver restart, hotplug, and display modeset is unknown and must be measured.
- Calling the setter may itself cause a modeset or monitor blank.
- No setter has been called during this investigation. `nvapi-vrr-query.cpp` remains strictly read-only.

The monitor OSD remains the safest persistent per-monitor control. If a software experiment is requested later, first build a guarded tool whose default operation is read-only, identifies a target by connector/Windows target/NVAPI display ID, records the original value, requires an explicit enable/disable command, verifies the result, and provides rollback.

Primary references:

- [ASUS G815LR-IS97 specifications: two Thunderbolt 5 ports with DisplayPort and G-SYNC support](https://rog.asus.com/us/laptops/rog-strix/rog-strix-g18-2025/spec/?config=90NR0LC1-M00460)
- [ASUS PA278QGV: DisplayPort 1.4, Adaptive-Sync, and 48–120 Hz VRR](https://www.asus.com/us/displays-desktops/monitors/proart/proart-display-pa278qv-gen2-pa278qgv/)
- [NVIDIA: mixed-monitor VRR supports no more than one G-SYNC-enabled display](https://nvidia.custhelp.com/app/answers/detail/a_id/4766/~/does-variable-refresh-rate-work-across-mixed-monitor-configurations%3F)
- [NVIDIA G-SYNC setup help](https://www.nvidia.com/content/Control-Panel-Help/vLatest/en-us/mergedProjects/nvdsp/To_use_variable_refresh_rates.htm)
- [NVIDIA NVAPI reference: per-display Adaptive-Sync get/set data](https://docs.nvidia.com/nvapi/nvapi_8h.html)
- [Microsoft `DISPLAYCONFIG_TARGET_DEVICE_NAME` connector identity](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-displayconfig_target_device_name)
- [NVIDIA's official open-source NVAPI SDK](https://github.com/NVIDIA/nvapi)

## Decisive evidence

### Persistent global and display state remained enabled

A read-only NVIDIA App launch at 14:59, after the reported failure, caused NVIDIA's own display APIs to report:

```text
GetGlobalGsyncState: 1
GetGsyncIndicator: 1
Display 1 gsyncState.enabled: true
Display 2 gsyncState.enabled: true
```

The same query reported `vrrMode: 1` (fullscreen only). Dynamic Display Switching reported success with `MuxState: 1`; there was no logged display-mode switch at the reproduction time.

Therefore the missing in-game indicator is not explained by a disabled global toggle, a disabled indicator toggle, or G-SYNC being disabled on the displays.

### AoE4's NVIDIA profile remained G-SYNC-capable

`RelicCardinal.exe` still resolves to NVIDIA's predefined `Age of Empires IV` profile. Its effective relevant settings after the failure are:

| Setting | Value | Source |
|---|---:|---|
| G-SYNC application override (`0x10A879CF`) | `0`, allow | Global profile |
| VRR requested state (`0x1094F1F7`) | `1`, fullscreen only | AoE4 profile |
| G-SYNC mode (`0x1194F158`) | `1`, fullscreen only | AoE4 profile |

AoE4 has no Fixed Refresh override. The driver's fully qualified and basename lookups both select `Age of Empires IV`.

### The failure survives removal of the exact-path profile

The user deleted the exact-path `Godot_v4.6.3-stable_win64.exe` profile in NVIDIA Profile Inspector, then set the remaining `Godot Engine` profile to `Monitor Technology: Fixed Refresh` in NVIDIA Control Panel.

The post-failure DRS query proves that the requested profile deletion took effect:

```text
Total profile count: 7958 (previously 7959)
FindProfileByName("Godot_v4.6.3-stable_win64.exe"): not found
Full-path lookup: Godot Engine
Basename lookup: Godot Engine
Application association: godot_v4.6.3-stable_win64.exe
```

Only one Godot profile now exists. Its relevant state after the reproduction is:

```text
Profile: Godot Engine
VRR requested state: disabled
G-SYNC application override: fixed refresh
G-SYNC mode: fullscreen only
OpenGL threaded optimization: disabled
```

This is a single, basename-associated Fixed Refresh profile. The exact-path profile, split-profile selection, and NVIDIA App-specific profile creation are no longer possible explanations, yet the physical and AoE4 symptoms were identical.

The apparently contradictory `G-SYNC mode: fullscreen only` line is a setting stored inside this application profile, not proof that Godot turned the base/global toggle on or off. Godot itself writes that value and disables OpenGL threaded optimization. NVIDIA Control Panel supplies the Fixed Refresh/VRR-disabled settings. Together they form the nine-setting `Godot Engine` profile seen after the test.

### The NVIDIA App row is not the deleted profile

After the exact-path DRS profile was deleted, NVIDIA App still displayed a sidebar row named `Godot_v4.6.3-stable_win64.exe`. A fresh comparison shows that this is NVIDIA App's separate manually-added application-catalog record, not a recreated DRS profile.

NVIDIA App's `ApplicationStorage.json` still contains:

```text
LocalId: 963528738
DisplayName: Godot_v4.6.3-stable_win64.exe
DetectedFiles: C:\Users\k\Program\Godot_v4.6.3-stable_win64.exe\Godot_v4.6.3-stable_win64.exe
IsManuallyAdded: true
IsFingerprintDetected: false
DriverProfile: empty
LastLaunchTime: 2026-08-28 15:15:35 PDT
```

At the same time, live DRS still reports:

```text
FindProfileByName("Godot_v4.6.3-stable_win64.exe"): not found
Full-path executable lookup: Godot Engine
Basename executable lookup: Godot Engine
Total profiles: 7958
```

These are different identity stores. NVIDIA Profile Inspector deleted the driver profile but does not edit NVIDIA App's private application catalog. The NVIDIA App `2/2 Programs` count refers to accepted application-catalog rows, not to DRS profiles. Merely seeing the row therefore does not show that the exact-path profile returned.

The remaining `Godot Engine` DRS profile is also intentional in the latest test: it was the "other" profile that the user retained and changed to Fixed Refresh. NVIDIA's backend resolved the running executable to `Godot Engine` at 15:15:38 and 15:16:38, agreeing with the direct DRS query.

The screenshot has AoE4 selected, so it provides no evidence that NVIDIA App successfully loaded or recreated a Godot profile. Selecting or editing the stale/manual Godot row could cause NVIDIA App to run its profile resolution/creation path again; preserve the current state and query DRS afterward if that behavior is tested.

### Selecting the NVIDIA App row creates an empty orphan profile

The user then selected the Godot row in NVIDIA App and made no other issue-related change. This action was not read-only. A DRS query immediately afterward found:

```text
Before selection:
  total profiles: 7958
  profile named Godot_v4.6.3-stable_win64.exe: absent

After selection:
  total profiles: 7959
  profile named Godot_v4.6.3-stable_win64.exe: present
  application associations: 0
  explicit settings: 0
```

The NVIDIA App log records the failed second half of the operation:

```text
15:40:39.264  NvAPI_DRS_CreateApplication for the exact Godot path failed with -167
15:40:39.265  cannot create DRS app - Godot_v4.6.3-stable_win64.exe
```

NVAPI status `-167` is `NVAPI_EXECUTABLE_ALREADY_IN_USE`: the executable is already associated with another profile. The direct DRS query identifies that other profile as `Godot Engine`. The active DRS database and selector were nevertheless saved at 15:40:39, and the profile count increased by one. The resulting call sequence is therefore:

1. NVIDIA App creates `Godot_v4.6.3-stable_win64.exe` as a profile;
2. it tries to associate the exact executable path;
3. DRS rejects that association because the executable basename is already owned by `Godot Engine`; and
4. NVIDIA App saves the partial transaction, leaving a zero-application, zero-setting orphan.

NVIDIA App then explicitly queries `ProfileName: Godot_v4.6.3-stable_win64.exe`. That explains the screenshot's inherited/global-looking values, including `Monitor Technology: Global - G-SYNC Compatible`. Those values belong to the empty orphan profile's global inheritance; they are not the settings selected for the executable at runtime.

The runtime lookup remains unambiguous and unchanged:

```text
Full-path executable lookup: Godot Engine
Basename executable lookup: Godot Engine
Godot Engine G-SYNC application override: Fixed Refresh
Godot Engine VRR requested state: disabled
```

NVIDIA's separate backend independently logged `Profile name: Godot Engine` for the same executable at 15:40:43 and 15:40:49. NVIDIA App is therefore internally inconsistent: its settings page is addressed to the newly created orphan by profile name, while executable-based driver/backend resolution selects `Godot Engine`.

`ApplicationStorage.json` was not changed by the selection and still has an empty `DriverProfile` for manual application `LocalId 963528738`. AoE4's profile also remains unchanged and G-SYNC-capable. Selecting the row did not change which profile either executable will use, but it did recreate the named DRS artifact the user had deleted.

### Clean pre-bypass-launch baseline

The user subsequently deleted the Godot entry from NVIDIA App and then deleted the remaining Godot profile in NVIDIA Profile Inspector. The current state is clean in both NVIDIA identity stores.

NVIDIA App successfully removed manual application `LocalId 963528738` at 15:46:09. Its current `ApplicationStorage.json` contains zero case-insensitive occurrences of `godot`. Deleting the row also removed the empty orphan profile created by the selection test. NVIDIA Profile Inspector then removed the remaining `Godot Engine` profile at 15:46:23.

The DRS profile count confirms two removals overall. The intermediate count is inferred from NVIDIA App's successful profile-removal log and the two separate DRS writes:

```text
Before the two deletions: 7959
Inferred after NVIDIA App deletion: 7958
Measured after both deletions: 7957
```

Direct target lookups now return only not-found statuses:

```text
FindApplicationByName(full path): -166, NVAPI_EXECUTABLE_NOT_FOUND
FindApplicationByName(basename): -166, NVAPI_EXECUTABLE_NOT_FOUND
FindProfileByName("Godot_v4.6.3-stable_win64.exe"): -163, NVAPI_PROFILE_NOT_FOUND
FindProfileByName("Godot_v4.6.3-stable_win64"): -163, NVAPI_PROFILE_NOT_FOUND
```

To rule out differently named profiles or other Godot executables, a read-only exhaustive scanner inspected all 7957 DRS profiles and every enumerated application record. It searched case-insensitively for `godot` in profile names and each application's `appName`, `userFriendlyName`, `launcher`, `fileInFolder`, and `commandLine` fields:

```text
Profiles scanned: 7957
Matching profiles: 0
Matching applications: 0
Profile enumeration/info failures: 0
Application enumeration failures: 0
Audit complete: CLEAN
```

No Godot or NVIDIA Profile Inspector process was running during this audit. This is the required pre-launch baseline for the direct D3D12/no-OpenGL-fallback test: before that launch, NVIDIA has no persistent Godot application record, profile name, executable association, or other `godot`-containing DRS entry.

The target project's current `[rendering]` configuration is also ready for a fail-closed bypass test:

```ini
rendering_device/driver.windows="d3d12"
rendering_device/fallback_to_opengl3=false
rendering_device/fallback_to_vulkan=false
```

With those settings and the explicit `--rendering-driver d3d12` command, this test will either initialize D3D12 or abort. It cannot silently fall back to native OpenGL, which is the Godot 4.6.3 path that calls `_nvapi_setup_profile()` and writes DRS.

### Direct D3D12 bypass result

The user ran the direct editor command from the clean baseline and observed:

```text
Editor opened: yes
Monitor blank/blink: none
G-SYNC indicator: visible
Pointer movement: choppy
```

The post-launch persistent state is exactly the clean baseline:

```text
DRS profile count: 7957
Known Godot profile names: absent
Known executable full-path and basename associations: absent
Exhaustive case-insensitive Godot profile/application matches: 0
DRS file timestamps and SHA-256 hashes: unchanged
NVIDIA App catalog Godot matches: 0; file hash unchanged
```

NVIDIA's backend observed the running executable at 15:55:42 and classified it with an empty DRS profile name. It loaded DRS for classification but did not save it. This agrees with the direct NVAPI queries and raw file hashes.

The result establishes three points:

1. successful D3D12 startup with native-OpenGL fallback disabled does not reach Godot 4.6.3's `_nvapi_setup_profile()` path;
2. the approximately three-second dual-monitor blank is absent when the Godot DRS save/reload and Fixed Refresh profile are both absent; and
3. without the per-application suppression, the editor activates G-SYNC and reproduces the unstable/choppy interaction that motivated Godot's workaround.

The test changed two trigger inputs together: it removed Fixed Refresh profiles and bypassed the OpenGL DRS save. It therefore verifies the safe bypass but, by itself, does not distinguish which of those two inputs is strictly necessary for the earlier sticky NVIDIA state. The earlier tests establish that the combination is sufficient.

The subsequent AoE4 validation completed the negative control. The user opened AoE4, observed the top-right G-SYNC indicator, and closed it. AoE4's persistent profile remained unchanged, both DRS database files and the selector retained the clean-baseline timestamps and hashes, and Godot remained absent from DRS and NVIDIA App's catalog. Therefore merely running the D3D12 Godot editor with G-SYNC active does not wedge the live VRR path.

Closing the editor window did not return the command prompt within ten seconds, so the user closed the console window. NVIDIA's one-minute process sampler still listed the Godot image at 15:55:42 and no longer listed it at 15:56:42; a direct query at 15:57:59 found no Godot process. Project/editor metadata writes ended at 15:54:45. No Application Hang, WER, TDR, `nvlddmkm`, or display event was recorded. This is best treated as a separate, not-yet-isolated shutdown-linger observation rather than evidence of profile editing or a driver reset.

### Godot saved DRS again at the failure transition

The active NVIDIA DRS files changed at exactly the project-open transition:

```text
C:\ProgramData\NVIDIA Corporation\Drs\nvdrsdb1.bin  2026-08-28 14:49:19 PDT
C:\ProgramData\NVIDIA Corporation\Drs\nvdrssel.bin  2026-08-28 14:49:19 PDT
```

This was not one-time profile creation. Both Godot profiles and all their settings already existed before 14:49, and the before/after semantic DRS query has no changed line. Godot rewrote the database even though its desired settings were already present.

The one-profile repetition shows the same behavior more directly. NVIDIA's active DRS files were written at 15:15:04 when the Control Panel profile change was applied and again at 15:15:32 when the first Godot process started:

```text
15:15:32.762  Godot project-manager process observed by NVIDIA
15:15:32.915  nvdrsdb1.bin and nvdrssel.bin rewritten
15:15:35.244  spawned Godot editor process observed by NVIDIA
```

The two settings that Godot's source writes were already present in `Godot Engine` from the earlier profile creation. The current nine-setting profile is consistent with NVIDIA Control Panel adding Fixed Refresh to those existing Godot settings, followed by Godot re-saving them. There was no prelaunch raw DRS snapshot in this test, so exact line-for-line semantic identity cannot be claimed for the 15:15:32 write; the source call and coincident database rewrite are established.

### Godot's source contains an unconditional save, but it is not required for the bug

Godot 4.6.3's native Windows OpenGL manager calls `_nvapi_setup_profile()` during `GLManagerNative_Windows::initialize()`. This explains how Godot creates/updates its profile, but the Unity control proves this save is not required for the NVIDIA failure.

That routine:

1. loads NVIDIA DRS;
2. finds or creates a profile using the project/application name;
3. finds or creates a basename application association;
4. sets OpenGL threaded optimization;
5. writes NVIDIA setting `0x1194F158` to fullscreen-only; and
6. calls `NvAPI_DRS_SaveSettings()` unconditionally.

The project manager defaults to native `opengl3` / `gl_compatibility`, so this runs even though the project itself specifies D3D12. The D3D12 editor is spawned only after the OpenGL project-manager process has initialized.

Godot added this behavior in commit [`b8edc643`](https://github.com/godotengine/godot/commit/b8edc64379b3c4b5f2e7334468be65fd44a4980c), explicitly to disable windowed G-SYNC because of unstable editor refresh rates. The current 4.6.3 implementation is in [`gl_manager_windows_native.cpp`](https://github.com/godotengine/godot/blob/4.6.3-stable/platform/windows/gl_manager_windows_native.cpp). The original editor/VRR problem is tracked in [Godot issue #38219](https://github.com/godotengine/godot/issues/38219).

### This was not a TDR or GPU crash

For 14:40 through 15:00 there were:

- no `Display`, `nvlddmkm`, `DxgKrnl`, or display-related `Kernel-PnP` System events;
- no System event 4101;
- no new `LiveKernelReports` dump; and
- no WER report around the reproduction.

The only warning/error in the narrow interval was an unrelated Game Bar DCOM timeout at 14:50:09.

The three-second blank is therefore best understood as display-pipeline/profile reconfiguration rather than Windows recovering from a driver timeout.

## Trigger sequence

1. Two external PA display heads are active; one may be non-VRR.
2. A G-SYNC-allowed control application works normally.
3. An application matched to an NVIDIA Fixed Refresh profile starts.
4. NVIDIA reconfigures the display/VRR path and the monitors blank for two to three seconds.
5. The Fixed Refresh application runs without G-SYNC and exits.
6. The VRR-capable external target remains `VRR possible=1` but becomes stuck at `displayInVrrMode=0`.
7. An unrelated G-SYNC-allowed or unprofiled application cannot activate G-SYNC.

Unity establishes this sequence without a DRS write on driver 596.49. Godot follows the same visible sequence on 616.56 but additionally invokes its profile writer during native-OpenGL startup.

## Recovery

Global G-SYNC off/on in NVIDIA App is now a verified recovery.

The user performed no other issue-related action between the failed AoE4 test and this sequence:

1. opened NVIDIA App;
2. disabled G-SYNC and applied;
3. enabled G-SYNC and applied;
4. closed NVIDIA App;
5. opened AoE4 and observed the top-right G-SYNC indicator; and
6. closed AoE4.

NVIDIA's logs independently corroborate the control changes:

```text
15:09:34.521  Set global GsyncState=0, globalVRRMode=0
15:09:37.814  SetGlobalGsyncState returned success (3528.9 ms)
15:09:43.082  Set global GsyncState=1, globalVRRMode=1
15:09:43.086  SetGlobalGsyncState returned success (233.4 ms)
15:09:54      NVIDIA App records the subsequent AoE4 launch
15:11:37.470 NVIDIA backend detects the AoE4 session ending
```

The DRS database writes at 15:09:34 and 15:09:43 align with the off and on applies. After the test, AoE4 still selected `Age of Empires IV` with G-SYNC allowed and fullscreen-only VRR; no AoE4-specific repair or profile change was needed.

This recovery sharply strengthens the live-state diagnosis. Cycling the global setting forces the display/VRR path to be programmed through an actual disabled state and back to enabled, clearing the stale Fixed Refresh condition that merely launching AoE4 could not clear.

Rebooting should also rebuild the display session but was not tested. A Windows graphics-driver reset may recover it, but that has not been verified in this investigation.

Merely restarting AoE4 is not sufficient in the reported reproduction. Re-saving its existing profile is also unlikely to help because its stored settings are already correct.

## Workarounds and isolation tests

### Verified low-risk D3D12 bypass

Bypass the OpenGL project manager and start this D3D12 editor directly:

```text
"C:\Users\k\Program\Godot_v4.6.3-stable_win64.exe\Godot_v4.6.3-stable_win64.exe" --editor --path "C:\Users\k\Repository\Godot\VsyncStutterTest\Godot" --rendering-driver d3d12
```

The direct D3D12 launch has now been runtime-verified from a state with no Godot profile or application association. It bypassed the project manager's native-OpenGL initialization, avoided Godot's `_nvapi_setup_profile()`/`NvAPI_DRS_SaveSettings()` call, and left DRS byte-for-byte unchanged. Because no Fixed Refresh profile existed, the editor inherited G-SYNC behavior and the user observed the indicator and choppy pointer movement.

The command alone is not a fail-closed guarantee. Godot 4.6.3 defines `rendering/rendering_device/fallback_to_vulkan`, `fallback_to_d3d12`, and `fallback_to_opengl3` as `true`. On Windows, a requested D3D12 startup tries D3D12, may try Vulkan, and, if both RenderingDevice backends fail, may switch to native OpenGL. That native OpenGL fallback constructs `GLManagerNative_Windows`, whose `initialize()` calls `_nvapi_setup_profile()`.

For a source-level guarantee that a failed D3D12 initialization cannot reach Godot's NVAPI DRS writer, set this project setting before the test:

```ini
[rendering]
rendering_device/fallback_to_opengl3=false
```

Optionally also set `rendering_device/fallback_to_vulkan=false` if the requirement is specifically D3D12-or-fail rather than merely no native-OpenGL fallback. With OpenGL fallback disabled, a D3D12/Vulkan failure aborts display-server initialization instead of entering the only Godot 4.6.3 Windows code path that references NVAPI or writes DRS.

The clean baseline was captured again immediately after the direct launch. DRS remained at 7957 profiles, the exhaustive `godot` audit remained clean, and all raw DRS hashes were identical.

This guarantee is narrowly about Godot's explicit NVIDIA settings mutation. A successful D3D12 editor necessarily loads `D3D12.dll` and `DXGI.dll`, creates a D3D12 device, and uses the NVIDIA display driver. NVIDIA may read and apply the executable's existing DRS profile as part of normal process/device initialization; that is not Godot editing the profile.

The AoE4 physical check after the clean bypass succeeded: the top-right G-SYNC indicator appeared. The direct launch is therefore a verified partial workaround for the profile mutation, monitor blank, and post-Godot loss of AoE4 G-SYNC in this environment. It leaves G-SYNC enabled and pointer movement choppy in the editor, so it does not solve the original per-app-control goal.

### Other practical options

- After each affected Godot session, use the now-verified global G-SYNC off/on recovery before gaming.
- Report both tested NVIDIA branches. The original Godot reproduction used 616.56, and the Unity control reproduced the same sticky failure on 596.49.
- For a durable Godot-side fix, build Godot with the NVAPI profile setup removed or changed so it does not save DRS when the profile already has the desired values.
- Removing only the exact-path profile is now proven insufficient if the remaining basename profile is also set to Fixed Refresh.
- Removing Fixed Refresh from every profile that can match the Godot executable should avoid this particular transition, but it also restores the previously observed unwanted G-SYNC activation/choppy pointer behavior in the editor. It is therefore a tradeoff, not a complete fix.

## Upstream bug split

### NVIDIA

Primary defect: with both external PA278QGV display targets active, a Fixed Refresh application plus a DRS reload/profile transition leaves a later G-SYNC-allowed application unable to activate G-SYNC even though NVIDIA's persistent global and application-profile state remain enabled. The initial reproduction additionally confirmed the live API and both display flags remained enabled. The same Fixed Refresh Godot profile behaves smoothly when only one external PA target is active.

Minimal environment data:

- GeForce RTX 5070 Ti Laptop GPU
- ASUS ROG Strix G18 G815LR-IS97
- original Game Ready driver 616.56; current Unity reproduction and topology controls on 596.49
- NVIDIA App 11.0.8.299
- two ASUS PA278QGV displays reported by NVIDIA App
- Windows build 26200

### Godot

Integration defect: the native OpenGL manager writes and saves NVIDIA DRS on every initialization, including the OpenGL project manager, even when the desired profile already exists and no setting changes. The resulting driver-wide profile reload is disproportionate for an editor startup and can expose display/VRR transition bugs.

Godot's basename-wide profile creation can conflict or combine with per-application settings created in NVIDIA tools. The split exact-path/basename arrangement documented earlier is confusing but, as the one-profile test proves, it is not necessary for this defect.

## Confidence and remaining uncertainty

- High confidence: global G-SYNC was not persistently disabled.
- High confidence: AoE4's profile was not changed and still allows G-SYNC.
- High confidence: Godot rewrote DRS at the blink despite no semantic setting change.
- High confidence: no TDR/display-driver crash was recorded.
- High confidence: the failed state is recoverable by reprogramming global G-SYNC off/on without repairing either application profile.
- High confidence: the failure is a sticky NVIDIA runtime VRR state rather than persistent global or application-profile damage.
- High confidence: duplicate profiles, exact-path matching, and NVIDIA App profile creation are not required.
- High confidence: merely selecting the stale/manual NVIDIA App row recreates a named but empty, unassociated DRS profile because NVIDIA App saves a partial profile-creation transaction after `NvAPI_DRS_CreateApplication` fails.
- High confidence: the Godot values displayed by NVIDIA App after that selection do not describe the profile selected for the executable at runtime.
- High confidence: immediately before the planned bypass launch, all known and exhaustively scanned Godot entries are absent from DRS and NVIDIA App's application catalog.
- High confidence: the direct D3D12/no-fallback launch does not create or edit a Godot NVIDIA profile and does not rewrite DRS.
- High confidence: the direct D3D12/no-profile launch avoids the dual-monitor blank and permits G-SYNC to activate in the editor.
- High confidence: active editor G-SYNC correlates with the user's choppy pointer movement, reproducing the behavior Godot's NVIDIA profile workaround targets.
- High confidence: AoE4 G-SYNC remains usable after the direct D3D12 editor session; the bypass avoids the sticky live-VRR failure.
- High confidence: the smooth one-external arm still has the matching Fixed Refresh/VRR-disabled Godot profile.
- High confidence: the two PA monitor identities are separate NVIDIA DisplayPort connector targets 8450 and 8452 on the same RTX adapter, not MST children or different-GPU paths.
- High confidence: the current one-external state has the internal panel plus one PA active; this is not a generic single-display configuration.
- High confidence: both external PA targets being active is a necessary condition in every reported bad arm; the internal panel is neither necessary for failure nor sufficient to cause it.
- High confidence: the inactive second PA may remain cabled, powered, and physically enumerated without triggering the bug; active scanout, not presence, is required.
- High confidence: two active external display heads are necessary in the bad arms but not sufficient; TB5/DisplayPort plus HDMI is smooth through the same Unity Fixed Refresh transition.
- High confidence: HDMI at 119.998 Hz works with the TB5/DisplayPort PA also at 119.998 Hz while internal eDP is active; the earlier 60-Hz HDMI mode was not necessary.
- High confidence from the user's repeated A/B: internal eDP active versus Windows-disconnected is the discriminator within the TB5/DisplayPort-plus-HDMI topology; HDMI MediaSync on/off is not.
- High confidence: disconnecting internal eDP does not immediately break the neutral G-SYNC control; it starts smooth with the indicator.
- High confidence: Unity Fixed Refresh with internal eDP off reproduces severe transient blanking but not sticky G-SYNC loss; two later controls recover normally.
- High confidence: lowering internal-off HDMI from 120 Hz to 60 Hz makes the primary DP public VRR query succeed again; external mode/load affects driver state before any editor opens.
- High confidence: internal-off 120/60 Unity changes primary target 8450 from VRR mode 1 to 0 without a DRS write, and later G-SYNC fails.
- High confidence: the bug is not a simple high aggregate scanout-load threshold; 120/120 produces transient repeated blanks with recovery, while 120/60 produces sticky loss.
- Leading conclusion: active internal eDP is the only tested stabilizer that prevents both manifestations in the DP+HDMI topology.
- High confidence: Windows display reconstruction by reconnecting eDP restores the stuck primary to VRR mode without toggling global G-SYNC; HDMI refresh also changes during that operation.
- High confidence: the reconstruction recovery is functional, not merely an NVAPI-state change; the later neutral control is smooth with the indicator and target 8450 remains in VRR mode 1.
- High confidence: duplicating internal eDP with the HDMI PA retains three active physical NVIDIA targets but produces only two Windows desktop sources; the primary DP PA remains separate and all targets initially remain in VRR mode 1.
- High confidence: each external connector works smoothly as the lone external target, so a single defective port is unlikely.
- High confidence: Unity and Godot editors share the same poor-two-external/smooth-one-external result, so general editor smoothness is not a Godot-specific defect.
- High confidence: Godot's DRS save/reload is not required for the blank or sticky failure; Unity reproduced both while the DRS database remained unchanged.
- High confidence: Godot's DRS save is also not sufficient for the blank or sticky failure; it saved across four routed-topology launches, only the initial cold/topology transition blanked, and none poisoned later G-SYNC.
- High confidence: internal-panel launch placement is not sufficient for the blank; a deliberate repeated internal-panel launch did not blank.
- High confidence: with internal eDP active, the 120-Hz TB5/DisplayPort primary plus 120-Hz HDMI secondary configuration preserves smooth Fixed Refresh Godot and later smooth G-SYNC without manual global toggles.
- Unresolved: whether the earlier single blank recurs after another hotplug/topology reconstruction. Reboot did not invalidate the Fixed Refresh profile, but exposed the internal-panel dependency.
- High confidence: AoE4 G-SYNC works normally before either editor in the mixed-MediaSync two-external topology, so that topology does not globally break VRR for all presentation paths.
- High confidence: after Unity Fixed Refresh, target 8450 remains VRR-capable but changes from `displayInVrrMode=1` to `0`; this is direct API evidence of the sticky live state.
- High confidence: `VsyncStutterTest.exe` has no DRS association yet loses G-SYNC after the Unity transition, proving the failure propagates through inherited live/global state.
- High confidence: the same sticky transition occurs on 596.49 with Unity and occurred on 616.56 with Godot.
- Resolved isolation: activation of an existing Fixed Refresh profile alone is sufficient in the two-external topology; the Unity transition proved this without a DRS write.
- Separate remaining issue: reproduce the editor-window-close/process-linger behavior with process and verbose shutdown capture if it matters independently.
- Primary unresolved requirement: find a stable per-application method that disables G-SYNC only for Godot while preserving G-SYNC elsewhere, without global toggling or monitor blanks.
