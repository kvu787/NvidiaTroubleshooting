# Godot / AoE4 G-SYNC session-state investigation

Date: 2026-08-28

## User

i've observed some strange behavior. it seems like opening a godot 4.6.3 project is turning off gsync globally, not just disabling it for godot editor.

these are my exact steps:
- open and play aoe4 and observe the g-sync indicator at the top right
- close aoe4
- open godot 4.6.3 and open this project: "C:\Users\k\Repository\Godot\VsyncStutterTest\Godot"
- as the godot project is opening, observe a 3 second long blink on both attached monitors
- when the godot project is fully opened, observe no g-sync indicator at the top right
- do some general usage of the godot editor and observe no gsync behavior
- close godot editor
- open aoe4 and observe no g-sync indicator at the top right

what's going on?

do your all work in "C:\Users\k\Repository\NvidiaTroubleshooting\2026-08-28 godot aoe4 gsync investigation"

## Assistant

Godot is not actually saving the global G-SYNC setting as off. It is leaving NVIDIA driver 616.56's live VRR path stuck in Fixed Refresh, which has the same practical effect for the rest of the Windows session.

The decisive evidence after your reproduction is:

- NVIDIA's live API reports `globalGsyncState=1`, indicator enabled, and G-SYNC enabled on both ASUS displays.
- AoE4 still resolves to its normal profile with G-SYNC allowed; no AoE4 setting changed.
- Both Godot profiles are line-for-line unchanged from the earlier working snapshot.
- The active NVIDIA DRS database was nevertheless rewritten at 14:49:19, exactly at the monitor blink.
- No TDR, `nvlddmkm`, display-reset event, LiveKernelReport, or WER crash was recorded.

The trigger is the interaction between two Godot profiles. Your exact-path NVIDIA App profile forces Fixed Refresh. Separately, Godot's project manager starts in native OpenGL even though this project uses D3D12. Native OpenGL initialization finds/creates the basename `Godot Engine` profile, writes fullscreen-only G-SYNC plus threaded optimization, and calls `NvAPI_DRS_SaveSettings()` unconditionally—even when nothing changed. Godot intentionally added this path to suppress unstable windowed G-SYNC ([origin commit](https://github.com/godotengine/godot/commit/b8edc64379b3c4b5f2e7334468be65fd44a4980c)); it remains in the [4.6.3 source](https://github.com/godotengine/godot/blob/4.6.3-stable/platform/windows/gl_manager_windows_native.cpp).

That save reloads/reapplies profile state while the exact-path Fixed Refresh profile is active. The driver reconfigures the display pipeline—hence the three-second blank—but then fails to transition VRR back after Godot exits. AoE4 receives the correct G-SYNC-allowed profile, yet G-SYNC does not engage because the live path is wedged. So “globally off” accurately describes the symptom, but not the saved configuration.

For immediate recovery, toggle global G-SYNC off and back on in NVIDIA App, or reboot. The best low-risk isolation test is to bypass the OpenGL project manager and launch this D3D12 editor directly:

```text
"C:\Users\k\Program\Godot_v4.6.3-stable_win64.exe\Godot_v4.6.3-stable_win64.exe" --editor --path "C:\Users\k\Repository\Godot\VsyncStutterTest\Godot" --rendering-driver d3d12
```

If that avoids the blink and AoE4 G-SYNC works afterward, Godot's unconditional DRS save is the decisive trigger. If it still fails, merely selecting the exact-path Fixed Refresh profile is sufficient to trigger the NVIDIA bug.

The complete analysis is in `2026-08-28 godot aoe4 gsync investigation/findings.md`; the timestamped snapshot is in `evidence.md`. Both the investigation and conversation record are committed, and the working tree is clean.

---

## User

since the last prompt, i did the exact following and nothing else related to this issue:
- open nvapp
- disable gsync and apply
- enable gsync and apply
- close nvapp
- open aoe4 and observe gsync top-right indicator
- close aoe4

add this data to your investigation

## Assistant

Added. This is a successful recovery confirmation, not merely another observation.

NVIDIA's logs corroborate the sequence:

- global G-SYNC was set to disabled at 15:09:34 and the API reported success;
- it was set back to enabled at 15:09:43 and the API reported success;
- NVIDIA App recorded the subsequent AoE4 launch at 15:09:54; and
- NVIDIA's backend detected the AoE4 session ending at 15:11:37.

Your observation that the G-SYNC indicator returned during that run proves that the global off/on cycle recovers the failed state. A fresh DRS query afterward still selects the ordinary `Age of Empires IV` profile with G-SYNC allowed and fullscreen-only VRR, so no AoE4 profile repair occurred.

This materially strengthens the diagnosis: Godot did not permanently turn off G-SYNC or corrupt AoE4's profile. The driver had retained stale Fixed Refresh/runtime VRR state, and forcing one real global disable/enable transition reprogrammed the display path and cleared it. The remaining unresolved question is narrower: whether Godot's unconditional DRS save is required to create the stuck state or whether activating the exact-path Fixed Refresh profile alone is sufficient.

I updated `findings.md`, `evidence.md`, and this conversation record with the exact sequence, timestamps, DRS writes, post-recovery AoE4 state, and revised confidence assessment. The changes are committed and the working tree is clean.

---

## User

since the last prompt, i did the exact following and nothing else related to this issue:
- use npi to delete the godot profile shown in the screenshot
- use nvcp to turn gsync off for the other godot profile as shown in the screenshot
- open godot 4.6.3 and open this project: "C:\Users\k\Repository\Godot\VsyncStutterTest\Godot"
- as the godot project is opening, observe a 3 second long blink on both attached monitors
- when the godot project is fully opened, observe no g-sync indicator at the top right
- do some general usage of the godot editor and observe no gsync behavior
- close godot editor
- open aoe4 and observe no g-sync indicator at the top right

continue your investigation

Attached screenshots:

![NVIDIA Profile Inspector exact-path profile before deletion](<../2026-08-28 godot aoe4 gsync investigation/screenshots/npi-exact-path-profile-before-deletion.png>)

![NVIDIA Control Panel remaining Godot Engine profile set to Fixed Refresh](<../2026-08-28 godot aoe4 gsync investigation/screenshots/nvcp-godot-engine-fixed-refresh.png>)

## Assistant

This materially narrows the cause. The duplicate-profile arrangement is not required.

The post-test DRS query confirms that your deletion succeeded:

- the profile count fell from 7959 to 7958;
- `Godot_v4.6.3-stable_win64.exe` no longer exists as a profile name;
- both the executable's full-path and basename lookups now select the one remaining `Godot Engine` profile; and
- that profile has VRR disabled and the G-SYNC application override set to Fixed Refresh.

The timing also repeated the important correlation. NVIDIA first saw Godot at 15:15:32.762, and the active DRS database plus selector were rewritten at 15:15:32.915. The spawned editor process appeared at 15:15:35.244, aligning with your roughly three-second dual-monitor blank. AOE4 started at 15:16:07; afterward its DRS lookup still selected the normal `Age of Empires IV` profile with G-SYNC allowed and fullscreen-only VRR. Windows recorded no TDR, `nvlddmkm`, display-reset, hardware, or application-crash event in the interval.

This rules out three candidates as necessary causes: the deleted exact-path profile, exact-path-versus-basename profile competition, and NVIDIA App-specific profile creation. The strongest remaining model is now:

1. the sole profile selected for Godot is Fixed Refresh;
2. Godot's native-OpenGL project-manager process runs its unconditional NVAPI DRS save;
3. NVIDIA reloads/reapplies profile state and blanks both displays; and
4. driver 616.56 leaves the runtime VRR path stuck in fixed refresh after Godot exits.

The global off/on recovery remains valid. The next discriminating test is now cleaner: recover G-SYNC, confirm it in AOE4, then bypass the OpenGL project manager and launch the D3D12 editor directly:

```text
"C:\Users\k\Program\Godot_v4.6.3-stable_win64.exe\Godot_v4.6.3-stable_win64.exe" --editor --path "C:\Users\k\Repository\Godot\VsyncStutterTest\Godot" --rendering-driver d3d12
```

If that produces no blank and AOE4 G-SYNC still works afterward, Godot's DRS save/reload is required. If it still produces the sticky failure, merely activating a Fixed Refresh Godot profile is sufficient. I did not launch this test because its decisive evidence is your physical observation of the monitor blank and G-SYNC indicator, and it deliberately risks leaving the current display session stuck again.

I preserved both screenshots, hashes, the complete one-profile DRS snapshot, the process/DRS timeline, and the revised analysis in the investigation folder.

---

## User

if i do this:
"C:\Users\k\Program\Godot_v4.6.3-stable_win64.exe\Godot_v4.6.3-stable_win64.exe" --editor --path "C:\Users\k\Repository\Godot\VsyncStutterTest\Godot" --rendering-driver d3d12

will that ensure that godot doesn't touch nvidia driver settings at all?

cite the actual godot 4.6.3 source code

## Assistant

No—not as an unconditional guarantee from that command alone.

If D3D12 initializes successfully, the command avoids Godot 4.6.3's NVIDIA DRS writer. `--rendering-driver` is parsed directly into `rendering_driver` in [`main.cpp`](https://github.com/godotengine/godot/blob/35e80b3a8822a9df9be390814b62f44c0a9c69e8/main/main.cpp#L1290-L1293). On Windows, `d3d12` constructs `RenderingContextDriverD3D12`; native OpenGL instead constructs `GLManagerNative_Windows` in [`display_server_windows.cpp`](https://github.com/godotengine/godot/blob/35e80b3a8822a9df9be390814b62f44c0a9c69e8/platform/windows/display_server_windows.cpp#L7250-L7277).

