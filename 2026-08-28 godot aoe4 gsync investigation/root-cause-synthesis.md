# Root-cause synthesis

Investigation state: 2026-08-29 PDT. Reboot persistence of the mixed TB5/DisplayPort-plus-native-HDMI clone workaround is validated for one reboot. The first Godot launch after that reboot blinked once; subsequent launches in that boot were clean.

## Bottom line

The evidence supports a layered diagnosis rather than one application bug:

1. **Primary defect — very high confidence:** the NVIDIA Windows display driver sometimes fails to restore live VRR mode on the primary DisplayPort target after a Fixed Refresh application transition. Persistent G-SYNC configuration remains enabled, but the target is left capable of VRR and outside VRR mode.
2. **Topology-dependent enabling condition — high confidence:** the failure depends on how active scanout targets, source surfaces, connector routes, timings, and likely display-engine resources are allocated on this laptop. It is not explained by monitor count, VRR capability, bandwidth, or refresh rate alone.
3. **Proximal trigger - very high confidence:** presenting a window matched to an already stored Fixed Refresh profile on the G-SYNC-capable primary target is sufficient in a susceptible allocation. Godot and Unity demonstrate the broader transition class; the latest Godot placement A/B localizes the current failure to the editor surface entering primary DP.
4. **Godot integration defects — confirmed contributors, not the primary cause:** Godot's native OpenGL startup writes NVIDIA DRS state and saves it even when the desired values already exist. Its fullscreen-only setting did not reliably suppress editor G-SYNC here, and its basename profile collides with NVIDIA App's exact-path/catalog behavior. Those writes increase transition and profile-management complexity, but Unity reproduces the display failure without them and Godot writes safely in the stable topology.
5. **NVIDIA App profile-management defects — confirmed but separate:** NVIDIA App's application catalog does not faithfully expose all underlying DRS profiles and can create/adopt them in the wrong order. This explains the disappearing/reappearing and duplicate Godot profile observations, not the later loss of VRR in unrelated applications.

The most precise current label is:

> A topology-sensitive NVIDIA VRR/Fixed-Refresh transition-state bug at the Windows presentation/display-pipeline boundary, exposed by editor profiles and aggravated—but not caused—by Godot's DRS writer.

The exact private driver routine or physical display-engine resource that fails cannot be identified with public APIs. Terms such as “display-head allocation,” “timing/clock domain,” “VidPN reconstruction,” “DirectFlip/MPO path,” or “Type-C output routing” are plausible mechanism classes, not individually proven root causes.

## Decisive causal evidence

### The global setting is not being turned off

After a damaging Unity transition in the internal-off DP-120/HDMI-60 topology:

- the primary target remained VRR-capable;
- its NVIDIA `displayInVrrMode` state changed from `1` to `0`;
- the HDMI target remained in mode `1`;
- the global G-SYNC setting and DRS files remained unchanged; and
- unprofiled `VsyncStutterTest.exe`, not only AoE4, lost its indicator and became choppy.

Cycling global G-SYNC or reconstructing the display topology restored the primary target to mode `1` and restored functional G-SYNC. This is live per-target driver state, not a saved global-setting change.

### Fixed Refresh activation is sufficient; a DRS write is not required

Unity's existing Fixed Refresh profile reproduced the monitor blank and later VRR failure while DRS timestamps and hashes stayed unchanged. Therefore:

- Godot does not need to edit the driver database for the failure to occur;
- deleting duplicate Godot profiles cannot solve the underlying transition defect; and
- the dangerous operation is the runtime transition into and out of Fixed Refresh in a susceptible topology.

Godot's DRS save is also not sufficient: repeated Godot starts changed the DRS database in the stable clone topology without causing a recurring blink or later G-SYNC loss.

### The current failure is target-placement dependent

After a healthy neutral control, Godot opened smoothly on the native-HDMI-plus-eDP clone. Moving the running Fixed Refresh editor onto the primary DP PA caused a three-second all-monitor blank. After exit, the neutral control was choppy without the indicator, and target 8452 had changed from `displayInVrrMode=1` to `0`.

