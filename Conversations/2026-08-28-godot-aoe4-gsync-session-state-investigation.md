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
