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
