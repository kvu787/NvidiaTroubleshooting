# Unity Fixed Refresh transition reproduces the sticky G-SYNC failure

Date: 2026-08-28 PDT

## Baseline

The display state was held constant:

- external PA target 8450: OSD MediaSync on, VRR possible;
- external PA target 8452: OSD MediaSync off, VRR not possible;
- internal target 8449 active;
- global G-SYNC not toggled; and
- Godot not opened.

Before Unity, AoE4 displayed the G-SYNC indicator and behaved normally. The 22:36 post-AoE NVAPI capture still reported target 8450 as `VRR possible=1` and `displayInVrrMode=1`.

## Transition and application results

The user then:

1. opened the same Unity editor/project used for the topology comparison;
2. observed a two-to-three-second monitor blink;
3. observed that Unity behaved acceptably and did not show G-SYNC, as expected from its Fixed Refresh NVIDIA profile;
4. closed Unity;
5. opened AoE4 and observed no G-SYNC indicator plus clearly choppy gameplay;
6. closed AoE4; and
7. ran `C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe`, which also showed no G-SYNC indicator and clearly choppy animation.

The user is replacing AoE4 with `VsyncStutterTest.exe` as the routine G-SYNC control application.

## Live post-Unity state

The 22:43 NVAPI capture reports:

| Target | VRR possible | Display in VRR mode | Change from 22:36 |
|---:|---:|---:|---|
| 8450 MediaSync-on PA | 1 | 0 | `displayInVrrMode` changed from 1 to 0 |
| 8452 MediaSync-off PA | 0 | 0 | unchanged |
| 8449 internal panel | 1 | 1 | unchanged |

Target 8450 remains physically capable of VRR (`possible=1`, maximum Adaptive-Sync interval 20583 us), but the driver no longer places its active display mode in VRR mode. This is direct API evidence of the sticky live state corresponding to the missing indicator and choppy output.

## Current Unity profile

The current matching predefined `Unity 3D` profile has a basename association for `unity.exe` and explicit user settings including:

```text
VRR requested state: disabled
G-SYNC application override: fixed refresh
G-SYNC mode: disabled
```

The profile contains 15 settings in the current snapshot, versus the earlier 14:28 Unity audit's eight predefined-only settings. The earlier audit is historically correct for its timestamp; the profile was configured afterward.

## Unity did not rewrite DRS during this test

At 22:43, after Unity, AoE4, and `VsyncStutterTest.exe` had closed:

```text
nvdrsdb0.bin last write: 22:11:41
nvdrsdb1.bin last write: 22:11:41
nvdrsdb0.bin SHA-256: 97110C9B3516B3622B3A9452CF8DF40C0B7788A2BDE9DCA0EA2F308ACEE14D02
nvdrsdb1.bin SHA-256: 97110C9B3516B3622B3A9452CF8DF40C0B7788A2BDE9DCA0EA2F308ACEE14D02
```

The Unity transition occurred after the successful 22:36 AoE control and before the 22:43 failed-state capture. The DRS databases were not written during that interval. Therefore Unity did not need to save/reload DRS to trigger the monitor blink or sticky VRR failure. Activation/deactivation of the already-stored Fixed Refresh profile is sufficient in this display topology.

`nvAppTimestamps` changed at 22:39:59, but that is NVIDIA App activity/catalog metadata, not the DRS profile database.

## Neutral replacement control

An exhaustive direct lookup for:

```text
C:\Users\k\Repository\Godot\VsyncStutterTest\MyBuildOutput\VsyncStutterTest.exe
```

returned:

```text
FindApplicationByName(full path): NVAPI_EXECUTABLE_NOT_FOUND
FindApplicationByName(basename): NVAPI_EXECUTABLE_NOT_FOUND
No DRS application association or target-related profile name was found.
```

Thus its failed G-SYNC state is inherited live/global behavior, not a per-application Fixed Refresh rule.

## Revised causal conclusion

Godot's unconditional DRS profile writer is not required for the core failure. The necessary tested sequence is now:

1. two external PA display heads are active;
2. one is a VRR-capable active display and the other may be non-VRR;
3. an application whose NVIDIA profile is Fixed Refresh starts, causing a two-to-three-second display transition;
4. after that application exits, target 8450 remains VRR-capable but `displayInVrrMode` is stuck at 0; and
5. unrelated globally G-SYNC-allowed applications cannot activate G-SYNC.

This is an NVIDIA driver/display-topology profile-transition bug. Godot exposed it and its profile-writing implementation is undesirable, but neither Godot nor its DRS save is the root cause. Unity reproduces the transition by ordinary activation of an already-existing Fixed Refresh profile.

## Next isolation

Recover G-SYNC once and verify target 8450 returns to `displayInVrrMode=1`. Then leave both PAs physically connected but disable the MediaSync-off PA in Windows. With only one external scanout head active, repeat:

```text
Unity Fixed Refresh -> close Unity -> VsyncStutterTest.exe
```

- If there is no blink/sticky failure, active external head count is confirmed as the topology condition.
- If it still fails, physical connection/presence rather than active scanout is sufficient.

## Verified recovery

The user toggled global G-SYNC off/apply and on/apply without launching any test application or editor. At 22:51, NVAPI reported:

```text
target 8450: VRR possible=1, displayInVrrMode=1
target 8452: VRR possible=0, displayInVrrMode=0
target 8449: VRR possible=1, displayInVrrMode=1
```

Target 8450 therefore returned from the post-Unity stuck state (`displayInVrrMode=0`) to the correct VRR-capable mode (`displayInVrrMode=1`). This maps the known UI recovery directly to the NVAPI state bit.

The off/on applies rewrote the DRS stores at 22:50:13, as expected for explicit settings changes. The next step is a clean `VsyncStutterTest.exe` run in this recovered two-external state before changing the Windows display topology.

## Replacement control validated

The user ran unprofiled `VsyncStutterTest.exe` in the recovered state. It displayed the top-right G-SYNC indicator and ran smoothly. The 22:55 post-control capture remained correct and unchanged: target 8450 was `VRR possible=1`, `displayInVrrMode=1`; target 8452 remained non-VRR; all three display paths remained active.

`VsyncStutterTest.exe` is therefore validated as a positive G-SYNC control, not merely as a failed-state detector. The next topology arm can use it instead of AoE4.