The controlled inverse then succeeded: after recovery, Godot opened, remained, and closed entirely on the HDMI clone. The immediate primary-DP neutral control retained smooth G-SYNC, and target 8452 remained in mode `1`. Godot still saved/reloaded DRS and the profile contents were identical.

This proves that, in the current connector-1/source-0 allocation:

- process launch, project open, DRS save, editor use, and exit on HDMI are insufficient;
- the physical TB5 jack is insufficient, because the same result occurred after moving to the other jack while public connector/head state remained identical; and
- presenting the Fixed Refresh surface on primary DP is necessary for the reproduced sticky failure.

Earlier placement repetitions were clean in a different volatile allocation. Window placement is therefore not a universal root cause; it is the operation that selects the bad per-target transition when the underlying NVIDIA allocation is susceptible.

### Active route/resource allocation controls the outcome

The full A/B matrix rules out simple explanations:

| State | Fixed Refresh transition result |
| --- | --- |
| One external PA plus internal eDP | Clean |
| Two PAs on the two TB5/USB-C DisplayPort outputs | Bad with eDP active or inactive |
| Same two physical PAs, but one Windows-disabled | Clean |
| Primary TB5/DP PA plus native-HDMI PA, eDP active | Clean in the earlier allocation at HDMI 60 or 120 Hz; latest allocation fails when Fixed Refresh editor enters primary DP |
| Primary TB5/DP PA plus HDMI PA, eDP inactive, both 120 Hz | Severe transient blanking; later VRR recovered in the tested sequences |
| Primary TB5/DP PA at 120 plus HDMI PA near 60, eDP inactive | One transition caused sticky primary-target VRR loss |
| Primary TB5/DP PA separate; internal eDP cloned with HDMI at 1440p | Repeated transitions clean in the earlier allocation; current allocation is clean only while Godot remains on HDMI clone |
| Same 1440p eDP+HDMI clone after reboot | Sticky-loss prevention persists; first Godot launch blinks once, later launches are clean |
| Same clone concept but both external PAs on TB5/DisplayPort | G-SYNC issues return; native HDMI restores smooth behavior |

Important deductions:

- A second cable or powered monitor is insufficient; it must be an active Windows scanout target.
- Two active external heads are necessary in the observed bad arms but not sufficient, because DP+HDMI with active eDP is clean.
- Active eDP changes the allocation even when it is not a separate desktop: cloning eDP with HDMI remains stable. The important fact is the active physical target/path, not an extra Windows workspace. Active/cloned eDP is not sufficient in the dual-TB5/DisplayPort route.
- Changing HDMI from 120 to about 60 Hz changed the failure from transient blanking to sticky loss. Lower load made the outcome worse, which refutes a simple pixel-bandwidth or “too much refresh rate” threshold.
- Both TB5/USB-C ports work with one PA, and both external targets are separate NVIDIA DisplayPort connectors rather than an MST branch. A single bad port and MST are unlikely.
- MediaSync off on the second PA did not prevent the issue, and NVAPI confirmed that target was not VRR-capable in that test. Two simultaneously VRR-enabled external monitors are not required.

This pattern is most consistent with a route- and mode-dependent state-machine/resource-allocation defect inside the NVIDIA/WDDM display stack. The post-reboot clone A/B makes the connector family the strongest operational discriminator: two simultaneous external TB5/DisplayPort paths remain susceptible, while moving one PA to native HDMI selects a stable allocation.

## Why the monitors blank

The two-to-three-second all-monitor blank is best understood as a failed or disruptive modeset/reprogramming episode while the driver changes presentation/VRR policy. It resembles link retraining or display-head reconstruction, but the investigation cannot prove the exact operation.

