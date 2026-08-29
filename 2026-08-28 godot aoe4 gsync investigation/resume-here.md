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

No tested two-external-monitor configuration meets all five requirements. A new physical-topology A/B shows that the same Fixed Refresh Godot profile works smoothly when only one PA278QGV is connected.

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

Current NVIDIA driver: 596.49 (`r596_25`), installed at 16:30:31 PDT. The original investigation used 616.56. The current smooth arm is confirmed on 596.49; capture the failing two-external arm on the same driver before calling this conclusively cross-branch.

Leading hypothesis: two identical PA278QGV Adaptive-Sync targets are active. NVIDIA's published mixed-monitor guidance says no more than one display should have G-SYNC enabled, while NVIDIA's setup help applies enablement to all connected displays of a selected model. Earlier evidence reported both PAs enabled.

Next test, in order:

1. connect and enable both PAs;
2. turn Adaptive-Sync off in the secondary PA's OSD only;
3. recover global G-SYNC once and confirm via `nvapi-vrr-query.cpp` that only the intended PA is VRR-possible;
4. repeat the ordinary Fixed Refresh Godot → AoE4 sequence;
5. if it still fails, leave both connected but disable the secondary in Windows and repeat; and
6. separately verify that each physical TB5/USB-C port works smoothly with exactly one PA.

If step 4 succeeds, leaving Adaptive-Sync disabled on the secondary PA is the first plausible stable workaround that keeps both monitors connected and avoids per-session global G-SYNC toggling.

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

## Earlier untested experiment

Potential future test, only if the user accepts the known risk:

1. capture a fresh clean DRS/catalog/hash baseline;
2. create exactly one Fixed Refresh profile associated with the Godot executable, without using the stale NVIDIA App manual row;
3. launch only through the verified direct D3D12/no-fallback command;
4. check for a monitor blank, editor G-SYNC indicator, DRS writes, and process exit;
5. close Godot and immediately test the AoE4 G-SYNC indicator; and
6. compare every DRS hash and association with the baseline.

This previously separated Fixed Refresh profile activation from Godot's native-OpenGL DRS save/reload. The new topology A/B is more discriminating and should be done first.

Do not treat this experiment as a recommendation or proven workaround. It deliberately reintroduces the condition associated with the failure.

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
