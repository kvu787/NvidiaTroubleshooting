# Last-known-good state and recovery guide

Last updated: 2026-08-29 PDT

This is the authoritative description and best-known recovery procedure for the last configuration that survived a reboot and repeatedly produced all of the desired application behavior:

- two usable external desktop spaces, with no hidden third desktop that can capture windows;
- Godot 4.6.3 and Unity running smoothly at Fixed Refresh without the G-SYNC indicator;
- `VsyncStutterTest.exe` and AoE4 automatically using G-SYNC afterward;
- no recurring monitor blank within the same boot; and
- no manual global G-SYNC toggle during normal application switching.

It is a workaround for a route-sensitive NVIDIA driver/display-state defect. It is not proof that the underlying bug has been repaired, and a later controlled recovery attempt reproduced the sticky failure even after restoring every recorded persistent setting and timing. Treat this as the best known state, not a deterministic cure.

## Important current-state finding

The first read-only capture taken after the user reported being in a bad state again found **two concrete deviations** from the last-known-good state:

1. The physical cabling and clone membership are still correct, but the internal eDP panel's target signal is now `2560x1600 at 60 Hz`, rather than the known-good approximately `240 Hz`.
2. The `Godot Engine` DRS profile still matches `godot_v4.6.3-stable_win64.exe`, but its explicit G-SYNC application override `0x10A879CF = 4` (`Fixed Refresh`) is gone. It now inherits the global value `0` (`Allow`).

The current Unity profile has also lost its explicit Fixed Refresh override. That matters only if Unity is used as an editor/control in the recovery test.

These were real configuration changes, not merely an unreliable G-SYNC indicator. The recovery attempt restored both, then passed the pre-Godot neutral control. After Godot exited, however, `VsyncStutterTest.exe` lost G-SYNC again.

The failed-state capture then proved:

- the DP PA was 2560x1440 at 119.998 Hz;
- the internal eDP target was back to 2560x1600 at 240 Hz;
- internal eDP and native HDMI shared a 2560x1440 clone source;
- the Godot profile explicitly contained `0x1094F1F7 = 0` and `0x10A879CF = 4`;
- `VsyncStutterTest.exe` remained unassociated in DRS; but
- the primary DP target 8452 was `VRR possible=1` and `displayInVrrMode=0`, while HDMI target 8448 and eDP target 8449 remained at `displayInVrrMode=1`.

This is a clean recurrence of the NVIDIA live per-target restoration failure inside the nominal workaround. The topology remains useful because it produced long stable runs and survived one reboot, but it is not independently sufficient.

## The exact last-known-good configuration

### Physical display routes

| Physical display | Connection | Role |
| --- | --- | --- |
| PA278QGV A | Exactly one TB5/USB-C DisplayPort output | Separate primary Windows desktop; the G-SYNC test target |
| PA278QGV B | Laptop's native HDMI output | Secondary external image; cloned with the internal panel |
| Internal `NE180QDM-NZC` | Built-in eDP | Active scanout target; cloned with the HDMI PA |

Do **not** connect both external PAs through the two TB5/USB-C DisplayPort outputs. The otherwise equivalent clone arrangement reproducibly brought the G-SYNC problems back. Each TB5 port worked with one PA by itself, so this is not evidence of one defective port; the bad selector is two simultaneously active external DP-over-USB-C routes.

The internal panel must remain logically active in Windows. If closing the laptop lid deactivates it, the workaround no longer matches the tested state. Because it is cloned with the HDMI PA, it does not create a third desktop on which windows can become lost.

### Windows topology and timings

The known-good topology has three active physical targets but only two Windows desktop sources:

| Target | Windows source relationship | Source mode | Target signal |
| --- | --- | --- | --- |
| TB5/DP PA | Separate source; main display | 2560x1440 | 2560x1440 at 119.998 Hz (`120 Hz` in the UI) |
| Native-HDMI PA | Cloned with internal eDP | 2560x1440 | 2560x1440 at 119.998 Hz (`120 Hz`) |
| Internal eDP | Cloned with native-HDMI PA | Shared 2560x1440 source | Native 2560x1600 at approximately 240 Hz |

The Windows display numbers, source IDs, GDI names, and NVIDIA target IDs are not stable. For example, the working DP PA appeared as target 8450 in one session and 8452 after later routing/reboot changes. Identify displays by using the Windows **Identify** button and by their physical connector; do not blindly duplicate displays numbered `1` and `3` just because those were the numbers in one capture.