Windows Error Reporting later surfaced `0x1A8` DXGKRNL and `0x1B8` miniport black-screen live dumps from several investigation periods. These are live diagnostics rather than fatal bugchecks or conventional TDRs. Their signatures record source parameter `1`, which Microsoft maps to the black-screen hotkey; whether a user or utility invoked that path is awaiting confirmation. They corroborate display-stack black-screen diagnostics but do not identify the failing NVIDIA resource.

Some sticky failures occurred after one blank, while some severe repeated blanks recovered normally. The post-reboot mixed-route clone now adds a case where the first Godot launch blinked once, later G-SYNC remained healthy, and later launches did not blink.

Therefore blinking is a symptom of the transition, not a reliable predictor of whether VRR will remain poisoned.

## Godot's exact role

Godot 4.6.3 contains an intentional NVIDIA workaround for unstable windowed G-SYNC editor behavior:

- native OpenGL initialization calls `_nvapi_setup_profile()`;
- it derives an application profile name and associates only the executable basename;
- it sets OpenGL threaded optimization and NVIDIA's G-SYNC mode to fullscreen-only; and
- it calls `NvAPI_DRS_SaveSettings()` after setting the values.

The upstream change explicitly describes windowed G-SYNC as buggy and capable of unstable editor refresh rates. That matches the observed active-indicator/choppy-pointer behavior when the direct D3D12/no-profile path was used.

However, Godot's chosen fullscreen-only DRS value did not reliably suppress G-SYNC in this editor. A separate Fixed Refresh override did. This produces two distinct Godot issues:

1. without Fixed Refresh, editor G-SYNC can activate and make mouse/editor motion choppy;
2. with Fixed Refresh, the NVIDIA driver may mishandle the transition in a susceptible topology.

The direct D3D12/no-fallback launch proved that the Godot writer can be bypassed: DRS remained byte-for-byte unchanged and later G-SYNC worked. It did not solve the desired behavior because G-SYNC stayed active in the editor and pointer motion was choppy.

## NVIDIA App and duplicate-profile observations

Those observations are a separate management layer:

- DRS can contain an exact-path profile and a basename `Godot Engine` profile at the same time.
- NVIDIA App starts from its own application catalog and exposes one selected profile per catalog row rather than enumerating the full DRS relationship.
- Its create-before-adopt behavior produced `EXECUTABLE_ALREADY_IN_USE` and `PROFILE_NAME_IN_USE` conflicts.
- Selecting a stale catalog row could recreate an empty/unassociated profile, making UI state look as if a deleted profile had returned.

These behaviors explain why NVIDIA App, NVCP, and NVIDIA Profile Inspector appeared to disagree. They do not explain why an unprofiled control application loses G-SYNC while persistent settings remain unchanged.

## Role of Windows presentation and laptop display routing

Windows flip-model applications can move between DWM composition, DirectFlip/Independent Flip, and multi-plane overlay paths depending on window and hardware state. VRR is also tied to swap-chain presentation behavior. The fixed-refresh editor transition therefore crosses both the NVIDIA policy layer and the Windows presentation/display pipeline.

The evidence places the visible stuck state in NVIDIA's per-display VRR state and shows NVIDIA controls/topology reconstruction recover it. It is reasonable to call this an NVIDIA driver bug at the Windows WDDM boundary. It is not possible from the current evidence to assign all blame to either NVIDIA's private display code or a Windows compositor/VidPN interaction.

On this hybrid laptop, all tested active targets were reported as owned by the RTX 5070 Ti. The internal eDP path nevertheless changes the stable allocation. That may involve the laptop's display mux/Advanced Optimus plumbing or simply a different NVIDIA scanout-resource assignment; neither is proven.

## Lower-confidence contributing factors and support boundary

NVIDIA's published mixed-monitor guidance says that multiple monitors may be connected but no more than one should have G-SYNC enabled. The two identical PAs are difficult to control independently through NVCP because NVIDIA's UI applies the checkbox by display model. This support limitation makes multi-monitor VRR fragility unsurprising, but it is not a complete explanation:

