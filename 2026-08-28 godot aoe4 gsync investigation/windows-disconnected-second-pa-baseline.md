# Windows-disconnected second-PA baseline

Capture time: 2026-08-28 22:58 PDT

Prepared state:

- both PA278QGV cables remain connected;
- both monitors remain powered;
- MediaSync-on PA target 8450 remains active;
- MediaSync-off PA target 8452 was set to `Disconnect this display` in Windows; and
- internal target 8449 remains active.

No Unity, Godot, or `VsyncStutterTest.exe` process was opened after the Windows topology change.

## Windows active paths

Windows reports exactly two active display paths:

| Target | Display | Active scanout |
|---:|---|---|
| 8449 | internal NE180QDM-NZC | yes |
| 8450 | MediaSync-on PA278QGV | yes |
| 8452 | MediaSync-off PA278QGV | no |

This is the intended active-head-count isolation: one external scanout head plus the internal panel.

## NVIDIA connection state

NVAPI still enumerates all three physical displays. For target 8452 it reports:

```text
active=0
osVisible=1
connected=1
physicallyConnected=1
current DP lanes=0
```

Thus the second PA is physically present but not active for scanout. This cleanly distinguishes Windows active-display state from cable/monitor presence.

## VRR-query anomaly

The internal panel remains queryable and reports `VRR possible=1`, `displayInVrrMode=1`.

The active MediaSync-on PA target 8450 remains physically active and its Adaptive-Sync data still reports the expected 20583-us maximum interval, but `NvAPI_Disp_GetVRRInfo` returned generic `NVAPI_ERROR` (`-1`). A second query five seconds later returned the same error.

The inactive target 8452 returns `NVAPI_INVALID_DISPLAY_ID` from the VRR query, which is expected for an inactive display ID.

The persistent generic error on active target 8450 means the Windows topology change cannot yet be called a healthy G-SYNC baseline solely from API state. Run unprofiled `VsyncStutterTest.exe` before Unity:

- if it shows the indicator and runs smoothly, the control establishes that G-SYNC is healthy despite the query anomaly;
- if it lacks the indicator or is choppy, the Windows topology change itself disturbed G-SYNC and a global off/on recovery is required in this topology before testing Unity.

## Healthy pre-Unity control

The user ran `VsyncStutterTest.exe` in this topology. It displayed the top-right G-SYNC indicator and ran smoothly. Therefore G-SYNC is healthy despite `NvAPI_Disp_GetVRRInfo` returning a generic error for target 8450.

The 23:01 post-control capture still shows exactly two active Windows paths and target 8452 physically connected but inactive. The DRS stores retain their 22:50:13 timestamps and pre-Unity hashes:

```text
nvdrsdb0.bin: 163CE7E0C26D5B3C136AC6514464A52A2E1660951D712D9D58D0F97E20050E85
nvdrsdb1.bin: 929E7C1F106022BF0BCC531D5FE4C6786984DB776B9D6894ED149C5F22FB99DC
```

This is a valid pre-Unity baseline for the active-head-count test. Next run Unity under its existing Fixed Refresh profile, close it, and immediately run `VsyncStutterTest.exe` without changing G-SYNC or display topology.

## Unity transition result

With target 8452 still physically connected but inactive in Windows, the user:

1. opened the same Unity editor/project under its Fixed Refresh profile;
2. observed no monitor blink;
3. observed Unity behaving acceptably without G-SYNC;
4. closed Unity; and
5. immediately ran `VsyncStutterTest.exe`, which displayed the G-SYNC indicator and ran smoothly.

The 23:07 post-test capture remained unchanged:

```text
Windows active paths: target 8449 internal + target 8450 external
target 8452: active=0, connected=1, physicallyConnected=1
DRS timestamps: 22:50:13
DRS hashes: identical to the 23:01 pre-Unity capture
```

This is the decisive active-head-count result. The second PA's cable, power, EDID presence, and NVIDIA physical enumeration are not sufficient for the bug. The Fixed Refresh transition requires **two external targets active for scanout** to produce the monitor blink and sticky loss of later G-SYNC.

The generic VRR-info query error for target 8450 persists in this topology, but both pre- and post-Unity functional controls prove G-SYNC works. Treat the error as an NVAPI/query anomaly specific to this Windows-disconnected layout, not as a failed VRR state.