Both PA OSDs used `MediaSync: On` in the final known-good configuration. MediaSync did not explain the failure in the A/B tests, but leaving both on removes an unnecessary difference from the proven state.

### NVIDIA settings

- Global G-SYNC: **enabled**.
- Godot executable: `C:\Users\k\Program\Godot_v4.6.3-stable_win64.exe\Godot_v4.6.3-stable_win64.exe`.
- Matching DRS profile: `Godot Engine`, with a basename association for `godot_v4.6.3-stable_win64.exe`.
- Godot `Monitor Technology`: **Fixed Refresh**.
- If Unity is expected to behave like the validated control, its editor profile must also use **Fixed Refresh**.
- `C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe` must remain unassociated with a Fixed Refresh profile and inherit global G-SYNC.

The relevant known-good Godot DRS values were:

| Setting | Expected value | Source |
| --- | --- | --- |
| `0x1094F1F7`, VRR requested state | `0`, disabled | NVIDIA Control Panel's Fixed Refresh selection |
| `0x10A879CF`, G-SYNC application override | `4`, Fixed Refresh | NVIDIA Control Panel's Fixed Refresh selection |
| `0x1194F158`, G-SYNC mode | `1`, fullscreen only | Godot native-OpenGL setup |
| `0x20C1221E`, threaded optimization | `2`, disabled | Godot native-OpenGL setup |

Do not use DRS database hashes or the total profile count as a restore target. Godot legitimately saves the database during native-OpenGL initialization, NVIDIA updates predefined profiles, and the binary hashes change even when the effective behavior remains correct.

## Controlled rebuild procedure

Perform the steps in this order. The ordering prevents a stale live VRR state from obscuring a topology or profile error, but it cannot guarantee that the driver will choose the same private display-head allocation as the earlier working run.

### 1. Quiesce 3D applications

Close all of the following before changing anything:

- Godot editors and project managers;
- Unity editors;
- `VsyncStutterTest.exe`;
- AoE4 and other games; and
- NVIDIA Control Panel, NVIDIA App, and NVIDIA Profile Inspector after any pending changes have been applied.

Unity Hub may remain running, but no Unity Editor process should be open.

### 2. Restore the physical routes

1. Connect one PA to one TB5/USB-C DisplayPort output.
2. Connect the other PA directly to the laptop's native HDMI output.
3. Power both PAs on.
4. Set `MediaSync: On` in both PA OSDs for exact parity with the final validated state.
5. Keep the internal panel available to Windows. Do not choose **Disconnect this display** for it.

If both PAs are currently connected through TB5/DisplayPort, move the secondary to native HDMI before doing any software reconstruction.

### 3. Rebuild the Windows display topology from an extended state

1. Open **Settings -> System -> Display**.
2. Click **Identify** and write down which rectangle is:
   - the TB5/DisplayPort PA;
   - the native-HDMI PA; and
   - the internal laptop panel.
3. Temporarily set all three displays to **Extend desktop to this display**. This makes their refresh rates independently configurable.
4. Open **Advanced display** and restore:
   - TB5/DP PA: `2560x1440`, `120 Hz`;
   - HDMI PA: `2560x1440`, `120 Hz`; and
   - internal panel: `2560x1600`, `240 Hz` or the highest approximately-240-Hz choice.
5. Return to the display layout, select the TB5/DP PA, and enable **Make this my main display**.
6. Select either member of the intended clone pair and choose **Duplicate desktop on the internal panel and the native-HDMI PA**. Do not include the TB5/DP PA in the clone.
7. Click **Apply**, then **Keep changes**.
8. Select the duplicate group and set its Windows source resolution to `2560x1440`. This avoids letterboxing the HDMI PA. The internal 16:10 panel may scale the shared 16:9 image, but it is tucked away and is not a separate desktop.

At the end, Windows should show two desktop rectangles: the separate main TB5/DP PA and one combined internal-plus-HDMI duplicate group.

If the internal panel falls back to 60 Hz after cloning, do not continue to application testing. Break the clone back into three extended displays, restore the internal panel to approximately 240 Hz, and create the internal-plus-HDMI clone again. The current bad-state capture demonstrates that clone membership alone does not guarantee the known-good target timing.

### 4. Restore the Godot Fixed Refresh profile

Use NVIDIA Control Panel for the edit because NVIDIA App's application catalog previously hid, duplicated, and recreated DRS profiles in confusing ways.

