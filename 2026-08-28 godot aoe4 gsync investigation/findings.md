# Godot 4.6.3 / Age of Empires IV G-SYNC investigation

Investigation date: 2026-08-28 PDT

## Conclusion

Opening the Godot project is not persistently turning off the NVIDIA global G-SYNC setting, and it is not changing the Age of Empires IV profile.

The failure is a live NVIDIA driver-state problem. The Godot project-manager path starts in native OpenGL and unconditionally saves its NVIDIA DRS profile. When the profile selected for Godot is configured as Fixed Refresh, that save/reload transition blanks both displays for roughly three seconds. On driver 616.56, the VRR presentation path then remains stuck in the fixed-refresh state after Godot exits, even though the stored global setting and AoE4 profile still allow G-SYNC.

This explains the apparently contradictory observations:

- No indicator in the Godot editor is expected from whichever matching Godot profile is configured as Fixed Refresh.
- No indicator in AoE4 afterward is the defect: the driver does not successfully transition back from the Godot fixed-refresh state.
- The settings UI and DRS database still show G-SYNC enabled because the persistent configuration was not turned off.

A subsequent recovery test confirmed this interpretation: disabling global G-SYNC, applying, enabling it again, and applying restored the G-SYNC indicator in AoE4 without any Godot or AoE4 profile edit.

A later one-profile isolation test made the trigger narrower. The exact-path NVIDIA App profile was deleted, and the remaining basename `Godot Engine` profile was set to Fixed Refresh in NVIDIA Control Panel. Godot still produced the same three-second blank, and AoE4 still failed to activate G-SYNC afterward. Therefore duplicate profiles, exact-path matching, and the NVIDIA App-created profile are not required for the failure. The common condition is a Fixed Refresh Godot profile combined with Godot's project-manager DRS save/reload.

The best classification is an NVIDIA multi-display VRR/profile-transition bug exposed by Godot's unconditional NVAPI profile save. The original reproduction used driver 616.56. The later topology A/B was reported after driver 596.49 had been installed, and the current smooth one-external arm is confirmed on 596.49 with the Fixed Refresh profile still present. Capturing the live failing two-external arm again will close the remaining driver-version confounder. Godot's behavior remains an important trigger and an avoidable integration problem, but the failure to restore G-SYNC for a later application is a driver/display-path failure.

The direct D3D12 bypass has now been runtime-verified from a completely clean Godot/NVIDIA baseline. With both rendering fallbacks disabled, the editor opened without a monitor blank, showed the G-SYNC indicator, and exhibited the user's choppy pointer movement. Afterward, every DRS file remained byte-for-byte identical to the pre-launch baseline, the profile count remained 7957, the exhaustive Godot audit remained clean, and NVIDIA App's private catalog remained free of Godot. The user then launched AoE4 and observed its G-SYNC indicator normally. This proves the direct D3D12 path avoids Godot's NVIDIA profile writer, the display blank associated with it, and the sticky live-VRR failure seen after the ordinary project-manager path. The choppy G-SYNC editor behavior is the tradeoff that Godot's profile-writing workaround was designed to suppress.

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
| Clean direct D3D12/no-fallback launch with no Godot profile | G-SYNC active; pointer is choppy | G-SYNC activates normally | No DRS mutation or monitor blank | Safe partial workaround, but does not meet editor requirement |
| Explicit global G-SYNC off/on in NVIDIA App or NVIDIA Control Panel | Can force desired global state manually | Works after re-enable | Cumbersome and causes a long monitor blank | Recovery/manual toggle, not a per-app solution |

Therefore the direct D3D12 procedure is a verified workaround for profile mutation and the sticky NVIDIA state, but it is not a solution to the core per-application requirement. Calling it a complete workaround would overstate the result.

One potentially useful combination remains untested: create one Fixed Refresh Godot profile, then use only the verified direct D3D12/no-fallback launch so Godot itself never saves DRS. That experiment could determine whether profile activation alone is safe and, if so, might provide the desired per-app behavior. It could also reproduce the sticky failure, so it should be treated as a deliberate future isolation test rather than an established recommendation.

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

A follow-up matrix narrows this further. Two external PAs behave poorly both with the internal panel active and with it disabled, even when OSD MediaSync is off on one external PA. One MediaSync-enabled external PA plus the internal panel behaves smoothly. The same result occurs in both the Godot editor and Unity editor. Therefore the leading condition is **two active external display heads**, not the internal panel, not merely two active displays in total, probably not two VRR-enabled panels, and not a Godot-specific editor implementation. The VRR-eligibility point still requires an NVAPI capture of the bad MediaSync-off state to prove that the OSD change reached the driver.

Unity's matching behavior separates two phenomena that were previously entangled:

1. Editor motion/smoothness is broadly poor under the two-external topology, independently of Godot's NVIDIA integration.
2. Godot's native-OpenGL project-manager path additionally saves/reloads its Fixed Refresh DRS profile, producing the observed monitor blank and, in the failing topology, the sticky loss of AoE4 G-SYNC after Godot exits.

Godot is therefore not the root cause of the general dual-external editor smoothness problem. It remains a specific trigger for the more severe DRS transition failure.