The NVIDIA settings mutation is specifically attached to the native OpenGL manager. Its `initialize()` calls `_nvapi_setup_profile()` unconditionally in [`gl_manager_windows_native.cpp`](https://github.com/godotengine/godot/blob/35e80b3a8822a9df9be390814b62f44c0a9c69e8/platform/windows/gl_manager_windows_native.cpp#L505-L507). That routine loads `nvapi64.dll`, creates and loads a DRS session, writes the threaded-optimization and G-SYNC-mode settings, and calls `NvAPI_DRS_SaveSettings()` in [the same source file](https://github.com/godotengine/godot/blob/35e80b3a8822a9df9be390814b62f44c0a9c69e8/platform/windows/gl_manager_windows_native.cpp#L109-L162) and [its setting/save block](https://github.com/godotengine/godot/blob/35e80b3a8822a9df9be390814b62f44c0a9c69e8/platform/windows/gl_manager_windows_native.cpp#L241-L280).

The caveat is fallback. Godot 4.6.3 defines the Vulkan, D3D12, and OpenGL fallbacks as enabled by default in [`main.cpp`](https://github.com/godotengine/godot/blob/35e80b3a8822a9df9be390814b62f44c0a9c69e8/main/main.cpp#L2338-L2347). If D3D12 and its Vulkan fallback both fail, Windows startup is allowed to switch to `opengl3`, which then constructs the NVAPI-writing manager; see [`display_server_windows.cpp`](https://github.com/godotengine/godot/blob/35e80b3a8822a9df9be390814b62f44c0a9c69e8/platform/windows/display_server_windows.cpp#L7335-L7457).

To make this fail closed with respect to Godot's NVAPI writer, add this to the project's existing `[rendering]` section before running the command:

```ini
[rendering]
rendering_device/fallback_to_opengl3=false
```

If you specifically want D3D12-or-fail, also use:

```ini
rendering_device/fallback_to_vulkan=false
```

Then a D3D12 initialization failure aborts instead of reaching native OpenGL. On a successful D3D12 run, Godot still interacts normally with the NVIDIA driver: the D3D12 backend loads `D3D12.dll` and `DXGI.dll`, enumerates adapters, and creates a device in [`rendering_context_driver_d3d12.cpp`](https://github.com/godotengine/godot/blob/35e80b3a8822a9df9be390814b62f44c0a9c69e8/drivers/d3d12/rendering_context_driver_d3d12.cpp#L104-L165). NVIDIA will also read and apply the executable's existing profile. That is different from Godot editing NVIDIA Control Panel/DRS settings.

So the precise claim is: successful D3D12 startup avoids Godot's known NVIDIA settings writer; the command alone does not guarantee that native OpenGL fallback can never reach it. Disabling OpenGL fallback supplies that guarantee for stock Godot 4.6.3's explicit NVAPI/DRS code path.

---

## User

i still see a godot profile in the nvidia app (see screenshot) which i thought should have been removed at this point. what's going on?

![NVIDIA App showing the Godot application row after exact-path DRS profile deletion](<../2026-08-28 godot aoe4 gsync investigation/screenshots/nvidia-app-godot-inventory-after-drs-profile-deletion.png>)

## Assistant

The deleted profile has not returned. The circled item is NVIDIA App's separate application-catalog row, not proof of a DRS profile with that name.

A fresh read-only DRS query still reports:

```text
FindProfileByName("Godot_v4.6.3-stable_win64.exe"): not found
Total DRS profiles: 7958
Full-path executable lookup: Godot Engine
Basename executable lookup: Godot Engine
```

NVIDIA App independently retains this object in its private `ApplicationStorage.json`:

```text
LocalId: 963528738
DisplayName: Godot_v4.6.3-stable_win64.exe
DetectedFiles: the exact Godot executable path
IsManuallyAdded: true
DriverProfile: empty
LastLaunchTime: 2026-08-28 15:15:35 PDT
```

There are therefore three distinct objects:

1. `Godot_v4.6.3-stable_win64.exe`, the exact-path DRS profile you deleted in Profile Inspector. It is still gone.
2. `Godot Engine`, the basename DRS profile you deliberately retained and set to Fixed Refresh. It still exists and currently matches the executable.
3. NVIDIA App application row `LocalId 963528738`, which was created when Godot was manually added to NVIDIA App. NPI does not edit NVIDIA App's application catalog, so this row survived.

NVIDIA App's `2/2 Programs` count is counting accepted application-catalog rows, not DRS profiles. The empty `DriverProfile` field is particularly decisive: the visible row currently has no stored driver-profile name.

Your screenshot also has AOE4 selected in the detail pane. It only shows that NVIDIA App rendered the Godot catalog row; it does not show NVIDIA App loading or recreating a Godot DRS profile. Merely opening NVIDIA App did not recreate it, as confirmed by the current DRS query.

Be cautious about selecting or editing the Godot row during this controlled test. NVIDIA App may then run its profile-resolution/creation path again. If that behavior is tested, query DRS immediately afterward to determine whether it adopted `Godot Engine`, failed, or recreated an exact-path profile.

I preserved the screenshot and added the catalog-versus-DRS comparison to the investigation.

---

## User

i selected the godot profile in nvapp and did nothing else. check state again.

![NVIDIA App Godot row selected after DRS profile deletion](<../2026-08-28 godot aoe4 gsync investigation/screenshots/nvidia-app-godot-row-selected-after-drs-profile-deletion.png>)

## Assistant

Selecting the row recreated a DRS profile, but it is a broken empty orphan. It did not take ownership of the Godot executable and it is not the profile the driver will use for Godot.

The before/after state is decisive:

```text
Before selecting the row:
  total DRS profiles: 7958
  Godot_v4.6.3-stable_win64.exe profile: absent

After selecting the row:
  total DRS profiles: 7959
  Godot_v4.6.3-stable_win64.exe profile: present
  application associations: 0
  explicit settings: 0
```

NVIDIA App's own log explains the partial result:

```text
15:40:39.264  NvAPI_DRS_CreateApplication ... failed with code -167
15:40:39.265  cannot create DRS app - Godot_v4.6.3-stable_win64.exe
```

`-167` is `NVAPI_EXECUTABLE_ALREADY_IN_USE`. The executable is already associated with the existing `Godot Engine` profile. NVIDIA App created the new profile first, failed while adding the executable, then nevertheless saved DRS at 15:40:39. That left a zero-application, zero-setting profile behind.

This also explains the misleading values in the screenshot. NVIDIA App explicitly called `GetProfileInfo` for `ProfileName: Godot_v4.6.3-stable_win64.exe`, so the page shows the empty orphan's inherited globals, such as `Monitor Technology: Global - G-SYNC Compatible`. That is not the runtime profile selected for the executable.

The actual matching state remains:

```text
Full-path executable lookup: Godot Engine
Basename executable lookup: Godot Engine
Godot Engine monitor technology: Fixed Refresh
Godot Engine VRR requested state: disabled
```

NVIDIA's backend independently logged `Profile name: Godot Engine` for this exact executable at 15:40:43 and 15:40:49. AoE4 still resolves to `Age of Empires IV` with G-SYNC allowed and fullscreen-only VRR. `ApplicationStorage.json` also remained unchanged and still has an empty `DriverProfile` for the manual Godot row.

So the corrected model is: the profile really was absent before the click; selecting the stale catalog row recreated its name in DRS, but association failed and left an orphan. The NVIDIA App page now does not reflect what the driver will apply to Godot.

I preserved the screenshot and added the timestamped DRS comparison, NVIDIA logs, hashes, and current AoE4 state to the investigation.

---

## User

i've done the following:
1. delete godot profile from nvapp
2. delete godot profile from npi

check to see if all godot nvidia profiles are gone now. we need to ensure this before trying to launch a godot project in a way that bypasses nvidia profile editing.

## Assistant

Yes. All Godot-related NVIDIA state is gone at this checkpoint.

The direct DRS results are:

```text
Total profiles: 7957 (down from 7959)
Full-path Godot application lookup: -166, executable not found
Basename Godot application lookup: -166, executable not found
Godot_v4.6.3-stable_win64.exe profile lookup: -163, profile not found
Godot_v4.6.3-stable_win64 profile lookup: -163, profile not found
```

I also performed a broader audit rather than checking only those known names. A read-only NVAPI scanner enumerated all 7957 profiles and every application record, searching case-insensitively for `godot` in profile names, executable/application names, friendly names, launchers, folder restrictions, and command lines:

```text
Profiles scanned: 7957
Matching profiles: 0
Matching applications: 0
Profile enumeration/info failures: 0
Application enumeration failures: 0
Audit complete: CLEAN
```

NVIDIA App's separate `ApplicationStorage.json` is clean too: it was rewritten at 15:46:09, contains zero `godot` occurrences, and its log confirms successful deletion of manual application `963528738` and removal of the named DRS profile. The second DRS write at 15:46:23 aligns with your subsequent NPI deletion. No Godot or NVIDIA Profile Inspector process was running during the audit.

The target project is also already configured for a fail-closed launch:

```ini
rendering_device/driver.windows="d3d12"
rendering_device/fallback_to_opengl3=false
rendering_device/fallback_to_vulkan=false
```

Therefore, using the explicit direct `--editor --path ... --rendering-driver d3d12` command will either initialize D3D12 or fail; it cannot silently fall back to Godot 4.6.3's native-OpenGL NVAPI/DRS writer.

This guarantees the pre-launch state, not what NVIDIA or Godot might create during the next launch. The post-launch check should be run immediately after closing the directly launched editor, before opening NVIDIA App, NPI, or another Godot process. The expected unchanged result is 7957 profiles and a clean exhaustive `godot` audit.

The baseline, DRS hashes, deletion logs, application-catalog hash, project configuration, and audit source are preserved in the investigation.

---

## User

i did the following:
1. open a command prompt window
2. run this: `"C:\Users\k\Program\Godot_v4.6.3-stable_win64.exe\Godot_v4.6.3-stable_win64.exe" --editor --path "C:\Users\k\Repository\Godot\VsyncStutterTest\Godot" --rendering-driver d3d12`
3. the project opened without any monitor blinking
4. i observed the gsync indicator in the top right
5. i moved the mouse around and observed choppy movement
6. i closed the godot editor window
7. i waited for the command prompt window to return to the prompt, but it didn't do so after 10 seconds, so i clicked the windows UI "x" to close it

continue the investigation

## Assistant

The bypass worked exactly as intended: the direct D3D12 editor launch did not create or edit any NVIDIA profile and did not rewrite DRS.

The post-launch state is byte-for-byte identical to the clean baseline:

```text
DRS profiles: 7957
Godot full-path association: absent
Godot basename association: absent
Known Godot profile names: absent
Exhaustive Godot profile/application matches: 0
DRS enumeration failures: 0
NVIDIA App catalog Godot entries: 0
```

All three DRS timestamps and SHA-256 hashes are unchanged from before the launch. `ApplicationStorage.json` is also unchanged. NVIDIA's backend did see the running Godot executable at 15:55:42 and loaded DRS for normal classification, but its resolved profile name was empty and it performed no save. This is the clean distinction we wanted: the NVIDIA driver read its settings normally, while Godot did not mutate them.

Your physical observations fit that state:

- No monitor blank: the DRS save/reload transition did not happen.
- G-SYNC indicator visible: with no Fixed Refresh Godot profile, the editor inherited G-SYNC behavior.
- Choppy pointer movement: this reproduces the editor/VRR behavior that Godot's NVIDIA profile workaround was designed to suppress.

This establishes that successful D3D12 startup with OpenGL and Vulkan fallback disabled bypasses Godot 4.6.3's native-OpenGL `_nvapi_setup_profile()` writer. It also verifies a practical tradeoff: the bypass avoids the dangerous global driver-profile reload, but leaves G-SYNC active in the editor, where you experience choppy interaction.

The delayed command-prompt return is a separate observation. Project/editor metadata writes ended at 15:54:45. NVIDIA's periodic sampler still reported the Godot image at 15:55:42 but not at 15:56:42, and a direct check at 15:57:59 found no Godot process. There is no Application Hang, WER, TDR, `nvlddmkm`, display, or crash event. The one-minute NVIDIA sampling interval cannot establish the exact exit time, so this currently supports a shutdown linger but not a root cause. Closing the console may have terminated the lingering process.

Godot also semantically preserved but textually rewrote `project.godot`: CRLF became LF and the two `fallback_to_*` keys swapped order while retaining the same `false` values. I left this project-owned change untouched.

AoE4's stored profile remains G-SYNC-capable. The final physical check for this bypass run is now:

1. do not open NVIDIA App, NPI, or Godot;
2. open AoE4;
3. observe whether the top-right G-SYNC indicator appears; and
4. close AoE4.

If it appears, the bypass avoided both profile mutation and the sticky live-VRR failure. If it does not, Fixed Refresh/profile activation is not required and the remaining defect is deeper in NVIDIA's application transition path.

The post-launch hashes, exhaustive audit, NVIDIA process/classification lines, shutdown evidence, and updated conclusions are preserved in the investigation.

---

## User

i opened aoe4, observed the gsync indicator in the top right, and closed aoe4

## Assistant

That completes the control successfully. The direct D3D12 bypass preserved live G-SYNC for the next application.

NVIDIA's catalog records the AoE4 launch at 16:02:57 PDT, and the backend processed its application record at 16:03:06. The physical top-right indicator is the decisive live-state result. Persistent state remains correct:

```text
DRS profiles: 7957
Godot profile/application associations: none
AoE4 selected profile: Age of Empires IV
AoE4 G-SYNC override: allow
AoE4 VRR mode: fullscreen only
```

All three DRS files retain the exact timestamps and SHA-256 hashes from before the Godot launch. NVIDIA App's catalog changed only to record AoE4's new `LastLaunchTimeISO`; it still contains zero `godot` occurrences.

The end-to-end result is now:

```text
Ordinary project-manager path + Fixed Refresh Godot profile + Godot DRS save:
  monitor blank -> editor fixed refresh -> AoE4 G-SYNC fails

Clean direct D3D12/no-fallback path with no Godot profile:
  no monitor blank -> editor G-SYNC active/choppy -> no DRS change
  -> AoE4 G-SYNC activates normally
```

This proves that merely running the Godot D3D12 editor with G-SYNC active does not wedge VRR. The practical workaround is validated for all three relevant outcomes: it avoids NVIDIA profile mutation, avoids the dual-monitor blank, and preserves subsequent AoE4 G-SYNC.

The strict causal split between Fixed Refresh profile activation alone and Godot's OpenGL DRS save/reload remains optional. Testing that would require deliberately recreating a Fixed Refresh Godot profile and carries a known risk of wedging VRR again; it is not required to validate the safe workflow.

I added the physical observation, AoE4 launch timestamp, unchanged DRS hashes, current profile state, and final control conclusion to the investigation.

---

## User

even with all the information and workarounds we've gathered, i can't get gsync to be disabled for godot editor while being enabled for other stuff like aoe4, except for explicitly enabling/disabling gsync in the nvidia app or nvcp which is cumbersome and causes a long monitor blink when you toggle gsync on/off.

so, this core issue remains. record this. i may pick up this investigation later.

## Assistant

Recorded as the unresolved primary user goal.

The successful D3D12 isolation is now explicitly classified as a partial workaround, not a complete solution. The current tradeoff is:

```text
Ordinary Godot path + per-app Fixed Refresh:
  G-SYNC is disabled in Godot
  but monitors blank and AoE4 G-SYNC becomes stuck afterward

Clean direct D3D12 path with no Godot profile:
  no blank, no profile mutation, and AoE4 G-SYNC works afterward
  but G-SYNC remains active in Godot and pointer movement is choppy

Global G-SYNC off/on:
  provides manual control/recovery
  but is cumbersome and causes a long monitor blank
```

No tested configuration currently provides all five required properties:

1. G-SYNC disabled only for Godot;
2. G-SYNC enabled for AoE4 and other intended applications;
3. no manual global toggle between applications;
4. no long monitor blank; and
5. no sticky post-Godot VRR failure.

I added this distinction to the main findings and evidence, and created `resume-here.md` as the investigation handoff. It records the clean state, proven paths, exact unresolved requirement, the separate shutdown-linger observation, and the most discriminating untested experiment: one Fixed Refresh Godot profile combined with only the direct D3D12/no-save launch. That experiment is marked risky and unproven, not recommended.

The core issue remains open when this investigation is paused.

## User

i've found a clean a/b test that demonstrates these gsync issues vs does not demonstrate these gsync issues.

works smoothly
one PA278QGV connected to a thunderbolt 5 sport

repros issues
two PA278QGV connected, one to each thunderbolt 5 port

i am using an asus rog strix g18 G815LR-IS97

investigate

## Assistant

This is the strongest discriminator found so far. The investigation now treats the second external PA278QGV/display head as a necessary condition in the user's clean A/B, with Godot's DRS save remaining the trigger rather than a sufficient cause by itself.

The current smooth one-external state was captured and it still has a matching `Godot Engine` DRS profile for Godot 4.6.3 with:

```text
VRR requested state: disabled
G-SYNC application override: fixed refresh
G-SYNC mode: fullscreen only
OpenGL threaded optimization: disabled
```

Therefore the smooth result is not caused by losing the per-app Fixed Refresh override.

Windows and NVIDIA's read-only APIs show:

- the internal 2560x1600/240 Hz panel and the current 2560x1440/119.998 Hz PA278QGV are both owned directly by the RTX 5070 Ti Laptop GPU;
- the connected PA is Windows target 8452, external DisplayPort connector instance 1;
- the disconnected second PA is target 8450, external DisplayPort connector instance 0;
- the PA paths are separate NVIDIA DP connectors, not MST and not different-GPU paths;
- the current PA uses four-lane HBR2 at 8 bpc; and
- no downstream USB4 device router is present, so the monitor is using DisplayPort output through a TB5-capable USB-C port rather than a Thunderbolt display tunnel.

The current NVIDIA driver is 596.49 (`r596_25`), installed at 16:30:31 PDT. The original failure capture was on 616.56. The current smooth arm is confirmed on 596.49, but the failing two-external arm should be captured again without changing driver before calling the defect conclusively cross-branch.

The leading hypothesis is now two identical external Adaptive-Sync/G-SYNC-eligible targets. NVIDIA's published mixed-monitor guidance says that multiple monitors may be connected but no more than one should have G-SYNC enabled. NVIDIA's setup help also describes display enablement as applying to all connected displays of a selected model. Earlier evidence reported both identical PA278QGVs enabled.

The highest-value next test is:

1. connect and enable both PAs;
2. turn Adaptive-Sync off in the secondary PA's OSD only;
3. recover global G-SYNC once;
4. verify that only the intended PA remains VRR-capable; and
5. repeat the ordinary Fixed Refresh Godot to AoE4 sequence.

If that succeeds, the cause is narrowed to two external Adaptive-Sync targets and leaving Adaptive-Sync disabled on the secondary PA is a practical stable workaround. If it still fails, leave both cables connected but disable the secondary display in Windows; that distinguishes two active scanout paths from two VRR-capable displays.

The investigation is recorded in `2026-08-28 godot aoe4 gsync investigation/topology-ab-evidence.md`, and two reusable read-only probes were added: `display-topology-query.cpp` and `nvapi-vrr-query.cpp`.

---

## User

is it possible to enable gsync on certain monitors but not on others via software? i can't seem to do this in nvcp.

## Assistant

Yes at NVIDIA's lower-level software interface, but not through NVCP for this pair of identical PA278QGVs.

NVIDIA documents the NVCP checkbox as applying settings to all connected displays of a selected model. Both monitors identify as PA278QGV, so NVCP addresses the model, not one physical connector/serial-number instance. That explains why the UI cannot express the split.

NVIDIA's public NVAPI has a per-display call, `NvAPI_DISP_SetAdaptiveSyncData(displayId, ...)`. Its input includes `bDisableAdaptiveSync`, documented as controlling whether Adaptive Sync is disabled on that display. The two PAs have distinct display IDs, so a guarded custom utility can request disabled on one and enabled on the other without editing Godot/AoE4 application profiles.

This remains an untested runtime path on this machine. NVIDIA does not document it as a persistent replacement for NVCP; a hotplug, reboot, driver restart, or modeset may reset it, and applying it may itself cause a brief display blank. No setter was run. The per-monitor OSD toggle is still the safest persistent control.

The sensible software experiment, if requested, is a separate utility that is read-only by default, targets the exact connector/Windows target/NVAPI display ID, snapshots the original state, requires an explicit enable or disable command, verifies the result, and supports rollback.

Recorded in the investigation as commit `1683794`.

---

## User

i tried these configurations:
1. PA278QGVs with osd mediasync on, PA278QGVs with osd mediasync off, laptop display connected, gsync on
2. PA278QGVs with osd mediasync on, PA278QGVs with osd mediasync off, laptop display disconnected, gsync on
3. PA278QGVs with osd mediasync on, laptop display connected, gsync on

1 and 2 had poor behavior. only 3 seemed to behave well.

this is extremely frustrating because having gsync work smoothly is a very valuable feature and having two external monitors connected is also a very valuable feature.

is there a clear explanation for what's going on?
what is the next step in the investigation?

## Assistant

There is now a clear trigger-level explanation, but not yet a proven internal root cause.

Assuming cases 1 and 2 mean two external PAs, one with MediaSync on and one off, while case 3 means one external PA with MediaSync on:

- The internal panel is not the cause: case 2 fails without it.
- Total active-display count is not the cause: case 2 has two active external displays and fails, while case 3 has two active displays (internal plus external) and succeeds.
- A single defective USB-C/DP port is unlikely. The latest good-state probe used external target 8450/connector 0; the earlier good-state capture used target 8452/connector 1. Each port works when it is the only external output.
- The shared condition in every bad arm is two active external display heads.
- Two VRR-enabled monitors are probably not required because one PA had MediaSync off, but this is not proven until the failing topology is captured and NVAPI confirms the OSD-off state reached the driver.

The most accurate functional model is therefore a dual-external-head NVIDIA/display-routing problem. If the poor state happens only after ordinary Godot startup, Godot's Fixed Refresh DRS save/reload is the trigger and the dual-external topology is the condition that makes the NVIDIA VRR transition stick. If AoE4 is already poor before Godot, then the topology itself breaks G-SYNC and Godot is a separate aggravating transition. The current observations do not yet distinguish those two versions.

The immediate next test is:

1. Return to two active external PAs, one MediaSync on and one off.
2. Power-cycle or reconnect the MediaSync-off PA so its advertised capability is re-enumerated.
3. Do not open Godot. Capture the read-only Windows/NVAPI probes first and verify whether that PA reports `VRR possible=0` or `Adaptive-Sync disabled=1`.
4. Recover G-SYNC once and test AoE4 before Godot, recording separately whether the indicator appears, whether motion is smooth, and whether the monitor's refresh indication varies.
5. Leave both cables connected but disable one PA in Windows and repeat. A good result isolates active external scanout-head count.
6. If two active external PAs remain the discriminator, connect one over HDMI and the other over one TB5/USB-C DisplayPort output. Smooth TB5+HDMI isolates the dual-TB5 route; poor TB5+HDMI implicates any two-external-head NVIDIA path.

Testing both PAs at 60 Hz is useful afterward, but lower priority. It distinguishes link/clock pressure only after the routing question is answered.

The findings and next-test protocol were recorded as commit `56a2203`.

---

## User

let me clarify my configurations:

1. one PA278QGV with osd mediasync on, another PA278QGV with osd mediasync off, laptop display connected, gsync on
2. one PA278QGV with osd mediasync on, another PA278QGV with osd mediasync off, laptop display disconnected, gsync on
3. one PA278QGV with osd mediasync on, laptop display connected, gsync on
1 and 2 had poor behavior. only 3 seemed to behave well.

add a note that unity editor also shows poor behavior (in addition to the existing godot editor poor behavior) for 1 and 2, but works smoothly for 3.

## Assistant

Recorded the configurations exactly as clarified:

| Case | External displays | Internal panel | Godot | Unity |
|---|---|---|---|---|
| 1 | PA MediaSync on + PA MediaSync off | connected | poor | poor |
| 2 | PA MediaSync on + PA MediaSync off | disconnected | poor | poor |
| 3 | one PA MediaSync on | connected | smooth | smooth |

The Unity result is an important causal split. The topology-dependent editor smoothness problem is not specific to Godot's rendering engine or NVIDIA profile writer. Two active external display heads are the common condition for poor behavior in both editors.

Godot's ordinary project-manager path remains separately responsible for saving/reloading its Fixed Refresh DRS profile. That behavior is still associated with the three-second monitor blank and sticky failure to restore AoE4 G-SYNC. In other words:

- dual-external editor smoothness problem: broader NVIDIA/Windows/display-routing issue, reproduced by Unity and Godot;
- display blank and sticky post-editor VRR state: additional Godot DRS transition issue.

Updated `topology-ab-evidence.md`, `findings.md`, and `resume-here.md`. Investigation commit: `ccdc91c`.

---

## User

Connect both external PAs.
Set one PA’s MediaSync on and the other off.
Power-cycle or disconnect/reconnect the MediaSync-off PA.
Do not open Godot yet.
Tell me when that state is ready.

i've done this. the state is ready.

## Assistant

Captured the pre-Godot state. The result is decisive:

```text
PA target 8450 / connector 0:
  VRR possible=1
  displayInVrrMode=1
  maximum Adaptive-Sync interval=20583 us

PA target 8452 / connector 1:
  VRR possible=0
  displayInVrrMode=0
  maximum Adaptive-Sync interval=0
```

Both PAs remain active at 2560x1440/119.998 Hz as separate external DP heads. The internal target 8449 is also active at 2560x1600/240 Hz.

This proves the OSD MediaSync-off change reached NVIDIA. The poor two-external editor behavior cannot require two VRR-enabled external monitors: target 8452 is conclusively non-VRR. The common condition is two active external heads.

The next step is to keep this exact state, avoid Godot and Unity, and launch AoE4 before either editor. Observe the G-SYNC indicator, actual motion smoothness, and—if available—the MediaSync-on monitor's live refresh indication. Close AoE4 and report those observations without making another issue-related change.

Raw and interpreted evidence is saved in `two-external-mixed-mediasync-baseline.md`. Investigation commit: `8a2e9a2`.

---

## User

Do not open Godot or Unity.
Do not toggle G-SYNC.
Open AoE4 on the MediaSync-on PA.
Observe:- G-SYNC indicator present or absent;
- actual motion smoothness;
- live monitor refresh changes, if its OSD provides them.
Close AoE4 and report the observations.

i've done this. aoe4 runs with the top-right gsync indicator and gsync seems to work as expected.

## Assistant

Recorded as a successful pre-editor control. Two active external heads—one VRR-capable and one verified non-VRR—do not inherently break G-SYNC in AoE4. A read-only capture after AoE4 closed showed the same three active targets and unchanged per-display VRR capabilities.

This makes the result application/presentation-path dependent:

- AoE4: G-SYNC indicator present and behavior normal before either editor;
- Unity/Godot editors: poor behavior with two external heads;
- ordinary Godot profile transition: additionally associated with the display blank and sticky loss of later AoE4 G-SYNC.

The next clean separation is Unity followed by AoE4, with the display state unchanged and no G-SYNC toggle:

1. Do not open Godot.
2. Open the same Unity editor/project that shows poor behavior on the MediaSync-on PA.
3. Observe Unity, then close it.
4. Immediately open AoE4 and observe indicator and smoothness.
5. Close AoE4 and report both Unity and post-Unity AoE4 results.

If AoE4 remains normal, Unity's failure is confined to its own editor/windowed presentation path and Godot's persistent post-exit problem remains tied to its DRS transition. If AoE4 loses G-SYNC after Unity, the persistent transition defect is broader than Godot.

Investigation commit: `59323dd`.

---

## User

i did this:
Do not open Godot or toggle G-SYNC.
Open the same Unity editor/project that demonstrates poor behavior on the MediaSync-on PA.
Observe Unity, then close it.
Immediately open AoE4.
Observe its G-SYNC indicator and actual smoothness.
Close AoE4 and report both results.

for unity:
after a 2-3 second monitor blink, unity seemed to work okay and did not show any gsync, as expected because the nvcp profile for unity editor disabled gsync

for aoe4:
the top-right gsync indicator was missing and the gameplay was clearly choppy

also, i am switching from aoe4 to this simple 3d scene for testing gsync: "C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe"

for vsyncstuttertest:
the top-right gsync indicator was missing and the animation was clearly choppy

## Assistant

This is the decisive reproduction. The live read-only state and DRS audit show:

```text
Before Unity, target 8450:
  VRR possible=1
  displayInVrrMode=1
  AoE4 G-SYNC worked

After Unity, target 8450:
  VRR possible=1
  displayInVrrMode=0
  AoE4 and VsyncStutterTest lacked G-SYNC and were choppy
```

Unity's current `Unity 3D` profile explicitly contains:

```text
VRR requested state: disabled
G-SYNC application override: fixed refresh
G-SYNC mode: disabled
```

Both DRS database files were last written at 22:11:41. The successful pre-Unity control was at 22:36 and the failed post-Unity capture at 22:43. Their post-test hashes are identical. Unity did not save or reload DRS during this test. Activation of the already-stored Fixed Refresh profile alone caused the monitor blank and sticky external VRR state.

`VsyncStutterTest.exe` has no NVIDIA full-path association, basename association, or matching profile. Its failed result is therefore a clean inherited live/global-state control. It replaces AoE4 as the routine G-SYNC test from this point forward.

The root-cause conclusion is revised:

- not Godot-specific;
- not dependent on Godot's profile writer;
- reproduced by Fixed Refresh profile activation in Unity;
- conditional on the two-external-head topology;
- reproduced on NVIDIA 596.49 as well as the earlier Godot case on 616.56; and
- directly visible as target 8450 remaining VRR-capable while becoming stuck outside VRR mode.

The next action is one known recovery cycle: toggle global G-SYNC off/apply, then on/apply, without opening Unity, Godot, or `VsyncStutterTest.exe`, and report when ready. The read-only probe should then verify target 8450 returned to `displayInVrrMode=1` before establishing the neutral `VsyncStutterTest.exe` baseline.

Investigation commit: `d1882d3`.

---

## User

i did the following:
Toggle global G-SYNC off and apply.
Toggle it back on and apply.
Do not open Unity, Godot, or VsyncStutterTest.exe.
Tell me when ready.

ready

## Assistant

Recovery is verified at the NVIDIA API level:

```text
target 8450, MediaSync-on PA:
  VRR possible=1
  displayInVrrMode=1

target 8452, MediaSync-off PA:
  VRR possible=0
  displayInVrrMode=0
```

Target 8450 has returned from the post-Unity stuck value `displayInVrrMode=0` to the correct value `1`. The global toggle therefore repairs the exact external-display live state that the Fixed Refresh transition corrupts. The explicit applies rewrote DRS at 22:50:13, as expected.

Before changing the Windows display topology, establish the replacement control:

1. Do not change G-SYNC, display connections, MediaSync settings, or open Unity/Godot.
2. Run `C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe` on the MediaSync-on PA.
3. Observe the top-right G-SYNC indicator and animation smoothness.
4. Close it and report the result.

Investigation commit: `7734775`.

---

## User

i did this:
Now establish the replacement control:
1. Do not change G-SYNC, displays, or MediaSync settings.
2. Do not open Unity or Godot.
3. Run:
C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe
4. Observe the G-SYNC indicator and animation smoothness.
5. Close it and report the result.

VsyncStutterTest runs smoothly and displays the top-right gsync indicator

## Assistant

The replacement control is validated. `VsyncStutterTest.exe` has no NVIDIA profile, and in the recovered two-external topology it displayed the G-SYNC indicator and ran smoothly.

The 22:55 post-control capture remained correct:

```text
target 8450: VRR possible=1, displayInVrrMode=1
target 8452: VRR possible=0, displayInVrrMode=0
active Windows display paths: 3
```

Next topology isolation:

1. Keep both PA cables connected and both monitors powered.
2. In Windows Settings -> System -> Display, identify and select the MediaSync-off PA.
3. Under Multiple displays, choose `Disconnect this display` for that PA. Do not unplug it.
4. Do not open Unity, Godot, or `VsyncStutterTest.exe` yet.
5. Report when ready so the read-only probes can verify one external scanout head is active while the second PA remains physically connected.

Investigation commit: `993d6ef`.

---

## User

i did this:
Keep both PAs connected and powered.
Open Windows Settings → System → Display.
Select the MediaSync-off PA.
Under Multiple displays, choose Disconnect this display.
Do not physically unplug it.
Do not open Unity, Godot, or VsyncStutterTest.exe.
Tell me when ready.

ready

## Assistant

The intended topology arm is established:

```text
Windows active paths:
  internal target 8449
  MediaSync-on PA target 8450

MediaSync-off PA target 8452:
  active=0
  connected=1
  physicallyConnected=1
```

This is exactly one active external scanout head with the second PA still cabled and powered.

There is one important anomaly: `NvAPI_Disp_GetVRRInfo` returns generic `NVAPI_ERROR` for active target 8450 on two consecutive queries after the Windows topology change. Its Adaptive-Sync capability data still reports the expected 20583-us interval. The API error may be a query limitation/transient topology condition, or the Windows change may already have disturbed G-SYNC.

Before Unity:

1. Do not change G-SYNC or the display topology.
2. Do not open Unity or Godot.
3. Run `C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe` on target 8450.
4. Observe the top-right indicator and animation smoothness.
5. Close it and report the result.

If it works, this is a healthy baseline and the Unity transition can proceed. If not, global G-SYNC must be recovered in this topology before Unity is tested.

Investigation commit: `bd4451d`.

---

## User

i did this:
Do not change G-SYNC or display settings.
Do not open Unity or Godot.
Run VsyncStutterTest.exe on the active PA.
Observe the G-SYNC indicator and animation smoothness.
Close it and report.

VsyncStutterTest runs smoothly and displays the top-right gsync indicator

## Assistant

This validates a healthy one-active-external baseline even though the public NVAPI VRR-info call returns a generic error in this Windows-disconnected topology.

The 23:01 capture confirms:

```text
Windows active paths:
  internal target 8449
  MediaSync-on external target 8450

Still physically connected but inactive:
  MediaSync-off target 8452

VsyncStutterTest.exe:
  G-SYNC indicator present
  animation smooth
```

The DRS stores retain their 22:50:13 timestamps and captured hashes, providing a pre-Unity comparison point.

Now perform the decisive transition without any intervening change:

1. Keep G-SYNC and the display topology unchanged.
2. Do not open Godot.
3. Open the same Unity editor/project on target 8450.
4. Observe whether the monitors blink and whether Unity behaves normally without G-SYNC.
5. Close Unity.
6. Immediately run `C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe`.
7. Observe its G-SYNC indicator and animation smoothness.
8. Close it and report both the Unity and `VsyncStutterTest.exe` results.

If the control remains healthy, active external scanout-head count is the decisive topology condition. If it fails, the second PA's physical presence is sufficient even while Windows has disconnected it.

Investigation commit: `c55819f`.

---

## User

i did this:
Keep G-SYNC and display topology unchanged.
Do not open Godot.
Open the same Unity editor/project on the active PA.
Observe any monitor blink and Unity’s behavior.
Close Unity.
Immediately run VsyncStutterTest.exe.
Observe its G-SYNC indicator and smoothness.
Close it and report both results.

for unity:
there was no monitor blink. unity seemed to work okay and did not show any gsync.

for vsyncstuttertest:
runs smoothly and displays the top-right gsync indicator

## Assistant

This is the decisive result:

- With the second PA still connected and powered but disabled in Windows, Unity's Fixed Refresh transition caused no monitor blink.
- After Unity closed, `VsyncStutterTest.exe` retained the G-SYNC indicator and smooth motion.
- The post-test display topology and both NVIDIA profile-database hashes remained unchanged.

Therefore the sticky failure is not caused by the second PA merely being connected, Unity profile activation alone, or a new DRS/profile write. In the tested configurations, it requires **two external PA scanout heads active at the same time**.

This also gives us a reliable, if imperfect, workaround: disable the second PA in Windows before opening Unity or Godot. That preserves G-SYNC without the global off/on toggle, but temporarily sacrifices the second desktop.

The next A/B test should distinguish the dual-Thunderbolt/DisplayPort route from a general two-active-external-head problem:

1. Keep the MediaSync-on PA on its current TB5/USB-C DisplayPort route.
2. Disconnect the other PA from its TB5/USB-C route and connect it to the laptop's HDMI output.
3. Keep MediaSync off on that second PA and make both external PAs active in Windows. Leave the internal display as it currently is.
4. Do not open Unity, Godot, or `VsyncStutterTest.exe` yet.
5. Tell me when that state is ready so I can capture the topology before establishing the healthy control.

The result is recorded in the investigation notes.

---

## User

i did the following:
Keep the MediaSync-on PA on TB5/DisplayPort.
Move the other PA from TB5/DisplayPort to the laptop’s HDMI output.
Keep its MediaSync off and make both PAs active in Windows.
Don’t open Unity, Godot, or VsyncStutterTest.exe yet.
Tell me when ready so I can capture the new topology first.

ready

## Assistant

The capture is complete. The intended physical routing is confirmed:

```text
target 8450: PA278QGV, TB5/DisplayPort, 119.998 Hz, active
target 8448: PA278QGV, native HDMI, 59.951 Hz, active
target 8449: internal panel, active
```

All three paths are directly owned by the RTX 5070 Ti Laptop GPU. The NVIDIA DRS timestamps and SHA-256 hashes are unchanged, so the cable move did not edit any application profile.

There is one important discrepancy: although MediaSync remains off in the HDMI PA's OSD, NVIDIA reports HDMI target 8448 as `VRR possible=1`, `displayInVrrMode=1`, with a 20583-us Adaptive-Sync interval. The same OSD-off PA previously reported non-VRR when connected by DisplayPort. I am treating this as an observed HDMI/input or driver-state difference, not as evidence that you changed the OSD setting.

Now establish the pre-Unity functional control:

1. Do not change G-SYNC, display topology, refresh rates, or either PA's OSD settings.
2. Do not open Unity or Godot.
3. Run `C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe` specifically on the MediaSync-on TB5/DisplayPort PA (target 8450), not the HDMI PA.
4. Observe the top-right G-SYNC indicator and animation smoothness.
5. Close it and report the result.

If this control works, we can perform the Unity transition immediately. If it does not, the HDMI hotplug/modeset disturbed G-SYNC before Unity and we must recover the baseline first.

---

## User

before we do this, i have a question. are you able to detect if a PA278QGV has mediasync off, or are you simply assuming that?
on side note, i confirmed that the osd on the hdmi-connected PA278QGV still shows mediasync=off.

## Assistant

No—the current probes cannot directly read the PA278QGV's OSD MediaSync setting. The “MediaSync off” label came from your reported OSD setting. I was detecting NVIDIA's driver-side VRR state and, in a few notes, described that corroboration too strongly as if NVAPI had verified the OSD itself. I have corrected that language.

The distinction is:

- **Monitor fact:** you directly confirmed the HDMI-connected PA's OSD says `MediaSync=off`.
- **NVIDIA fact:** the driver currently reports that same HDMI target as `VRR possible=1`, `displayInVrrMode=1`, and `bDisableAdaptiveSync=0`.
- **Earlier DisplayPort fact:** after you confirmed MediaSync off on the DP-connected PA, NVIDIA reported that target as `VRR possible=0`, `displayInVrrMode=0`, and a zero Adaptive-Sync interval.

`NvAPI_Disp_GetVRRInfo` is NVIDIA's assessment of the display target. `NvAPI_DISP_GetAdaptiveSyncData` returns NVIDIA's per-display Adaptive-Sync state; its fields are associated with an NVIDIA setter and are not documented as reading the physical OSD. So the HDMI result is a real disagreement between the monitor's OSD state and NVIDIA's current classification, not evidence that MediaSync is actually on.

A truly direct software read would require the monitor to expose that exact setting over DDC/CI—probably as a vendor-specific VCP feature—and we have not established that the PA278QGV does. Until then, your OSD observation is authoritative.

This does not require changing the next test. It makes the TB5/DP-plus-HDMI arm especially informative: it tests the different route while preserving the fact that the HDMI monitor is physically set to MediaSync off, even though the driver still classifies it as VRR-capable.

---

## User

i did this:
Don’t change G-SYNC, topology, refresh rates, or OSD settings.
Don’t open Unity or Godot.
Run C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe specifically on the MediaSync-on TB5/DisplayPort PA.
Observe the G-SYNC indicator and animation smoothness.
Close it and report the result.

vsyncstuttertest runs smoothly and displays the top-right gsync indicator

## Assistant

The healthy pre-Unity baseline is confirmed:

- `VsyncStutterTest.exe` ran smoothly on the MediaSync-on TB5/DisplayPort PA and displayed the G-SYNC indicator.
- All three active display paths are unchanged.
- Target 8450 remains `VRR possible=1` and `displayInVrrMode=1`.
- The DRS timestamps and SHA-256 hashes are unchanged.

Now perform the decisive routing transition:

1. Do not change G-SYNC, display topology, refresh rates, or either monitor's OSD settings.
2. Do not open Godot.
3. Open the same Unity editor/project on the MediaSync-on TB5/DisplayPort PA.
4. Observe whether either monitor blinks and whether Unity behaves normally without G-SYNC.
5. Close Unity.
6. Immediately run `C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe` on that same TB5/DisplayPort PA.
7. Observe its G-SYNC indicator and animation smoothness.
8. Close it and report both results.

Interpretation:

- If Unity causes no sticky failure and the control remains smooth, the defect is specific to having both external PAs routed through the two TB5/USB-C DisplayPort outputs.
- If the sticky failure reproduces, the defect applies more generally to two active external NVIDIA scanout heads, independent of whether the second uses DisplayPort or HDMI.

---

## User

i did this:
Don’t change G-SYNC, topology, refresh rates, or OSD settings.
Don’t open Godot.
Open the same Unity editor/project on the MediaSync-on TB5/DisplayPort PA.
Observe whether either monitor blinks and whether Unity behaves normally without G-SYNC.
Close Unity.
Immediately run C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe on that same PA.
Observe its G-SYNC indicator and smoothness.
Close it and report both results.

for unity:
there was no monitor blink. unity seemed to work okay and did not show any gsync.

for vsyncstuttertest:
runs smoothly and displays the top-right gsync indicator

## Assistant

This is decisive on one point: two active external scanout heads are not sufficient.

- With both PAs active through the two TB5/USB-C DisplayPort outputs, Unity Fixed Refresh causes the monitor blank and sticky loss of later G-SYNC.
- With the primary PA on TB5/DisplayPort and the second PA on native HDMI, the same Unity transition causes no blank and `VsyncStutterTest.exe` retains G-SYNC and smooth motion.
- The 23:20 post-transition capture confirms target 8450 remains `displayInVrrMode=1`, all three paths are unchanged, and the DRS database is byte-for-byte unchanged.

The successful arm does have one important confound: moving the secondary to HDMI also changed it from 119.998 Hz to 59.951 Hz and created a different NVIDIA target/mode state. Therefore we have isolated a route/mode family, but we have not yet proved whether the decisive variable is HDMI versus the second TB5/DisplayPort route, the lower secondary refresh/link load, or another route-associated state difference.

This still gives us the first viable two-monitor workaround candidate: keep the G-SYNC/MediaSync-on PA at 119.998 Hz on TB5/DisplayPort and the fixed-refresh PA at 59.951 Hz on native HDMI. It is validated with Unity; the final practical step is to validate the original Godot workflow. A later HDMI-at-120-Hz or dual-TB5-at-60-Hz test can separate route from refresh/load.

The current `Godot Engine` profile still matches `godot_v4.6.3-stable_win64.exe` and explicitly requests VRR disabled plus Fixed Refresh, so this will be a real per-application test:

1. Do not change G-SYNC, display topology, refresh rates, or OSD settings.
2. Launch Godot 4.6.3 exactly as in the original reproduction—do not use the explicit `--rendering-driver d3d12` bypass—and open `C:\Users\k\Repository\Godot\VsyncStutterTest\Godot` on the TB5/DisplayPort PA.
3. Observe whether either monitor blinks, whether the G-SYNC indicator is absent in Godot, and whether editor motion behaves acceptably.
4. Close Godot.
5. Immediately run `C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe` on the same PA.
6. Observe its G-SYNC indicator and animation smoothness.
7. Close it and report both results.

If this matches Unity, the HDMI routing is a complete practical workaround for the original Godot problem while keeping both external monitors active.

---

## User

i launched godot 4.6.3 and then opened the project. the godot editor with the project launched on the laptop display instead of the tb5 pa primary display. it did a 2 second monitor blink. i moved the godot editor window to the primary tb5 pa monitor. i then closed godot

then i launched vsyncstuttertest and it ran smoothly and displayed the gsync indicator, and i closed it

for a second time, i launched godot 4.6.3 and then opened the project. the godot editor with the project launched on the primary monitor. it didn't show the gsync indicator and usage was smooth.

then for a second time i launched vsyncstuttertest and it ran smoothly and displayed the gsync indicator, and i closed it

## Assistant

This validates the practical per-application behavior:

- Godot runs smoothly without G-SYNC.
- `VsyncStutterTest.exe` regains smooth G-SYNC immediately afterward without a global toggle.
- That transition succeeded twice, and primary target 8450 remains `displayInVrrMode=1`.

The first two-second blank remains, but it is no longer tied to sticky G-SYNC loss. The two Godot runs also reveal an important confound:

- first run: project editor launched on the internal laptop display and blanked;
- second run: project editor launched on the primary TB5/DisplayPort PA and did not blank.

Godot rewrote the DRS databases on both launches—at 23:24:38 and 23:25:09—while the effective profile remained VRR disabled plus Fixed Refresh. Therefore Godot's unconditional profile save is not sufficient to cause the blank: the second save did not blank, and neither save poisoned later G-SYNC.

The next test should isolate window placement from a one-time cold transition:

1. Keep G-SYNC, topology, refresh rates, and OSD settings unchanged.
2. Launch Godot and open the same project. Record whether this setup launch blinks.
3. Move the project-editor window to the internal laptop display and close Godot there, so Godot can persist that placement.
4. Launch Godot and open the same project again.
5. Report which display the editor opens on and whether either monitor blinks. Confirm that Godot remains smooth without the G-SYNC indicator.
6. Close Godot.
7. Immediately run `C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe` on the primary TB5/DisplayPort PA.
8. Report its indicator and smoothness, then close it.

Interpretation:

- If the deliberately internal-panel launch blinks again, editor launch-display placement is the remaining trigger.
- If it does not blink, the earlier blank was a one-time cold transition after the topology/profile-state change.

The current HDMI arrangement is already a strong workaround for the damaging part of the bug: it preserves both monitors and correct per-application G-SYNC switching. The remaining question is whether its first-launch blank can also be eliminated reliably.

---

## User

i did the following:
Keep all display, G-SYNC, refresh-rate, and OSD settings unchanged.
Launch Godot and open the project. Record whether this setup launch blinks.
Move the project editor to the laptop display and close Godot there, allowing it to save that placement.
Launch Godot and open the project again.
Report where the editor opens and whether either monitor blinks. Confirm Godot remains smooth without G-SYNC.
Close Godot.
Immediately run VsyncStutterTest.exe on the primary TB5/DisplayPort PA and report its indicator and smoothness.

i observed no monitor blinking at any point.
godot editor always ran smoothly and didn't display gsync indicator.
vsyncstutter test ran smoothly and did display gsync indicator.

## Assistant

This completes the stable-use validation. The current configuration reliably provides the desired behavior:

- both external monitors remain active;
- Godot runs smoothly without G-SYNC;
- later intended applications regain smooth G-SYNC immediately, without a global toggle; and
- repeated Godot launches produce no monitor blank, including when the editor is persisted to the internal laptop display.

The final 23:33 capture confirms primary target 8450 remains `displayInVrrMode=1`, the MediaSync-off HDMI target remains outside VRR mode, and the effective Godot profile remains VRR disabled plus Fixed Refresh. Godot again saved DRS on both launches—at 23:31:52 and 23:32:20—without a blank or sticky failure.

Therefore neither editor window placement nor Godot's profile save is sufficient to cause the earlier isolated blank. That one blank is best classified as a transient first/cold transition after changing the physical topology or driver target state. It has not reproduced in stable use; recurrence after reboot or another hotplug remains untested.

Validated workaround:

```text
Primary PA278QGV:
  TB5/USB-C DisplayPort, 119.998 Hz, MediaSync on

Secondary PA278QGV:
  native HDMI, 59.951 Hz, MediaSync off

Global G-SYNC:
  on

Godot profile:
  VRR disabled, G-SYNC Fixed Refresh
```

I recommend preserving this configuration for normal work. No further test is needed to validate the workaround. If you later want to isolate the underlying cause more precisely, the least disruptive next test is raising the HDMI monitor to 120 Hz if Windows offers it. Otherwise, the closest comparison is primary 120 Hz plus secondary 60 Hz over the two TB5/DisplayPort routes—but that risks restoring the sticky failure and requiring the global G-SYNC recovery toggle.

---

## User

since the last prompt, i've restarted my laptop and tried to get back to normal usage.

however, it seems there is another wrinkle: if i disconnect the laptop's built-in display via "windows 11 > settings > system > display", i get the poor gsync behavior again. when i reconnect the laptop's built-in display, things work smoothly again.

this is not ideal because my ideal setup is just the two external monitors while the laptop is tucked away into the back of my desk and the laptop display is not really seeable. i explicitly want the laptop display to be disconnected so i don't accidentally "lose" windows over there.

i also tried mediasync on and off on the hdmi connected pa monitor, and that didn't seem to affect anything.

so currently, this is the setup:

laptop internal display:
  connected
  240 hz
Primary PA278QGV:
  TB5/USB-C DisplayPort
  120 hz
  MediaSync on

Secondary PA278QGV:
  native HDMI
  120 hz
  MediaSync on

Global G-SYNC:
  on

Godot profile:
  VRR disabled
  G-SYNC Fixed Refresh

let's continue the investigation

## Assistant

The healthy post-reboot baseline is captured:

```text
internal target 8449: active, about 240 Hz, displayInVrrMode=1
TB5/DisplayPort PA target 8450: active, 119.998 Hz, displayInVrrMode=1
HDMI PA target 8448: active, 119.998 Hz, displayInVrrMode=1
```

All three paths are directly owned by the RTX GPU, and the Godot profile remains VRR disabled plus Fixed Refresh.

This establishes two important points:

- HDMI at 120 Hz is smooth while the internal panel is active, so the earlier 60 Hz HDMI mode was not necessary.
- Within the TB5/DisplayPort-plus-HDMI topology, internal eDP active versus Windows-disconnected is now the apparent discriminator. HDMI MediaSync on/off is not.

We still need to separate whether disconnecting the internal panel immediately breaks the neutral G-SYNC path or merely makes the next Fixed Refresh editor transition unsafe.

Please establish the capture arm:

1. Keep both external PAs at 120 Hz and leave all G-SYNC and OSD settings unchanged.
2. In Windows Settings, select the internal laptop display and choose `Disconnect this display`.
3. Do not open Godot, Unity, `VsyncStutterTest.exe`, or another 3D application afterward.
4. Tell me when ready so I can capture the two-external-only topology before the neutral control.

After that capture, we will run `VsyncStutterTest.exe` before opening any editor. That will tell us whether the Windows topology change itself is sufficient or whether the failure still requires the Fixed Refresh transition.

---

## User

i did this:
Keep both external PAs at 120 Hz and leave G-SYNC and OSD settings unchanged.
In Windows Display Settings, select the internal laptop display and choose Disconnect this display.
Don’t open Godot, Unity, VsyncStutterTest.exe, or another 3D application afterward.
Tell me when ready so I can capture the two-external-only topology.

ready

## Assistant

The two-external-only capture is complete:

```text
Windows active paths:
  target 8450: TB5/DisplayPort PA, 119.998 Hz
  target 8448: HDMI PA, 119.998 Hz

Internal target 8449:
  physically connected but inactive
  zero active DisplayPort lanes

DRS:
  timestamps and hashes unchanged from the healthy three-display baseline
```

NVIDIA still reports HDMI target 8448 in VRR display mode. The public VRR-info query for primary DP target 8450 now returns generic `NVAPI_ERROR`, while its Adaptive-Sync and link queries remain healthy. We observed this same query error in an earlier topology where the functional G-SYNC control worked, so it does not establish failure by itself.

Now run the decisive pre-editor control:

1. Keep the internal panel disconnected and change no G-SYNC, refresh-rate, topology, or OSD setting.
2. Do not open Godot or Unity.
3. Run `C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe` on the primary TB5/DisplayPort PA.
4. Observe the top-right G-SYNC indicator and animation smoothness.
5. Close it and report the result.

Interpretation:

- If the indicator is missing or animation is choppy, Windows-disconnecting internal eDP is sufficient to break the neutral G-SYNC path immediately.
- If the control remains smooth with the indicator, the two-external-only topology starts healthy and the later Fixed Refresh editor transition is still required to trigger the failure.

---

## User

i did this:
Keep the internal panel disconnected and change no G-SYNC, refresh-rate, topology, or OSD setting.
Don’t open Godot or Unity.
Run C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe on the primary TB5/DisplayPort PA.
Observe the G-SYNC indicator and animation smoothness.
Close it and report the result.

vsyncstuttertest runs smoothly and shows the gsync indicator

## Assistant

This proves internal-panel disconnection alone is not sufficient to break G-SYNC. The two-external-only DP+HDMI topology starts healthy:

- `VsyncStutterTest.exe` displays the G-SYNC indicator and runs smoothly;
- the two active display paths remain unchanged;
- the NVIDIA profile database remains byte-for-byte unchanged; and
- the primary target's generic public VRR-query error persists despite functional G-SYNC, confirming it is not a reliable failure indicator in this topology.

The reported poor state must therefore require a later event. Now test the Fixed Refresh transition without Godot's profile writer:

1. Keep the internal panel disconnected and change no G-SYNC, topology, refresh-rate, or OSD setting.
2. Do not open Godot.
3. Open the same Unity editor/project on the primary TB5/DisplayPort PA.
4. Observe whether either monitor blinks and whether Unity behaves smoothly without the G-SYNC indicator.
5. Close Unity.
6. Immediately run `C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe` on the same PA.
7. Observe its G-SYNC indicator and animation smoothness.
8. Close it and report both results.

Interpretation:

- If Unity is followed by missing/choppy G-SYNC, the two-external-only topology is healthy initially but cannot restore the primary after a Fixed Refresh profile transition.
- If the control remains healthy, the event that produced your earlier poor state was something other than internal disconnection plus Fixed Refresh activation.

---

## User

i opened the unity project and observed significant and disruptive monitor blinking on open, during usage, and on close. however, no gsync indicator was displayed and viewport usage was smooth other than one 2-second blink in the middle of using it.

i ran VsyncStutterTest and it ran smoothly and showed the gsync indicator

again, i opened the unity project and observed significant and disruptive monitor blinking on open, during usage, and on close. however, no gsync indicator was displayed and viewport usage was smooth other than one 2-second blink in the middle of using it.

again, i ran VsyncStutterTest and it ran smoothly and showed the gsync indicator

## Assistant

This precisely reclassifies the internal-panel-off problem:

- The two-external-only topology starts with working G-SYNC.
- Unity Fixed Refresh causes severe repeated display blinking on open, during use, and on close.
- Unity itself remains smooth without the G-SYNC indicator, as intended.
- `VsyncStutterTest.exe` restores smooth G-SYNC correctly afterward—twice.
- The final display topology, NVIDIA target state, and DRS hashes are unchanged, and Windows logged no relevant display-driver reset.

So internal eDP disconnection does not cause immediate or sticky G-SYNC failure in the DP+HDMI topology. It destabilizes the Fixed Refresh display-mode transition. Keeping eDP active suppresses that blinking.

The next test separates external scanout load from the presence of the eDP path:

1. Keep the internal laptop display disconnected.
2. Keep the primary TB5/DisplayPort PA at 120 Hz with MediaSync on.
3. In Windows Advanced Display, change only the HDMI PA from 120 Hz to approximately 60 Hz. Leave its OSD setting unchanged.
4. Do not open Godot, Unity, `VsyncStutterTest.exe`, or another 3D application afterward.
5. Tell me when ready so I can capture the internal-off 120-Hz/60-Hz baseline.

If the later Unity transition stops blinking, the leading cause is external scanout clock/resource allocation and this provides a two-external-only workaround. If it still blinks, the active internal eDP path itself is the stabilizer; the next practical test would be duplicating the hidden internal panel with one external display so Windows exposes only two desktop spaces while eDP remains active.

---

## User

i did this:
Keep the internal laptop display disconnected.
Keep the primary TB5/DisplayPort PA at 120 Hz with MediaSync on.
Change only the HDMI PA from 120 Hz to approximately 60 Hz in Windows Advanced Display. Leave its OSD unchanged.
Don’t open Godot, Unity, VsyncStutterTest.exe, or another 3D application afterward.
Tell me when ready so I can capture the internal-off 120/60 Hz baseline.

ready.

## Assistant

The internal-off 120/60 Hz baseline is captured:

```text
target 8450 primary PA:
  TB5/DisplayPort, 119.998 Hz
  VRR query succeeds
  displayInVrrMode=1

target 8448 secondary PA:
  HDMI, 59.951 Hz
  VRR query succeeds
  displayInVrrMode=1

internal target 8449:
  physically connected but inactive

DRS:
  timestamps and hashes unchanged
```

This produced an immediate, meaningful change: at internal-off 120/120 Hz, the primary target's public VRR query returned generic error; after lowering only HDMI to 60 Hz, the same query succeeds and reports the primary in VRR display mode. That directly implicates external mode/clock/resource allocation, although the visible transition still needs testing.

First establish the neutral control:

1. Keep internal eDP disconnected, primary DP at 120 Hz, and HDMI at approximately 60 Hz.
2. Change no G-SYNC or OSD setting and do not open Godot or Unity.
3. Run `C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe` on the primary TB5/DisplayPort PA.
4. Observe its G-SYNC indicator and animation smoothness.
5. Close it and report the result.

If healthy, we will immediately repeat the Unity Fixed Refresh transition. A no-blink result there would provide the two-external-only workaround you want and strongly identify high external scanout load as the cause.

---

## User

i did this:
Keep internal eDP disconnected, primary DP at 120 Hz, and HDMI at approximately 60 Hz.
Change no G-SYNC or OSD setting, and don’t open Godot or Unity.
Run C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe on the primary TB5/DisplayPort PA.
Observe its G-SYNC indicator and smoothness.
Close it and report the result.

VsyncStutterTest ran smoothly and showed the gsync indicator

## Assistant

The internal-off 120/60 Hz arm has a clean functional baseline:

- `VsyncStutterTest.exe` displays the G-SYNC indicator and runs smoothly;
- both external targets still return successful VRR queries with `displayInVrrMode=1`; and
- topology and DRS hashes remain unchanged.

Now run the direct Unity comparison:

1. Keep internal eDP disconnected, primary TB5/DisplayPort at 120 Hz, and HDMI at approximately 60 Hz.
2. Change no G-SYNC or OSD setting and do not open Godot.
3. Open the same Unity editor/project on the primary TB5/DisplayPort PA.
4. Observe carefully whether either monitor blinks on open, during editor use, or on close. Confirm whether Unity remains smooth without the G-SYNC indicator.
5. Close Unity.
6. Immediately run `C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe` on the same PA.
7. Observe its G-SYNC indicator and smoothness.
8. Close it and report both results.

This is the decisive load test: no disruptive blinking would validate a two-external-only 120/60 workaround and strongly implicate high aggregate external scanout load. Continued blinking would mean the active internal eDP path itself provides the missing stabilization.

---

## User

i opened the unity project and observed a monitor blink on open. however, no gsync indicator was displayed and viewport usage was smooth. then i closed it.

i ran VsyncStutterTest and it ran choppy and didn't show the gsync indicator

again, i opened the unity project, observed zero monitor blinks, no gsync indicator, smooth usage, and closed it

again, i ran VsyncStutterTest and it ran choppy and didn't show the gsync indicator

## Assistant

This is the damaging sticky failure, captured directly:

```text
Before Unity:
  primary target 8450 displayInVrrMode=1
  VsyncStutterTest smooth with G-SYNC indicator

After Unity:
  primary target 8450 displayInVrrMode=0
  VsyncStutterTest choppy without indicator

Unchanged:
  HDMI target 8448 remains displayInVrrMode=1
  display topology
  DRS timestamps and hashes
```

Lowering HDMI to 60 Hz is therefore not a workaround. It changes the failure mode:

- Internal off, DP 120 + HDMI 120: severe repeated transient blinking, but later G-SYNC recovers.
- Internal off, DP 120 + HDMI 60: one initial blink, then primary G-SYNC becomes stuck off.
- Internal on, DP 120 + HDMI 120: clean transitions and correct later G-SYNC.

This disproves a simple “too much display bandwidth” threshold. The active internal eDP path is the only tested condition that prevents both forms of failure in the mixed DP+HDMI topology. External refresh allocation changes how the driver fails, not whether the two-external-only transition path is fragile.

Now test topology-only recovery before using the global G-SYNC toggle:

1. Leave both external monitors connected. Keep the primary DP PA at 120 Hz and HDMI PA at approximately 60 Hz.
2. Do not toggle G-SYNC and do not open Godot, Unity, or `VsyncStutterTest.exe`.
3. In Windows Display Settings, reconnect/extend the internal laptop display.
4. Tell me when ready so I can capture whether target 8450 returns from `displayInVrrMode=0` to `1` solely from restoring the eDP path.

If it recovers, reconnecting eDP is a topology-level recovery mechanism and the next practical experiment is Windows duplicate/clone mode: keep eDP active but mirror it with one external display so Windows exposes only two desktop spaces. That would address hidden windows, though the internal panel would technically remain active.

---

## User

i did this:
Leave both external monitors connected, with primary DP at 120 Hz and HDMI at approximately 60 Hz.
Don’t toggle G-SYNC or open Godot, Unity, or VsyncStutterTest.exe.
In Windows Display Settings, reconnect/extend the internal laptop display.
Tell me when ready so I can check whether target 8450 returns from displayInVrrMode=0 to 1 solely by restoring eDP.

ready.

## Assistant

The primary target recovered at the API level without a global G-SYNC toggle:

```text
target 8450 primary TB5/DisplayPort:
  before reconnect: displayInVrrMode=0
  after reconnect:  displayInVrrMode=1

target 8448 HDMI: displayInVrrMode=1
target 8449 internal eDP: active, displayInVrrMode=1
DRS timestamps and hashes: unchanged
```

One confound matters: when Windows extended the internal panel, it automatically changed the HDMI PA from approximately 60 Hz back to 119.998 Hz. Therefore the result proves that display-topology/mode reconstruction recovers the stuck primary without toggling G-SYNC, but not that activating eDP alone was solely responsible.

Now verify functional recovery:

1. Keep the current three-display state unchanged: internal active, primary DP at 120 Hz, HDMI at 120 Hz.
2. Do not open Godot or Unity and do not toggle G-SYNC.
3. Run `C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe` on the primary TB5/DisplayPort PA.
4. Observe its G-SYNC indicator and smoothness.
5. Close it and report the result.

If it works, the next practical test is Windows duplicate mode: mirror the hidden internal panel with the HDMI PA. That keeps the eDP path active while exposing only two Windows desktop spaces, directly addressing the “lost windows on the hidden laptop screen” problem—although the physical internal panel would technically remain active.

---

## User

i did this:
Keep the current three-display state unchanged: internal active and both external PAs at 120 Hz.
Don’t open Godot or Unity, and don’t toggle G-SYNC.
Run C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe on the primary PA.
Observe its G-SYNC indicator and smoothness.
Close it and report the result.

VsyncStutterTest ran smoothly and displayed the gsync indicator

## Assistant

Functional recovery is confirmed, not just inferred from the API state:

- `VsyncStutterTest.exe` ran smoothly with the G-SYNC indicator;
- primary target 8450 remains at `displayInVrrMode=1`;
- all three active display targets report VRR mode 1; and
- DRS timestamps and hashes are unchanged.

So Windows display-topology/mode reconstruction restored working G-SYNC without toggling the global setting. The caveat remains that reconnecting eDP also returned HDMI from about 60 Hz to 120 Hz, so this does not isolate eDP activation from the wider mode reconstruction.

Now test the practical clone workaround:

1. Keep global G-SYNC and both monitors’ OSD settings unchanged.
2. Open Windows Settings → System → Display and use **Identify** so you can distinguish the internal panel, primary TB5/DisplayPort PA, and secondary HDMI PA.
3. Duplicate the **internal laptop display with the secondary HDMI PA**. Do not duplicate the primary TB5/DisplayPort PA.
4. Leave the primary TB5/DisplayPort PA as the main, separately extended display.
5. Let Windows choose the clone pair’s common mode for now; do not manually correct refresh rates afterward.
6. Do not open Godot, Unity, `VsyncStutterTest.exe`, or another 3D application.
7. Tell me when ready so I can capture the clone topology before the functional tests. If Windows does not offer that exact duplicate pair or makes an obviously unusable topology, stop and report what it offers instead.

This would keep eDP active while exposing only two logical Windows desktop spaces, addressing the hidden-window problem. It does not literally disconnect or power down the laptop panel.

---

## User

i'm unsure if this duplication topology is available. this is what i see.

![Windows Display Settings offering duplicate pairs 1 and 2 or 1 and 3](<Attachments/2026-08-28-gsync-investigation/windows-display-duplicate-options-1-2-1-3.png>)

## Assistant

Yes—the exact duplication topology is available.

The unchanged left-to-right arrangement in your screenshot maps to the preceding captured source positions:

- display 2 = primary TB5/DisplayPort PA;
- display 3 = secondary HDMI PA; and
- display 1 = internal laptop panel.

Choose **Duplicate desktop on 1 and 3**. Do not choose 1 and 2, because that would clone the internal panel with the primary PA.

Then:

1. Click **Apply** and accept **Keep changes** if Windows asks.
2. Do not adjust refresh rates, G-SYNC, or either monitor's OSD afterward.
3. Do not open Godot, Unity, `VsyncStutterTest.exe`, or another 3D application.
4. Tell me when it is ready so I can capture how Windows and NVIDIA represent the clone paths before testing it.

Windows may renumber or visually combine displays after applying the clone; that is expected. The intended end state is display 2 as the separate primary desktop and displays 1+3 sharing the other desktop.

---

## User

yes, i was mistaken. you are correct.

i did this:
Choose Duplicate desktop on 1 and 3.
Then click Apply and Keep changes. Don’t change refresh rates, G-SYNC, or OSD settings, and don’t open any 3D applications afterward. Tell me when ready so I can capture the resulting clone topology.

ready.

## Assistant

The captured clone topology is exactly what we intended:

- Windows has three active physical display paths but only two desktop sources.
- Internal eDP and the HDMI PA share source ID 2 and `\\.\DISPLAY3`, proving they are cloned.
- The primary TB5/DisplayPort PA remains separate on source ID 0 and `\\.\DISPLAY1`.
- All three NVIDIA targets remain active, OS-visible, VRR-capable, and in VRR mode 1.
- DRS timestamps and hashes are unchanged.

One practical detail: Windows chose a 2560x1600 shared source, while the HDMI PA receives a 2560x1440 signal at 120 Hz. Check whether that copy looks acceptably scaled or letterboxed.

Now establish the neutral control:

1. Keep the clone topology, G-SYNC, refresh rates, and OSD settings unchanged.
2. Do not open Godot or Unity.
3. Run `C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe` on the separate primary TB5/DisplayPort PA.
4. Observe the G-SYNC indicator and animation smoothness.
5. Close it and report the result. Also report whether the cloned HDMI desktop looks visually usable or is distorted, cropped, or letterboxed.

If the neutral control succeeds, the decisive next step is the Unity Fixed Refresh transition in this clone topology.

---

## User

the hdmi monitor was letterboxed, so i changed the 1|3 resolution to 2560*1440. i haven't done anything else. record state before we proceed.

## Assistant

The revised pre-application state is recorded and healthy:

- The cloned 1|3 source is now 2560x1440.
- The HDMI PA receives a matching native 2560x1440 signal at 120 Hz.
- The hidden internal panel remains physically active with its 2560x1600 target signal, fed by the shared 1440p source.
- The primary TB5/DisplayPort PA remains a separate 2560x1440/120-Hz source.
- All three NVIDIA targets remain active, OS-visible, VRR-capable, and in VRR mode 1.
- DRS timestamps and SHA-256 hashes remain unchanged.

This 2560x1440 clone state supersedes the earlier 2560x1600 clone source as our authoritative baseline.

Now run the neutral control:

1. Keep the clone topology, G-SYNC, refresh rates, resolution, and OSD settings unchanged.
2. Do not open Godot or Unity.
3. Run `C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe` on the separate primary TB5/DisplayPort PA.
4. Observe the G-SYNC indicator and animation smoothness.
5. Close it and report the result.

If it succeeds, the next test is `Unity Fixed Refresh -> close Unity -> VsyncStutterTest.exe` in this exact clone topology.

---

## User

i ran VsyncStutterTest and i ran smoothly and displayed the gsync indicator

## Assistant

The 2560x1440 clone topology has passed its neutral control:

- `VsyncStutterTest.exe` ran smoothly with the G-SYNC indicator;
- the two-source/three-target clone topology is unchanged;
- all three targets remain in VRR mode 1; and
- DRS timestamps and hashes remain unchanged.

Now perform the decisive Unity transition:

1. Keep the clone topology, resolution, refresh rates, G-SYNC, and OSD settings unchanged.
2. Do not open Godot.
3. Open the same Unity editor/project on the separate primary TB5/DisplayPort PA. If it opens elsewhere, note where before moving it.
4. Observe whether either monitor blinks on open, during use, or on close. Confirm whether Unity remains smooth without the G-SYNC indicator.
5. Close Unity.
6. Immediately run `C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe` on the primary TB5/DisplayPort PA.
7. Observe its G-SYNC indicator and animation smoothness, then close it.
8. Report both Unity and `VsyncStutterTest.exe` results.

This directly tests whether keeping eDP active as a cloned target is sufficient to stabilize the Fixed Refresh transition while eliminating the hidden third desktop.

---

## User

i did this:
Keep all display and G-SYNC settings unchanged.
Open the same Unity project on the primary TB5/DisplayPort PA.
Observe blinking on open, during use, and on close. Confirm Unity is smooth without G-SYNC.
Close Unity.
Immediately run VsyncStutterTest.exe on the primary PA.
Report its indicator and smoothness, along with the Unity observations.

the unity project opened, ran smoothly, and closed with zero blinks. it didn't show the gsync indicator
i ran VsyncStutterTest and i ran smoothly and displayed the gsync indicator

## Assistant

This validates the practical workaround at the NVIDIA transition level:

- Unity entered Fixed Refresh, ran smoothly without the G-SYNC indicator, and exited with zero blinks.
- The immediate neutral control regained smooth G-SYNC and the indicator.
- The post-transition capture retains the same two-source/three-target clone topology.
- Targets 8448, 8449, and 8450 all remain in VRR mode 1.
- DRS timestamps and hashes remain unchanged.

The important conclusion is that eDP does not need its own extended desktop to stabilize the transition. It can share a cloned desktop with HDMI as long as it remains an active target. This gives you two usable external desktop spaces without a hidden third desktop.

Now validate the exact original Godot workflow once:

1. Keep the clone topology, resolution, refresh rates, G-SYNC, and OSD settings unchanged.
2. Launch Godot 4.6.3 normally through its project manager—not with the direct D3D12 bypass command.
3. Open `C:\Users\k\Repository\Godot\VsyncStutterTest\Godot`.
4. Record where the project editor opens and whether either monitor blinks while the project manager starts, the project opens, during use, or on close.
5. Confirm whether Godot remains smooth without the G-SYNC indicator, then close Godot completely.
6. Immediately run `C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe` on the primary TB5/DisplayPort PA.
7. Observe its G-SYNC indicator and animation smoothness, close it, and report both results.

This final test includes Godot's native-OpenGL project manager and its DRS save, so it validates the complete workflow that originally triggered the failure.

---

## User

i launched godot 4.6.3 and then opened the project. the godot editor with the project launched on the hdmi+laptop display instead of the tb5 pa primary display. it did a 2 second monitor blink. i moved the godot editor window to the primary tb5 pa monitor. i then closed godot

then i launched vsyncstuttertest and it ran smoothly and displayed the gsync indicator, and i closed it

for a second time, i launched godot 4.6.3 and then opened the project. the godot editor with the project launched on the primary monitor. it didn't show the gsync indicator and usage was smooth.

then for a second time i launched vsyncstuttertest and it ran smoothly and displayed the gsync indicator, and i closed it

## Assistant

The exact Godot workflow is safe from the original sticky failure in clone mode:

- both immediate post-Godot controls regained smooth G-SYNC and the indicator;
- the clone topology is unchanged;
- all three targets remain in VRR mode 1; and
- Godot performed its expected DRS saves at 00:59:06 and 00:59:30 without poisoning later G-SYNC.

The remaining issue is the single two-second blink. It occurred when the first editor opened on the cloned HDMI+eDP desktop; the second launch on primary DP was clean. We need one controlled placement repetition before deciding whether clone placement causes the blank or whether it was only a first/cold transition.

Do this:

1. Keep the clone topology and every display, G-SYNC, refresh-rate, resolution, and OSD setting unchanged.
2. Launch Godot normally and open the project. Record whether this setup launch blinks.
3. Move the project editor to the cloned HDMI+laptop desktop and close Godot there, allowing it to save that placement.
4. Launch Godot normally and open the same project again.
5. Report where the editor opens and whether either monitor blinks during project-manager startup, project opening, use, or close. Confirm Godot remains smooth without the G-SYNC indicator.
6. Close Godot.
7. Immediately run `C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe` on the primary TB5/DisplayPort PA and report its indicator and smoothness.

If the deliberately persisted clone-desktop launch is clean, the initial blink is another one-time cold/topology transition rather than a recurring limitation of this workaround.

---

## User

i did several of the primary to hdmi+laptop display and vice versa transitions. i could not repro any blinks, and godot editor ran smoothly without the gsync indicator in all cases.

i launched vsyncstuttertest and it ran smoothly and displayed the gsync indicator

## Assistant

This completes the current-session validation. The 2560x1440 eDP+HDMI clone now meets the full practical workflow:

- Windows exposes only two usable desktop spaces.
- Godot and Unity run smoothly under Fixed Refresh without the G-SYNC indicator.
- Repeated primary-to-clone and clone-to-primary Godot placements produce no monitor blink.
- `VsyncStutterTest.exe` automatically regains smooth G-SYNC afterward.
- The final capture retains the same two-source/three-target clone topology and VRR mode 1 on every target.
- Godot continues making its expected DRS saves, but the selector and one-profile/two-application association remain stable and later G-SYNC is not poisoned.

The earlier isolated two-second blink is therefore not caused by editor placement. It is best classified as a non-reproduced cold/topology transition immediately after creating the clone state.

The remaining durability test is reboot persistence:

1. Leave the current clone topology configured.
2. Save other work and restart Windows normally.
3. After signing in, do not change display, resolution, refresh-rate, G-SYNC, or OSD settings.
4. Do not open Godot, Unity, `VsyncStutterTest.exe`, or another 3D application.
5. Tell me when ready so I can capture whether Windows restored the same clone paths and healthy pre-application VRR state.

If the topology survives, we will run one neutral control and then one first-after-reboot Godot transition. That will tell us whether the workaround is durable and whether the isolated cold blink recurs after a true reboot.

---

## User

i will test reboot persistence later.

take into account this entire investigation, and everything in this repo, and explain what may be the root cause(s) of all the issues we've encountered.

## Assistant

The best-supported diagnosis is **a topology-sensitive NVIDIA driver bug in the transition between per-application Fixed Refresh and live G-SYNC/VRR**. Godot exposes and aggravates it, but Godot is not the primary cause.

| Layer | Best explanation | Confidence |
| --- | --- | --- |
| Core failure | NVIDIA fails to re-arm VRR on the primary DP target after a Fixed Refresh app exits | Very high |
| Trigger | Entering/leaving an already stored Fixed Refresh profile | Very high, but only in susceptible topologies |
| Topology selector | Active-output routing/timings change NVIDIA display-head/resource and Windows presentation-path allocation | High; exact private resource unknown |
| Godot behavior | Persistent, unconditional native-OpenGL DRS profile setup/save; inadequate fullscreen-only workaround | Confirmed contributor, not core cause |
| NVIDIA App behavior | Its app catalog does not faithfully represent/adopt every underlying DRS profile | Confirmed separate defect |

The strongest proof is the internal-off DP-120/HDMI-60 Unity test. Before Unity, unprofiled `VsyncStutterTest.exe` worked. After Unity's existing Fixed Refresh profile transition, the primary target was still `VRR possible=1`, but its successfully queried `displayInVrrMode` changed from `1` to `0`; the HDMI target stayed at `1`; DRS and global G-SYNC remained unchanged; and both AoE4 and the unprofiled control lost G-SYNC. Cycling global G-SYNC or reconstructing the display topology restored target mode `1` and functional G-SYNC. So the saved global setting was never turned off—the NVIDIA driver's live per-target state became stuck.

Unity is the decisive causal control. It produced the same class of blink/sticky failure without changing DRS. Conversely, Godot repeatedly saved DRS in the stable clone topology without causing a recurring blink or later G-SYNC loss. Therefore Godot's write is neither necessary nor sufficient. The demonstrated proximal trigger is the runtime Fixed Refresh transition in a susceptible display allocation.

The topology A/Bs show why this looked inconsistent:

- one external PA plus eDP is clean;
- two active TB5/USB-C DP PAs are bad with eDP on or off;
- leaving the second PA cabled but Windows-disabled is clean;
- primary DP plus native HDMI is clean when eDP is active;
- with eDP inactive, DP-120/HDMI-120 caused severe transient blanks but recovered, while DP-120/HDMI-60 caused sticky VRR loss; and
- keeping eDP active but cloning it with HDMI is clean in repeated Unity and Godot tests.

Thus two active external heads were necessary in every observed bad arm but were not sufficient. The clone result is especially revealing: eDP does not need its own desktop source; it only needs to remain an active physical target/path. That strongly implicates route-dependent display-engine/head/timing allocation or Windows VidPN reconstruction. Reducing HDMI from 120 to about 60 Hz made the result worse rather than fixing it, so this is not a simple bandwidth or high-refresh overload.

Godot has two real, separate problems here. In the actual 4.6.3 source, native OpenGL initialization calls `_nvapi_setup_profile()`, uses the executable basename, creates/associates a persistent DRS profile, sets threaded optimization and G-SYNC mode to fullscreen-only, then calls `NvAPI_DRS_SaveSettings()` without checking whether those values already match. The project manager defaults to native OpenGL, so it performs this work before spawning a D3D12 project editor. It does **not** write the Fixed Refresh override used in our later tests. Godot's upstream change explicitly says windowed G-SYNC is known to produce unstable editor refresh rates. That matches the direct-D3D12/no-profile result: no DRS mutation or sticky loss, but G-SYNC stayed active in the editor and pointer motion was choppy. See the local [Godot NVIDIA setup code](C:/Users/k/Repository/External/Godot_4-6-3/platform/windows/gl_manager_windows_native.cpp:109), its [unconditional final save](C:/Users/k/Repository/External/Godot_4-6-3/platform/windows/gl_manager_windows_native.cpp:269), the [project-manager OpenGL default](C:/Users/k/Repository/External/Godot_4-6-3/main/main.cpp:2453), and the [originating Godot commit](https://github.com/godotengine/godot/commit/b8edc64379b3c4b5f2e7334468be65fd44a4980c).

The NVIDIA App/profile confusion is another independent defect. DRS can contain an exact-path profile and a basename `Godot Engine` profile simultaneously. NVIDIA App starts from its separate application catalog and exposes one chosen profile per catalog row instead of enumerating that full relationship; its create-before-adopt behavior caused the observed `EXECUTABLE_ALREADY_IN_USE` and `PROFILE_NAME_IN_USE` collisions. Selecting a stale row could recreate an empty profile. That explains why NVApp, NVCP, and NPI appeared to disagree; it cannot explain an unprofiled app losing VRR while DRS stays unchanged.

The two-to-three-second blackouts are most likely disruptive modesets/display-head reprogramming, possibly including link retraining, while the driver changes presentation policy. That exact mechanism is not directly observable. They were not TDRs or GPU crashes: there were no matching display-driver reset, WER, or LiveKernelReport events. Blinking is also not a reliable predictor—some severe repeated blinks recovered, while one blink preceded a sticky failure.

Windows is probably part of the boundary conditions, not necessarily a separate root bug. Windowed VRR depends on swap-chain presentation, and Windows may move a flip-model window among composed, DirectFlip/Independent Flip, and MPO paths; Microsoft documents those hardware-dependent transitions in its [VRR](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/variable-refresh-rate-displays) and [flip-model](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/for-best-performance--use-dxgi-flip-model) guidance. We cannot currently distinguish a bad NVIDIA head/PLL assignment from a VidPN-commit ordering problem or an NVIDIA/DWM presentation interaction. The persistent per-target state and NVIDIA-controlled recovery make “NVIDIA driver defect at the WDDM/display boundary” the defensible label.

NVIDIA also publishes a support caveat that multiple monitors may be connected but [no more than one should have G-SYNC enabled](https://nvidia.custhelp.com/app/answers/detail/a_id/4766/), while its UI applies display-specific settings to every connected display of the selected model in the [G-SYNC setup guide](https://www.nvidia.com/content/Control-Panel-Help/vLatest/en-us/mergedProjects/Display/To_use_variable_refresh_rates.htm). That may explain general fragility with two identical PAs, but it is not the whole root cause: the issue also reproduced when the secondary PA had MediaSync off and NVAPI reported it non-VRR.

So the concise causal chain is:

> editor/windowed-VRR incompatibility -> Fixed Refresh profile transition -> topology-sensitive NVIDIA display-state restoration bug -> primary target remains outside VRR -> unrelated later apps are choppy until the display state is rebuilt.

The current 2560x1440 eDP+HDMI clone avoids the faulty allocation in repeated in-session tests. It is a workaround around the bug, not a repair. Reboot persistence and the first cold transition after reboot remain the only current durability unknowns.

I recorded the full ranked analysis in [root-cause-synthesis.md](C:/Users/k/Repository/NvidiaTroubleshooting/2026-08-28%20godot%20aoe4%20gsync%20investigation/root-cause-synthesis.md) and committed it as `a5c6fae`.

---

## User

i rebooted and things continue to work smoothly, except that godot 4.6.3 editor needs a one-time monitor blink. after that blink and before another reboot, successive open/close of godot editor work smoothly without blinks.

i also tried the two external monitor + laptop display cloned w/ one external monitor config but with both external monitor connected to tb5 instead of one of them connected to hdmi. that resurfaced the gsync issues, so i switched one external monitor to hdmi and it returned to smooth usage.

## Assistant

Recorded. This changes the workaround from “successful in one session” to **successful across the tested reboot**, and it makes physical connector routing the strongest operational discriminator.

The live capture confirms Windows restored the intended working topology:

- three active physical NVIDIA targets but only two Windows desktop sources;
- internal eDP cloned with the native-HDMI PA at a 2560x1440 source; and
- the other PA separate over one TB5/DisplayPort output at 119.998 Hz.

The Godot profile structure also remains stable. The later Godot DRS writes occurred at 01:29 and 01:30, with the same one `Godot Engine` profile and its 4.4.1/4.6.3 associations.

The implications are now:

1. **The mixed-route clone is reboot-durable for preventing sticky G-SYNC loss.** We have validated one reboot. The first Godot launch after that reboot blinked once; later launches in that boot were clean and later G-SYNC remained healthy.
2. **The blink and the sticky failure are distinct outcomes.** A disruptive cold transition can blank the monitors without leaving the primary target poisoned afterward.
3. **Clone mode and active eDP are not sufficient.** The same general clone arrangement still fails when both external heads use the two TB5/USB-C DisplayPort paths.
4. **Native HDMI is the stabilizing route in the tested family.** Moving one PA to HDMI does more than reconstruct the topology once: the resulting allocation remains stable through repeated application transitions and a reboot.
5. **The root-cause model is now more specifically route-sensitive.** The likely defect is in NVIDIA's handling of two simultaneous external DP-over-USB-C paths during Fixed Refresh/VRR transitions. Native HDMI moves one target onto a different transmitter/head/timing allocation. Public APIs cannot tell us whether the private failing object is a DP transmitter, PLL, Type-C mux state, or per-head VRR bookkeeping, so “HDMI protocol itself fixes it” would be too strong.

One new diagnostic caveat appeared. Windows Error Reporting surfaced black-screen live dumps created around 01:26 and 01:28:

- `0x1A8 VIDEO_DXGKRNL_BLACK_SCREEN_LIVEDUMP`; and
- `0x1B8 VIDEO_MINIPORT_BLACK_SCREEN_LIVEDUMP`.

Microsoft documents these as live diagnostics, not fatal bugchecks or conventional TDRs. Their parameter 1 is `1`, which Microsoft maps to the black-screen hotkey source. The latest dump groups also precede the Godot DRS writes at 01:29/01:30, so they do not implicate those later Godot saves and may instead align with the dual-TB5/topology-switch tests. See Microsoft's [`0x1A8`](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/bug-check-0x1a8--video-dxgkrnl-black-screen-livedump) and [`0x1B8`](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/bug-check-0x1b8--video-miniport-black-screen-livedump) documentation.

I need one clarification: **did you press `Win+Ctrl+Shift+B`, or use an ASUS/NVIDIA shortcut that might invoke it, at any point during these tests?** If not, the reported hotkey source itself becomes another anomaly to investigate.

If you later want to isolate the remaining cold blink, the clean next A/B is another reboot followed by **Unity first, before Godot**:

- Unity also blinks once -> general first Fixed Refresh transition after boot;
- Unity is clean, then Godot blinks -> Godot native-OpenGL/profile-save initialization is involved; or
- neither blinks -> the cold event depends on another topology/startup condition.

I recorded the complete capture and revised interpretation in [post-reboot-clone-route-validation.md](C:/Users/k/Repository/NvidiaTroubleshooting/2026-08-28%20godot%20aoe4%20gsync%20investigation/post-reboot-clone-route-validation.md) and updated the canonical synthesis. The evidence commit is `ce4d28c`.

---

## User

it seems like i've gotten into a bad state again somehow. provide a comprehensive guide for the "last known good state".

## Assistant

I created the comprehensive [last-known-good recovery guide](C:/Users/k/Repository/NvidiaTroubleshooting/2026-08-28%20godot%20aoe4%20gsync%20investigation/last-known-good-state-guide.md).

The current state is measurably different from the proven one in two important ways:

- the internal eDP target is now running at **60 Hz**, not approximately **240 Hz**; and
- the matching `Godot Engine` profile has lost its explicit **Fixed Refresh** override (`0x10A879CF = 4`) and is inheriting global G-SYNC `Allow` instead.

The shortest deterministic recovery sequence is:

1. Close all editors, games, and 3D tests.
2. Keep one PA on one TB5/USB-C DisplayPort output and move/keep the other on native HDMI. Never use both TB5/DP outputs for this workaround.
3. Temporarily extend all three displays. Set the DP PA to 2560x1440/120 Hz, HDMI PA to 2560x1440/120 Hz, and internal panel to 2560x1600/approximately 240 Hz.
4. Make the TB5/DP PA the main display. Clone the internal panel with the native-HDMI PA, using a 2560x1440 clone source. The TB5/DP PA remains a separate extended desktop.
5. In NVIDIA Control Panel, set the matching Godot 4.6.3 / `Godot Engine` profile's **Monitor Technology** to **Fixed Refresh**, apply, and close NVCP. Do the same for Unity only if it will be used.
6. Before opening an editor, run `C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe` on the main DP PA. It must be smooth and show the indicator. If it is not, perform the verified global G-SYNC off/apply/on/apply recovery once, then retest.
7. Open Godot normally. It should be smooth without the indicator. Close it and immediately rerun `VsyncStutterTest.exe`; the test must again be smooth with the indicator.

The expected working topology is three active physical targets but only two Windows desktop sources: the DP PA by itself, plus the HDMI PA cloned with active eDP. After a reboot, one two-second blank on the first Godot launch was observed; repeated blanks or post-Godot loss of G-SYNC are failures, not that benign cold-launch quirk.

The guide includes exact target modes, DRS values, a full validation matrix, symptom-based diagnosis, and a compact checklist. It is committed in `0829c5d`.

---

## User

my recovery attempt is failing here (see screenshot)

help me troubleshoot

Attached screenshot saved as `Conversations/Attachments/2026-08-28-gsync-investigation/recovery-guide-acceptance-failure.png`.

## User

post-Godot VsyncStutterTest.exe lost G-SYNC

## Assistant

We have now isolated the failure precisely. **Your recovery settings are correct, but Godot poisoned the NVIDIA driver's live VRR state again.**

The post-failure capture shows:

- Correct topology: one PA on TB5/DP at 120 Hz; the other on native HDMI at 120 Hz; eDP active at 240 Hz and cloned with HDMI using a 1440p source.
- Correct Godot profile: `VRR requested state = disabled` and explicit `G-SYNC = Fixed Refresh`.
- Correct neutral-test policy: `VsyncStutterTest.exe` still has no DRS association.
- Failed live state: primary DP target 8452 is still `VRR possible=1`, but `displayInVrrMode=0`. HDMI and eDP remain at `1`.

That last point exactly explains the missing indicator and choppy motion. This is the original sticky per-target NVIDIA failure, not a Godot-profile or refresh-rate mistake. The query used [NVIDIA's official NVAPI](https://github.com/NVIDIA/nvapi) definitions.

This also corrects one part of the prior guide: the mixed DP+HDMI/eDP-clone topology is the **best known state**, but it is not deterministic. It can produce a stable allocation and did survive one reboot, yet the visible settings do not uniquely determine NVIDIA's private display-head allocation.

One observable difference is worth testing next:

- rebooted working capture: DP target 8452 was on Windows source ID 2; the clone used source ID 0;
- current failed capture: DP target 8452 is on source ID 0; the clone uses source ID 2.

That is a hypothesis, not proof. The next controlled arm is therefore connector/source allocation—not another profile edit.

For now, do only this:

1. Toggle global G-SYNC off and apply.
2. Toggle it back on and apply.
3. Do not open Godot or Unity.
4. Run `VsyncStutterTest.exe` on the main DP PA and confirm that it is smooth with the indicator.
5. Close it and tell me when ready.

I will then capture whether target 8452 returned to `displayInVrrMode=1`. After that, the clean next A/B is moving the DP PA to the other TB5 port while leaving HDMI, eDP clone membership, refresh rates, profiles, and MediaSync unchanged.

The full failed-state evidence is in [recovery-attempt-post-godot-failure.md](C:/Users/k/Repository/NvidiaTroubleshooting/2026-08-28%20godot%20aoe4%20gsync%20investigation/recovery-attempt-post-godot-failure.md), and the corrected guide is in [last-known-good-state-guide.md](C:/Users/k/Repository/NvidiaTroubleshooting/2026-08-28%20godot%20aoe4%20gsync%20investigation/last-known-good-state-guide.md). Evidence commit: `5dac1a8`.

---

## User

i did this:
Toggle global G-SYNC off and apply.
Toggle it back on and apply.
Do not open Godot or Unity.
Run VsyncStutterTest.exe on the main DP PA.
Confirm it is smooth with the indicator, close it, and tell me when ready.

VsyncStutterTest runs smoothly and shows the gsync indicator. ready.

## Assistant

Confirmed at the driver level. Primary DP target 8452 changed from `displayInVrrMode=0` to `1`. The Windows topology, refresh timings, clone relationship, and all nine Godot profile settings remained unchanged.

That proves the global off/on cycle repaired only volatile NVIDIA state; it did not fix the problem by editing a profile or rebuilding the topology.

Now change only the connector allocation:

1. Do not open Godot, Unity, `VsyncStutterTest.exe`, or another 3D application.
2. Do not change G-SYNC, MediaSync, NVIDIA profiles, refresh rates, or Windows display settings.
3. Leave the secondary PA connected to native HDMI.
4. Physically move only the primary PA's USB-C/DisplayPort cable from its current TB5 port to the laptop's other TB5 port.
5. Let the displays settle. Even if Windows changes the layout, main-display choice, clone relationship, or refresh rate, do not correct it yet.
6. Tell me when ready.

I will capture the raw post-hotplug target ID, connector instance, Windows source/head assignment, topology, timings, and VRR state before we normalize anything. That cleanly tests whether the current connector-1/source-0 allocation is the remaining selector.

The recovery baseline is recorded in [recovery-attempt-post-godot-failure.md](C:/Users/k/Repository/NvidiaTroubleshooting/2026-08-28%20godot%20aoe4%20gsync%20investigation/recovery-attempt-post-godot-failure.md), commit `1a3a142`.

---

## User

i did this:
Don’t open any 3D application.
Don’t change G-SYNC, MediaSync, NVIDIA profiles, refresh rates, or Windows display settings.
Leave the secondary PA connected to native HDMI.
Move only the primary PA’s USB-C/DisplayPort cable to the laptop’s other TB5 port.
Let the displays settle. Don’t correct any Windows topology changes yet.
Tell me when ready.

ready.

## Assistant

The physical move caused **no observable Windows or NVIDIA topology change**:

- primary DP PA remains target 8452, connector instance 1, source ID 0, 1440p/120 Hz;
- eDP+HDMI remain cloned on source ID 2, with eDP at 240 Hz and HDMI at 120 Hz;
- every target remains `displayInVrrMode=1`; and
- the Godot Fixed Refresh profile is unchanged.

With only one external DP target active, NVIDIA therefore appears to assign the display back to the same logical connector/head regardless of which TB5 jack is used. There is nothing to normalize.

This gives us a clean physical-jack A/B despite the identical public state. Keep everything exactly as it is and perform:

1. Run `VsyncStutterTest.exe` on the main DP PA.
2. If it is not smooth with the indicator, stop and report that result without opening Godot.
3. If it passes, close it.
4. Launch Godot 4.6.3 normally through the project manager and open `C:\Users\k\Repository\Godot\VsyncStutterTest\Godot`.
5. Record any monitor blink and confirm whether Godot is smooth without the G-SYNC indicator.
6. Close Godot completely.
7. Immediately run `VsyncStutterTest.exe` again on the main DP PA.
8. Report its indicator and smoothness, plus the Godot observations.

If the failure repeats, physical TB5 jack alone is ruled out. If this arm succeeds, a hidden physical-route difference exists below the connector/source information exposed by Windows and NVAPI.

The raw post-hotplug capture is recorded in [recovery-attempt-post-godot-failure.md](C:/Users/k/Repository/NvidiaTroubleshooting/2026-08-28%20godot%20aoe4%20gsync%20investigation/recovery-attempt-post-godot-failure.md), commit `7e34e05`.

---

## User

i ran VsyncStutterTest and it behaved well.
i launched the godot project and it launched on the secondary hdmi monitor smoothly without blinking. however, when i moved it to the primary monitor it did a 3 second blink. usage was fine and no gsync indicator. i then closed it.
i ran VsyncStutterTest and it was choppy and didn't display the gsync indicator.

on a side note, does the 2025 Asus ROG Strix G18 G815LR-IS97 have any sort of "EC Reset" like the legion 9i has in which you hold the power button down for about 30 seconds?

## Assistant

The alternate TB5 jack reproduced the same failure, so the physical jack alone is ruled out. The post-failure capture again shows primary target 8452 changing from `displayInVrrMode=1` to `0`, with topology and the Godot profile unchanged.

The important new localization is **where the transition occurred**:

- Godot opened and ran harmlessly on the HDMI+eDP clone.
- Moving its Fixed Refresh window onto the G-SYNC-capable DP target caused the three-second blank.
- That DP target then remained outside VRR mode after Godot exited.

The strongest current hypothesis is therefore not merely “Godot process launch.” It is the driver applying Fixed Refresh when the editor surface enters the primary VRR target, then failing to restore that target afterward.

Yes, the G815LR has the ASUS equivalent of an EC reset. ASUS calls it an **EC reset / RTC reset / hard reset**, and its default notebook procedure uses **40 seconds**, not Lenovo's approximately 30 seconds. ASUS says some models use a 20-second design signaled by rapid power-LED flashing after about 15 seconds, but I found no G815LR-specific official instruction replacing the general 40-second procedure. [ASUS EC-reset instructions](https://www.asus.com/us/support/faq/1050239/)

The procedure is:

1. Save work and shut Windows down completely.
2. Disconnect all external devices, including both monitors and USB devices.
3. Leave/connect the ASUS AC power adapter.
4. Hold the power button continuously for 40 seconds.
5. Release it and power the laptop on normally.

The next boot may perform full memory training and remain at the ROG logo or show no display for several minutes. Keep AC connected and do not force it off; ASUS says newer systems can require roughly 5-10 minutes. [ASUS post-reset boot guidance](https://www.asus.com/us/support/faq/1042613/)

I also verified this machine is `ROG Strix G18 G815LR_G815LR` on BIOS `G815LR.338`; ASUS currently lists 338 for the G815LR, so this is not an obvious old-BIOS case. [G815LR BIOS support page](https://www.asus.com/us/supportonly/g815lr/helpdesk_bios/)

I would **not perform the EC reset yet**. It is a reasonable later experiment because the EC may participate in USB-C/TB power, mux, or retimer initialization, but the required shutdown also resets the NVIDIA driver. We could not attribute any improvement specifically to the EC. The global G-SYNC cycle already repairs the failed target immediately, which points more directly to NVIDIA's volatile display state.

The next placement-only A/B is:

1. While the system is still in the captured failed state, reopen the Godot project, move it to the HDMI clone, and close it there so Godot saves that placement. Further blinking is not diagnostically important because VRR is already failed.
2. Toggle global G-SYNC off/apply/on/apply.
3. Run the neutral control on primary DP and confirm smooth G-SYNC, then close it.
4. Reopen Godot. If it does not open on the HDMI clone, stop and report without moving it.
5. If it opens on HDMI, keep it entirely there, use it briefly, and close it there. Do not move it onto primary DP.
6. Immediately run the neutral control on primary DP and report its indicator and smoothness.

If this succeeds, crossing the Fixed Refresh editor onto the VRR primary is necessary in the current allocation and keeping the editor on HDMI is a potential operational workaround. If it fails, Godot launch/exit can poison the DP target even without placing a window on it.

The transition capture is recorded in [recovery-attempt-post-godot-failure.md](C:/Users/k/Repository/NvidiaTroubleshooting/2026-08-28%20godot%20aoe4%20gsync%20investigation/recovery-attempt-post-godot-failure.md), and the EC-reset research is in [asus-g815lr-ec-reset-note.md](C:/Users/k/Repository/NvidiaTroubleshooting/2026-08-28%20godot%20aoe4%20gsync%20investigation/asus-g815lr-ec-reset-note.md). Evidence commit: `f3b090a`.

---

## User

i did this:
While VRR is already failed, reopen Godot, move it to the HDMI clone, and close it there to save its placement.
Cycle global G-SYNC off/apply/on/apply.
Verify the neutral control works, then close it.
Reopen Godot.
If it opens on HDMI, keep it entirely there and close it there.
Immediately test VsyncStutterTest.exe on primary DP.

And it succeeded.

## Assistant

Confirmed by the post-test driver capture. Primary DP target 8452 remained `displayInVrrMode=1`; HDMI and eDP also remained at `1`. The topology, 120/120/240-Hz target timings, clone membership, and Godot's nine-setting Fixed Refresh profile are unchanged.

Godot still rewrote DRS at 04:06:08 and 04:06:40, yet later G-SYNC survived. That gives us a decisive within-allocation A/B:

| Godot behavior | Result afterward |
| --- | --- |
| Opens on HDMI, then moves onto primary DP | Three-second blank; target 8452 becomes mode `0`; later G-SYNC fails |
| Opens, stays, and closes entirely on HDMI | No damaging transition; target 8452 stays mode `1`; later G-SYNC works |

So the current operational workaround is now precise: **keep the Godot editor entirely on the native-HDMI-plus-eDP clone and never move any part of it onto the primary G-SYNC DP display.** Godot's process launch, project open, DRS save/reload, use on HDMI, and exit are all insufficient to cause the failure by themselves.

Do not perform the EC reset now; we have found a narrower working behavior and an EC reset would change many lower-level variables.

The next small durability check is:

1. Do not toggle G-SYNC or change any display/profile setting.
2. Launch Godot again; confirm it opens on the HDMI clone.
3. Keep it entirely there, use it briefly, and close it there.
4. Immediately run `VsyncStutterTest.exe` on primary DP.
5. Report its indicator and smoothness.

If this second consecutive cycle succeeds, the placement workaround is repeatable within the current boot. The later durability test is one reboot with the placement left saved, followed by the same sequence.

I updated the [recovery guide](C:/Users/k/Repository/NvidiaTroubleshooting/2026-08-28%20godot%20aoe4%20gsync%20investigation/last-known-good-state-guide.md), [failed-state/placement evidence](C:/Users/k/Repository/NvidiaTroubleshooting/2026-08-28%20godot%20aoe4%20gsync%20investigation/recovery-attempt-post-godot-failure.md), and [root-cause synthesis](C:/Users/k/Repository/NvidiaTroubleshooting/2026-08-28%20godot%20aoe4%20gsync%20investigation/root-cause-synthesis.md). Evidence commit: `2b7c7f4`.

---

## User

i did this:
Don’t toggle G-SYNC or change any settings.
Launch Godot again and confirm it opens on HDMI.
Keep it there, use it briefly, and close it there.
Immediately run VsyncStutterTest.exe on primary DP.
Report its indicator and smoothness.

everything works smoothly.
godot editor ran smoothly without blinks and without gsync indicator.
VsyncStutterTest ran smoothly with gsync indicator.

## Assistant

Confirmed by the final driver capture. This second consecutive cycle preserved:

- target 8452 at `displayInVrrMode=1`;
- HDMI and eDP targets at mode `1`;
- the same DP-plus-HDMI/eDP-clone topology and 120/120/240-Hz target timings; and
- the same nine-setting Godot Fixed Refresh profile.

Godot performed another DRS save at 04:09:42, but later G-SYNC remained healthy. The placement workaround is therefore repeatable within this boot without an intervening global reset:

> Keep Godot entirely on the native-HDMI-plus-eDP clone; do not move any part of the editor onto primary DP.

The next durability boundary is reboot persistence. When convenient:

1. Leave Godot's HDMI placement and every display/G-SYNC/profile setting saved exactly as they are.
2. Restart Windows normally. Do not perform the EC reset for this test.
3. After signing in, do not open Godot, Unity, `VsyncStutterTest.exe`, or another 3D application, and do not change display settings.
4. Tell me when ready so I can capture the pre-application topology, timings, profile, and live VRR state.

After that capture, the sequence will be neutral control -> Godot kept entirely on HDMI -> immediate neutral control. This will distinguish a persistent operational workaround from a same-boot-only allocation.

The repeated success is recorded in the [placement evidence](C:/Users/k/Repository/NvidiaTroubleshooting/2026-08-28%20godot%20aoe4%20gsync%20investigation/recovery-attempt-post-godot-failure.md), [recovery guide](C:/Users/k/Repository/NvidiaTroubleshooting/2026-08-28%20godot%20aoe4%20gsync%20investigation/last-known-good-state-guide.md), and [root-cause synthesis](C:/Users/k/Repository/NvidiaTroubleshooting/2026-08-28%20godot%20aoe4%20gsync%20investigation/root-cause-synthesis.md). Evidence commit: `4418885`.

---

## User

i did this:
Leave Godot’s HDMI placement and all settings unchanged.
Restart Windows normally—don’t perform the EC reset.
After signing in, don’t open any 3D application or change display settings.
Tell me when ready so I can capture the pre-application state.

ready

## Assistant

The untouched post-reboot state is healthy:

- primary DP target 8452 is `displayInVrrMode=1`;
- HDMI and eDP targets are also mode `1`;
- DP/HDMI/eDP target timings remain 120/120/240 Hz;
- the 1440p HDMI+eDP clone persisted;
- the Godot profile still explicitly requests Fixed Refresh; and
- `VsyncStutterTest.exe` remains unprofiled.

The reboot did change volatile Windows source assignment: DP moved from source ID 0 to source ID 1, while the clone moved from source ID 2 to source ID 0. Do not try to correct those numbers; they are evidence of driver allocation, not user-facing settings. The current allocation is healthy and resembles the earlier rebooted working family.

Now run the reboot-persistence sequence without changing anything:

1. Run `VsyncStutterTest.exe` on primary DP.
2. If it is not smooth with the indicator, stop and report without opening Godot.
3. If it passes, close it.
4. Launch Godot normally and open the project.
5. If Godot does not open on the HDMI clone, stop and report without moving it.
6. If it opens on HDMI, keep it entirely there, use it briefly, and close it there.
7. Immediately run `VsyncStutterTest.exe` on primary DP.
8. Report its indicator and smoothness, along with any Godot blink and the editor's behavior.

If this succeeds, the HDMI-only placement workaround has survived a reboot without a global G-SYNC recovery cycle. The pre-application baseline is recorded in [recovery-attempt-post-godot-failure.md](C:/Users/k/Repository/NvidiaTroubleshooting/2026-08-28%20godot%20aoe4%20gsync%20investigation/recovery-attempt-post-godot-failure.md), commit `7375fd9`.

---

## User

i did this:
Run VsyncStutterTest.exe on primary DP.
If it fails, stop without opening Godot.
Otherwise close it.
Launch Godot normally and open the project.
If it doesn’t open on HDMI, stop without moving it.
If it opens on HDMI, keep it entirely there and close it there.
Immediately rerun VsyncStutterTest.exe on primary DP.
Report both applications’ behavior and any blinking.

everything works smoothly.
godot editor ran smoothly without blinks and without gsync indicator.
VsyncStutterTest ran smoothly with gsync indicator.

## Assistant

The post-test capture confirms that the workaround survived reboot cleanly:

- primary DP target 8452 remained `displayInVrrMode=1`;
- HDMI and eDP targets remained in mode `1`;
- the post-reboot source assignment, clone membership, and 120/120/240-Hz timings remained unchanged;
- Godot stayed Fixed Refresh and ran without the indicator; and
- Godot's expected DRS writes occurred without harming later G-SYNC.

The validated operational rule is now:

> Keep Godot entirely on the native-HDMI-plus-eDP clone. Do not move any part of the editor onto the primary G-SYNC DP display.

This has succeeded for two consecutive same-boot cycles and the first controlled cycle after a normal reboot, with no intervening global recovery. No EC reset is warranted for normal use.

If Godot accidentally enters primary DP and later G-SYNC fails:

1. move/save Godot back on HDMI while the state is already failed;
2. close Godot;
3. cycle global G-SYNC off/apply/on/apply; and
4. keep later Godot sessions on HDMI.

The remaining untested durability boundaries are sleep, hibernate, NVIDIA driver update/reset, and monitor power/hotplug. They do not need to be tested unless one becomes relevant to normal use.

I updated the [recovery guide](C:/Users/k/Repository/NvidiaTroubleshooting/2026-08-28%20godot%20aoe4%20gsync%20investigation/last-known-good-state-guide.md), [placement evidence](C:/Users/k/Repository/NvidiaTroubleshooting/2026-08-28%20godot%20aoe4%20gsync%20investigation/recovery-attempt-post-godot-failure.md), and [root-cause synthesis](C:/Users/k/Repository/NvidiaTroubleshooting/2026-08-28%20godot%20aoe4%20gsync%20investigation/root-cause-synthesis.md). Evidence commit: `b945267`.
