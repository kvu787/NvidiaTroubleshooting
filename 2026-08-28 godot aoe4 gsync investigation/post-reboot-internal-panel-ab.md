# Post-reboot internal-panel A/B

Captured: 2026-08-29 00:11 PDT

## User observation after reboot

After restarting the laptop and returning to normal usage, the user found a new topology dependency:

- with the internal laptop display active, G-SYNC behavior is smooth;
- using Windows 11 Display Settings to `Disconnect this display` for the internal panel restores poor G-SYNC behavior; and
- reconnecting the internal panel restores smooth behavior.

The user separately tried the HDMI-connected PA with OSD MediaSync both on and off; that did not materially change this internal-panel A/B.

The desired physical setup is two external monitors with the laptop tucked out of view. Keeping an extended desktop on the hidden internal panel is undesirable because windows can be placed or restored there.

## Current healthy state

The user currently reports smooth behavior with:

```text
Internal panel:
  active at about 240 Hz

Primary PA278QGV:
  TB5/USB-C DisplayPort
  119.998 Hz
  OSD MediaSync on

Secondary PA278QGV:
  native HDMI
  119.998 Hz
  OSD MediaSync on

Global G-SYNC:
  on

Godot profile:
  VRR requested state disabled
  G-SYNC Fixed Refresh
```

## Read-only healthy baseline

All three active paths are owned by the RTX 5070 Ti Laptop GPU at post-reboot adapter LUID `00000000:0001036E`:

| Target | Route | Mode | NVAPI mode state |
|---|---|---|---|
| 8449 internal | embedded DisplayPort | 2560x1600 at about 240 Hz | `possible=1`, `displayInVrrMode=1` |
| 8450 primary PA | TB5/USB-C external DisplayPort | 2560x1440 at 119.998 Hz | `possible=1`, `displayInVrrMode=1` |
| 8448 secondary PA | native HDMI | 2560x1440 at 119.998 Hz | `possible=1`, `displayInVrrMode=1` |

The Godot profile remains semantically unchanged and correctly associated with `godot_v4.6.3-stable_win64.exe`.

## Revised topology conclusion

The working HDMI arm no longer has a refresh-rate confound: both external PAs are now 119.998 Hz, and the setup is smooth while the internal panel is active. Therefore the earlier success was not dependent on lowering the HDMI PA to 60 Hz.

The accumulated matrix is now:

| External routes | Internal panel | Result |
|---|---|---|
| one TB5/DP PA | active | smooth |
| two TB5/DP PAs | active | poor |
| two TB5/DP PAs | disconnected | poor |
| TB5/DP PA + HDMI PA | active | smooth |
| TB5/DP PA + HDMI PA | disconnected | user reports poor; exact pre-application state not yet captured |

The internal eDP path is therefore a necessary stabilizing member of the current two-external mixed-route topology, while it is not sufficient to repair the dual-TB5/DisplayPort topology. The HDMI PA's OSD MediaSync state is not the discriminator in the user's tests.

The next isolation must distinguish two possibilities:

1. Windows-disconnecting the internal panel immediately damages G-SYNC even before a Fixed Refresh editor opens; or
2. the two-external-only topology begins healthy but makes the subsequent Fixed Refresh transition fail.

Capture the internal-panel-disconnected state before opening any 3D application, then run `VsyncStutterTest.exe` as a pre-editor control.