The bad-topology baseline subsequently verified the MediaSync distinction in NVAPI before Godot opened. External target 8450 reports VRR possible and a 20583-us maximum frame interval; external target 8452 reports VRR impossible, not in VRR mode, and a zero maximum frame interval. Both external heads are nevertheless active at 2560x1440/119.998 Hz. This proves the poor editor behavior does not require two VRR-enabled external monitors. It requires the second active external head under the tested configurations.

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
2. Godot's native-OpenGL startup saves/reloads DRS while the matching Godot profile is Fixed Refresh;
3. NVIDIA reprograms multiple external display/VRR targets and both monitor device nodes may transiently disappear/re-enumerate; and
4. the driver does not restore a usable G-SYNC presentation path for a later AoE4 session.

With only one external PA target active, the same matching Fixed Refresh profile does not produce the user-visible failure. Godot's unconditional save is therefore a trigger, but it is not independently sufficient on this machine.

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

The mixed-MediaSync bad-topology baseline is complete and proves target 8452 is non-VRR. Without opening Godot or Unity, test AoE4 in the captured state and record indicator, motion smoothness, and monitor refresh behavior. This separates a topology-only G-SYNC problem from the known Godot-induced sticky transition.

Next, leave both PAs physically connected but disable one in Windows. A smooth result would identify the number of active external scanout paths rather than physical connection presence. If two active PAs remain the discriminator, route one PA through the laptop's HDMI output and one through a TB5/USB-C DisplayPort output:

- smooth TB5+HDMI would isolate the dual-TB5/USB-C display route;
- poor TB5+HDMI would implicate the NVIDIA/Windows two-external-head path more generally; and
- two PAs at 60 Hz remains a lower-priority bandwidth/link-allocation test.

NVIDIA's public support article for mixed-monitor VRR says multiple monitors may be connected but no more than one should have G-SYNC enabled. NVIDIA's setup help also describes its display enablement as applying to every connected display of the selected model. Both PAs have the same model/EDID, and the earlier NVIDIA App capture reported G-SYNC enabled on both. Those documents originally made dual enabled PA278QGVs the leading hypothesis. The MediaSync-off test now points more broadly to two active external heads, subject to verifying the OSD-off state in NVAPI; the older support article is not proof of the exact 2026 driver defect.

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

### Godot's source contains the triggering save

Godot 4.6.3's native Windows OpenGL manager calls `_nvapi_setup_profile()` during `GLManagerNative_Windows::initialize()`.

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

1. AoE4 runs with its profile and activates G-SYNC.
2. AoE4 exits.
3. Godot starts through the project manager.
4. The process matches a Godot profile configured as Fixed Refresh. In the latest test this is the sole basename `Godot Engine` profile.
5. The project manager uses native OpenGL and invokes `_nvapi_setup_profile()`.
6. Godot saves DRS, causing a profile reload/reapplication.
7. NVIDIA reconfigures the display/VRR path; both monitors blank.
8. Godot correctly remains Fixed Refresh, but driver 616.56 fails to restore usable VRR activation after Godot exits.
9. AoE4 later matches the correct G-SYNC-allowed profile, but the G-SYNC indicator never activates because the live VRR path is still stuck.

Steps 4 through 6 are directly established. Step 7 is established by the user's physical observation at the same timestamp. Step 8 is an inference from the enabled stored configuration, the verified global-toggle recovery, and the failed runtime activation; it is the explanation consistent with all recorded state.

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
- Test another NVIDIA driver branch/version. The sticky failure was observed on Game Ready 616.56; no other version was tested here.
- For a durable Godot-side fix, build Godot with the NVAPI profile setup removed or changed so it does not save DRS when the profile already has the desired values.
- Removing only the exact-path profile is now proven insufficient if the remaining basename profile is also set to Fixed Refresh.
- Removing Fixed Refresh from every profile that can match the Godot executable should avoid this particular transition, but it also restores the previously observed unwanted G-SYNC activation/choppy pointer behavior in the editor. It is therefore a tradeoff, not a complete fix.

## Upstream bug split

### NVIDIA

Primary defect: with both external PA278QGV display targets active, a Fixed Refresh application plus a DRS reload/profile transition leaves a later G-SYNC-allowed application unable to activate G-SYNC even though NVIDIA's persistent global and application-profile state remain enabled. The initial reproduction additionally confirmed the live API and both display flags remained enabled. The same Fixed Refresh Godot profile behaves smoothly when only one external PA target is active.

Minimal environment data:

- GeForce RTX 5070 Ti Laptop GPU
- ASUS ROG Strix G18 G815LR-IS97
- original Game Ready driver 616.56; current one-external control on 596.49
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
- High confidence: the narrower trigger is two active external display heads, not two VRR-enabled monitors; NVAPI verified the second external PA is non-VRR in the bad topology.
- High confidence: each external connector works smoothly as the lone external target, so a single defective port is unlikely.
- High confidence: Unity and Godot editors share the same poor-two-external/smooth-one-external result, so general editor smoothness is not a Godot-specific defect.
- High confidence: Godot's DRS save/reload is still a separate trigger for the display blank and sticky post-Godot VRR failure; Unity's matching smoothness result does not absolve that integration behavior.
- Remaining confounder: capture the two-external failure arm again while driver 596.49 is still installed.
- Optional remaining isolation: combine a Fixed Refresh Godot profile with the verified direct D3D12/no-save launch to distinguish profile activation alone from the DRS save/reload. This would deliberately reintroduce risk and is not required to validate the workaround.
- Separate remaining issue: reproduce the editor-window-close/process-linger behavior with process and verbose shutdown capture if it matters independently.
- Primary unresolved requirement: find a stable per-application method that disables G-SYNC only for Godot while preserving G-SYNC elsewhere, without global toggling or monitor blanks.