- the issue reproduced when the second PA's MediaSync was off and NVAPI reported it non-VRR;
- a three-active-target clone topology currently works; and
- the failure requires a particular Fixed Refresh transition and routing state rather than merely having two VRR targets.

Monitor firmware or link retraining may influence the visible blank duration, but the PA monitors are unlikely to be the primary cause. Each works individually, OSD MediaSync does not select the result, and software topology reconstruction changes the behavior without changing the hardware.

## What is ruled out or strongly disfavored

- Godot globally disabling the saved G-SYNC setting.
- AoE4 or `VsyncStutterTest.exe` profile corruption.
- Godot as the only affected application.
- Godot's DRS save as a necessary or sufficient cause.
- Duplicate Godot profiles or exact-path selection as necessary.
- Two VRR-capable external monitors as necessary.
- MediaSync on the secondary PA as necessary.
- A single defective TB5/USB-C port.
- MST or separate GPU ownership of the two external DP targets.
- A simple bandwidth, 120-Hz, or high-pixel-clock limit.
- A GPU crash/TDR.
- Physical TB5 jack as the sole selector when only one external DP path is active.

## Current workaround and remaining uncertainty

The current 2560x1440 topology keeps:

- the primary TB5/DisplayPort PA as one extended desktop source; and
- the HDMI PA cloned with the active internal eDP target as the second desktop source.

Across the original session and one reboot, repeated Unity and Godot Fixed Refresh transitions preserved later G-SYNC. A later reconstruction of the same visible state was not sufficient: moving Godot from the HDMI clone onto primary DP reproduced sticky loss. The visible topology therefore does not uniquely determine NVIDIA's volatile private allocation.

The new operational workaround for the current allocation is to keep Godot entirely on the native-HDMI-plus-eDP clone. Two consecutive controlled runs succeeded without an intervening recovery, and a third controlled run succeeded after a normal reboot without a global G-SYNC cycle: Godot opened and closed on HDMI, its expected DRS writes occurred, and each immediate primary-DP neutral control retained smooth G-SYNC with target 8452 still in VRR mode. This directly satisfies automatic per-application switching only if using the editor on the HDMI display is acceptable.

The same clone concept with both external PAs routed through TB5/DisplayPort remains bad, so clone mode and active eDP are not general solutions. Mixed routing plus HDMI-only editor placement avoids the current faulty transition; it is not a repair. Same-placement repetition and one normal reboot are validated. Sleep/hibernate, driver reset/update, monitor power-cycle, and hotplug-order durability remain untested.

## References

Local evidence:

- `findings.md`
- `evidence.md`
- `topology-ab-evidence.md`
- `unity-fixed-refresh-transition.md`
- `post-reboot-internal-panel-ab.md`
- `clone-internal-hdmi-baseline.md`
- `post-reboot-clone-route-validation.md`
- `../2026-08-28 gsync investigation/bugs-and-workarounds.md`
- `../2026-08-28 gsync investigation/two-godot-profiles-ui-explanation.md`
- `../2026-08-28 unity investigation/findings.md`

Relevant implementation/documentation:

- Godot 4.6.3: `C:/Users/k/Repository/External/Godot_4-6-3/platform/windows/gl_manager_windows_native.cpp`
- Godot origin commit: <https://github.com/godotengine/godot/commit/b8edc64379b3c4b5f2e7334468be65fd44a4980c>
- Microsoft VRR presentation: <https://learn.microsoft.com/windows/win32/direct3ddxgi/variable-refresh-rate-displays>
- Microsoft DXGI flip model: <https://learn.microsoft.com/windows/win32/direct3ddxgi/for-best-performance--use-dxgi-flip-model>
- NVIDIA G-SYNC setup: <https://www.nvidia.com/content/Control-Panel-Help/vLatest/en-us/mergedProjects/Display/To_use_variable_refresh_rates.htm>
- NVIDIA mixed-monitor guidance: <https://nvidia.custhelp.com/app/answers/detail/a_id/4766>