1. Open **NVIDIA Control Panel -> Manage 3D settings -> Program Settings**.
2. Select the entry that resolves to the Godot 4.6.3 executable. Prefer the existing `Godot Engine` basename-associated profile rather than creating another exact-path profile in NVIDIA App.
3. Set **Monitor Technology** to **Fixed Refresh**.
4. Click **Apply** and wait for completion.
5. Close NVIDIA Control Panel.

Optional verification in NVIDIA Profile Inspector:

- select `Godot Engine`;
- confirm the application list includes `godot_v4.6.3-stable_win64.exe`;
- confirm `G-SYNC - Application State` is `Fixed Refresh Rate` / raw value `0x00000004`; and
- confirm `G-SYNC - Application Requested State` is disabled / raw value `0x00000000`.

It is acceptable for the one `Godot Engine` profile also to contain the 4.4.1 executable association. Avoid manufacturing a second profile solely to obtain an exact path. If Unity will be used, repeat the same NVCP **Monitor Technology: Fixed Refresh** check for the Unity Editor profile.

### 5. Re-arm live G-SYNC only if the neutral control is still bad

Topology reconstruction may re-arm live G-SYNC on its own, so test it first:

1. Do not open Godot or Unity.
2. Run `C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe` on the main TB5/DP PA.
3. Expect the top-right G-SYNC indicator and smooth animation.
4. Close the test.

If the indicator is absent or motion is choppy, perform the verified recovery once:

1. Open NVIDIA App or NVIDIA Control Panel.
2. Disable global G-SYNC and apply.
3. Enable global G-SYNC again and apply.
4. Close the NVIDIA settings UI.
5. Repeat `VsyncStutterTest.exe` on the main TB5/DP PA.

The off/apply/on/apply cycle causes a long display blank, but it is the recovery that was actually proven to clear the sticky state. `Win+Ctrl+Shift+B` has not been validated as a substitute and should not be part of the recovery recipe.

Do not proceed until the neutral control is smooth and shows the indicator. A failure at this stage precedes Godot and therefore cannot be diagnosed as a new Godot transition failure.

## Full acceptance test

### Current placement rule

The latest controlled A/B adds a stricter operational requirement for the current connector-1/source-0 allocation: **keep the Godot editor entirely on the native-HDMI-plus-eDP clone**. Moving it onto the primary G-SYNC DP PA caused a three-second blank and left that target outside VRR mode. Opening, using, and closing Godot entirely on HDMI preserved later G-SYNC even though Godot still saved/reloaded DRS.

This placement rule was not required in an earlier stable allocation, which is why older observations include clean primary-to-clone movement. The visible topology does not uniquely select NVIDIA's private allocation. When recovering the current state, persist Godot's placement on HDMI before the global G-SYNC recovery, then do not let any part of the editor cross onto primary DP. Two consecutive HDMI-only Godot-to-neutral transitions have now succeeded in the same boot without an intervening G-SYNC reset.

Once the neutral control passes:

1. Launch Godot 4.6.3 normally through its project manager. Do **not** use the direct `--rendering-driver d3d12` bypass for this acceptance test.
2. Open `C:\Users\k\Repository\Godot\VsyncStutterTest\Godot` on the HDMI clone. If it opens on primary DP, stop rather than dragging it across displays; first persist its HDMI placement while VRR is already failed, then recover global G-SYNC and restart the acceptance test.
3. Confirm:
   - Godot is smooth;
   - the G-SYNC indicator is absent; and
   - no repeated monitor blank occurs during ordinary use.
4. Close Godot completely.
5. Immediately run `VsyncStutterTest.exe` on the main TB5/DP PA.
6. Confirm the indicator is present and the animation is smooth.
7. Close the test.
8. Launch and close Godot a second time on the HDMI clone without crossing onto primary DP, then repeat the neutral control.

After a reboot, one two-second blank on the first ordinary Godot launch was observed in the otherwise working state. Later Godot launches in that boot were clean, and later G-SYNC remained healthy. Classify that isolated cold blank separately from the original sticky failure. The acceptance test fails if blanks keep recurring or if the immediate post-Godot neutral control loses the indicator or becomes choppy.

That exact post-Godot failure occurred in the 03:45 recovery attempt despite the correct visible topology, timings, and DRS values. If it recurs, do not keep launching Godot. Preserve the failed state for capture, then use the global G-SYNC off/apply/on/apply cycle to restore the neutral control before changing one topology variable at a time.

Expected matrix:

| Stage | G-SYNC indicator | Motion | Blink expectation |
| --- | --- | --- | --- |
| Neutral control before editor | Present | Smooth | None |
| Godot editor | Absent | Smooth | At most one cold first-launch blank after reboot |
| Neutral control immediately after Godot | Present | Smooth | None |
| Second Godot launch in same boot | Absent | Smooth | None |
| Second post-Godot neutral control | Present | Smooth | None |

AoE4 may be used as a secondary confirmation, but `VsyncStutterTest.exe` is faster and is intentionally unprofiled.

## Failure diagnosis by symptom

| Symptom | Most likely mismatch | Action |
| --- | --- | --- |
| Neutral control is bad before Godot | Stale global/live VRR state or incorrect route/topology | Verify mixed DP+native-HDMI route and active eDP clone; then perform the global off/on recovery |
| Godot shows the G-SYNC indicator or mouse motion is choppy | Godot Fixed Refresh override is missing or the wrong profile matched | Restore `Monitor Technology: Fixed Refresh`; verify `0x10A879CF = 4` on the matching profile |
| Godot is correct, but the immediate neutral control loses G-SYNC | Susceptible NVIDIA Fixed Refresh-to-VRR transition returned | Preserve/capture the state; if target remains VRR-capable but outside VRR mode, re-arm global G-SYNC and test a different connector/source allocation |
| Godot is harmless on HDMI but crossing onto primary DP causes a long blank and later G-SYNC loss | Current placement-dependent Fixed Refresh transition | Persist and keep Godot entirely on the HDMI clone; do not move it onto primary DP |
| HDMI PA is letterboxed | Clone source is 2560x1600 | Set the combined internal+HDMI source to 2560x1440 |
| Windows can disappear onto an unseen laptop desktop | Internal panel is extended rather than cloned | Duplicate internal eDP with the native-HDMI PA |
| Windows exposes only the two external targets | Internal eDP is disconnected | Re-enable it and clone it with HDMI |
| Correct clone, but internal target reports 60 Hz | Timing drift from the known-good allocation | Unclone, restore internal to approximately 240 Hz, then clone internal+HDMI again |
| Problems return after moving HDMI PA to the other TB5 port | Both external targets are using DP-over-USB-C | Move one PA back to native HDMI |
| `displayInVrrMode=1`, but the test is visibly bad | Public NVAPI bit is insufficient by itself | Trust the functional indicator/smoothness control and rebuild/re-arm state |

## Fast checklist

Before opening an editor, every box should be true:

- [ ] Exactly one PA uses TB5/USB-C DisplayPort.
- [ ] The other PA uses the laptop's native HDMI output.
- [ ] The TB5/DP PA is the separate main desktop at 2560x1440/120 Hz.
- [ ] Internal eDP is active, not disconnected.
- [ ] Internal eDP is cloned with the HDMI PA, not with the DP PA.
- [ ] The clone source is 2560x1440.
- [ ] The HDMI target is 2560x1440/120 Hz.
- [ ] The internal target is 2560x1600 at approximately 240 Hz, not 60 Hz.
- [ ] Both PA OSDs have MediaSync on for parity with the proven state.
- [ ] Global G-SYNC is enabled.
- [ ] The matching Godot profile says Monitor Technology: Fixed Refresh.
- [ ] `VsyncStutterTest.exe` is smooth and shows the indicator before opening Godot.
- [ ] Godot is persisted on the HDMI clone and will not be moved onto primary DP in the current allocation.

## Why this topology is required

The evidence supports a topology-sensitive NVIDIA driver bug during a per-application `Fixed Refresh -> global G-SYNC` transition. In susceptible topologies, the primary DP target remains VRR-capable but is not restored to live VRR mode after the editor exits. Unity reproduced the failure without writing DRS, proving that Godot's profile writer is not the primary cause. Conversely, Godot repeatedly wrote DRS in the stable mixed-route clone topology without poisoning later G-SYNC.

The mixed route and active eDP clone can select a stable NVIDIA display-head/transmitter/timing allocation, but the failed recovery proves that the visible topology does not uniquely determine the driver's private allocation. Public APIs cannot identify whether the private failing resource is a DP transmitter, PLL, Type-C mux/retimer state, VidPN commit, source/head assignment, or per-head VRR bookkeeping. The best operational starting point remains deliberately narrow: **one external PA on TB5/DP, the other on native HDMI, and active eDP cloned with HDMI at the proven timings**.

Primary evidence:

- `clone-internal-hdmi-baseline.md`
- `post-reboot-clone-route-validation.md`
- `root-cause-synthesis.md`
- `topology-ab-evidence.md`
