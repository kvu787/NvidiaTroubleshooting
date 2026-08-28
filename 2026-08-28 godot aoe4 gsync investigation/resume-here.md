# Resume point: unresolved per-application Godot G-SYNC control

Paused: 2026-08-28 16:19 PDT

## Status

The core user goal is unresolved.

Required end state:

- G-SYNC disabled in the Godot editor.
- G-SYNC enabled in AoE4 and other intended applications.
- No manual global G-SYNC toggle between applications.
- No long monitor blank.
- No sticky loss of G-SYNC after Godot exits.

No tested configuration meets all five requirements.

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

## Most discriminating untested experiment

Potential future test, only if the user accepts the known risk:

1. capture a fresh clean DRS/catalog/hash baseline;
2. create exactly one Fixed Refresh profile associated with the Godot executable, without using the stale NVIDIA App manual row;
3. launch only through the verified direct D3D12/no-fallback command;
4. check for a monitor blank, editor G-SYNC indicator, DRS writes, and process exit;
5. close Godot and immediately test the AoE4 G-SYNC indicator; and
6. compare every DRS hash and association with the baseline.

This separates Fixed Refresh profile activation from Godot's native-OpenGL DRS save/reload. A success might yield the desired per-app behavior. A failure would show that activating Fixed Refresh alone is enough to wedge VRR on driver 616.56.

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
