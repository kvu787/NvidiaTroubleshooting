# Godot NVIDIA App profile: post-launch findings

Captured on 2026-08-28 after the Godot 4.6.3 project was opened, the editor was used, and Godot was closed.

## Observed behavior

The user observed:

- both monitors blinked for approximately two or three seconds while the project opened;
- the G-SYNC indicator did not appear;
- mouse movement remained smooth; and
- there were no other issues.

The intended workaround therefore succeeded for this launch.

## Decisive profile comparison

| State | Clean baseline | NVIDIA App pre-launch | After Godot launch |
|---|---:|---:|---:|
| Total DRS profiles | 7,957 | 7,958 | 7,959 |
| NVIDIA App exact-path profile | absent | present | present and unchanged |
| `0x10A879CF` in exact-path profile | absent | `4` (Fixed Refresh) | `4` (Fixed Refresh) |
| Godot basename profile | absent | absent | `Godot Engine` present |

Godot did not overwrite or remove the NVIDIA App profile. Instead, two distinct user-defined profiles now coexist:

1. `Godot_v4.6.3-stable_win64.exe`, created by NVIDIA App, associates the exact full executable path and retains seven explicit settings, including Fixed Refresh.
2. `Godot Engine`, created by Godot during OpenGL startup, associates only the executable basename and contains its usual two settings: fullscreen-only G-SYNC mode and disabled OpenGL threaded optimization.

This coexistence is possible because the application association strings differ. The full-path DRS lookup selects the NVIDIA App profile, while the basename lookup selects the Godot profile.

## Which profile was used

NVIDIA App's backend observed the running image at 13:34:27 and again at 13:35:27. Both times it loaded DRS and resolved the full executable path to:

```text
drsData.profileName=Godot_v4.6.3-stable_win64.exe
```

It did not resolve the running path to `Godot Engine`. Together with the absent G-SYNC indicator and smooth mouse movement, this establishes that the exact-path NVIDIA App profile was the applicable profile for this launch. Its Fixed Refresh override prevented the editor from entering G-SYNC/VRR operation.

## What Godot changed

The active DRS database and selector changed at 13:34:09.304, and the profile count increased by one. The new `Godot Engine` profile matches Godot 4.6.3 source in `platform/windows/gl_manager_windows_native.cpp`:

- lines 168-174 use the executable basename and the project/application name;
- lines 204-238 create the named profile and basename application association if absent;
- lines 241-267 store disabled threaded optimization and fullscreen-only VRR mode; and
- line 269 saves the DRS changes.

The NVIDIA App-created exact-path profile is logically identical before and after launch: same name, association, setting count, setting IDs, and values.

## NVIDIA App catalog

The manual Godot entry remains present with the same local ID and initial time. Its only relevant recorded change is:

- before launch: `LastLaunchTimeISO=1601-01-01T00:00:00Z`;
- after launch: `LastLaunchTimeISO=2026-08-28T20:34:11Z`.

This confirms that NVIDIA App recognized the launch while retaining its manual application record.

## Monitor blink

The blink is consistent with a display-pipeline transition when the running executable activates a profile that changes G-SYNC/VRR from the global state to Fixed Refresh. Godot also saved its new DRS profile at nearly the same time, so this single run cannot distinguish profile activation from first-run DRS creation as the immediate trigger.

There is no evidence of a driver crash or timeout: the Windows System log contains no matching Display, `nvlddmkm`, Dxg, Kernel-PnP, Kernel-Power, graphics, monitor, or NVIDIA event from 13:33:55 through 13:34:40. No persistent problem was observed.

A second launch with both profiles already present would separate the two explanations: if the blink repeats, activation of the Fixed Refresh profile is the likely cause; if it does not, creation and saving of `Godot Engine` was the likely one-time cause.

## Conclusion

The original failure is now narrowly classified:

- NVIDIA App can create a working custom Godot profile and apply Fixed Refresh when no conflicting Godot profile exists at manual-add time.
- NVIDIA App cannot reliably discover or adopt the separately named `Godot Engine` profile that Godot creates first; that is the profile-visibility/adoption defect demonstrated earlier.
- Once NVIDIA App successfully creates its own exact-path profile, Godot's later basename-only profile does not defeat it for this executable path.

The workaround through NVIDIA App succeeded in this controlled clean-baseline test.
