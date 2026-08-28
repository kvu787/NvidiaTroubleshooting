# Confirmed bugs, limitations, risks, and workarounds

Investigation date: 2026-08-28 PDT

Scope: Godot 4.6.3 and NVIDIA App custom driver profiles. Unity is intentionally excluded because the investigation was narrowed to Godot.

Tested environment:

- NVIDIA App `11.0.8.299`
- NVIDIA driver `616.56` (`r616_41`)
- GeForce RTX 5070 Ti Laptop GPU
- Godot executable: `C:\Users\k\Program\Godot_v4.6.3-stable_win64.exe\Godot_v4.6.3-stable_win64.exe`

## Executive summary

The original runtime issue and the NVIDIA App management issue are both real, but distinct:

1. Godot's automatically created `Godot Engine` profile does not prevent G-SYNC from activating in the tested editor session. G-SYNC activation correlates with choppy mouse movement.
2. NVIDIA App cannot reliably discover, expose, or adopt that pre-existing DRS profile because its Program Settings UI is driven by a separate application catalog and its manual-add workflow tries to create before it resolves an existing association.
3. NVIDIA App can create a working Fixed Refresh profile if it does so before Godot creates `Godot Engine`.
4. Afterward, Godot creates a second basename-wide profile. Both can coexist, but NVIDIA App shows only its own exact-path profile.

The current machine is in the working state: retain the exact-path `Godot_v4.6.3-stable_win64.exe` profile with Fixed Refresh. Do not delete it merely because the duplicate `Godot Engine` profile looks untidy.

## Bug and workaround matrix

| ID | Status | Problem | User impact | Best workaround |
|---|---|---|---|---|
| G1 | Confirmed runtime defect | G-SYNC activates in the Godot editor despite the Godot-created profile | G-SYNC indicator appears and mouse movement is choppy | Exact-path NVIDIA profile with **Monitor Technology = Fixed Refresh** |
| G2 | Confirmed profile inadequacy | Godot's profile sets fullscreen-only mode but does not store the Fixed Refresh/application override | `Godot Engine` alone does not stop G-SYNC | Use the separate exact-path Fixed Refresh profile |
| N1 | Confirmed NVIDIA App defect | Program Settings does not enumerate valid DRS profiles absent from NVIDIA App's catalog | `Godot Engine` is invisible even though DRS, NVCP, and NPI see it | Create the profile in NVIDIA App first; otherwise use a DRS-aware legacy/third-party tool to clean/manage it |
| N2 | Confirmed NVIDIA App defect | Manual add uses create-before-adopt ordering | Existing `Godot Engine` association causes `-167`/`-164` collisions | Delete the conflicting Godot profile, then let NVIDIA App create its profile before launching Godot |
| N3 | Confirmed NVIDIA App limitation | One application-catalog row exposes only one DRS profile | With two valid profiles, NVIDIA App shows only its exact-path profile | Use NPI/NVCP for visibility of both; keep the working exact-path profile |
| N4 | Confirmed support/parity gap | NVIDIA App is the stated replacement but cannot fully manage valid DRS state | No complete currently supported official management path | Use the verified creation-order workaround; report N1-N3 to NVIDIA |
| I1 | Confirmed integration problem | Godot creates a second basename-wide profile even when an applicable exact-path profile exists | Duplicate profiles and split settings | Leave both when runtime is working; exact path wins for this installation |
| I2 | Confirmed architectural side effect | DRS selects one application profile; it does not merge both Godot profiles | Godot's threaded-optimization setting is not inherited into the exact-path profile | If an OpenGL issue appears, copy/set that setting on the selected exact-path profile using a DRS-aware tool |
| O1 | Unresolved side effect | Both monitors blink for two or three seconds at project startup | Brief visual interruption | Repeat launch with both profiles already present to separate Fixed Refresh activation from one-time profile creation |

## Detailed findings

### G1: G-SYNC activates in the editor and causes choppy pointer movement

Before the Fixed Refresh workaround, the user observed:

- the NVIDIA G-SYNC indicator active in the Godot editor;
- choppy mouse movement over the editor; and
- smooth movement over the Windows Start menu.

The behavior was application-specific and coincided with G-SYNC activation. After the exact-path Fixed Refresh profile was created, the indicator stayed absent and pointer movement was smooth.

#### Workaround

Create a profile for the exact editor executable and set:

```text
Monitor Technology = Fixed Refresh
```

The resulting DRS setting is `0x10A879CF = 4`. This workaround was verified at runtime.

### G2: Godot's automatically created profile is insufficient

Godot 4.6.3's OpenGL initialization creates `Godot Engine` with:

- basename association `godot_v4.6.3-stable_win64.exe`;
- `0x1194F158 = 1` (fullscreen-only G-SYNC mode); and
- `0x20C1221E = 2` (threaded optimization disabled).

It does not set `0x10A879CF = 4` (Fixed Refresh). Its effective application override remains global `allow`. The observed active indicator proves the profile did not suppress G-SYNC in this editor session.

#### Workaround

Do not rely on `Godot Engine` for G-SYNC suppression. Use the exact-path Fixed Refresh profile.

### N1: NVIDIA App does not discover existing DRS profiles

NPI and NVCP directly enumerate DRS and showed `Godot Engine`. NVIDIA App showed no Godot entry because its Program Settings list begins with NVIDIA App's scanned/manual application catalog rather than DRS enumeration.

NVIDIA App's backend could still resolve a running Godot process to `Godot Engine`, proving the backend knew the profile existed, but that DRS result was not used to populate Program Settings.

#### Workarounds

- Preventive: for a new Godot executable/path, add it to NVIDIA App and set Fixed Refresh before first opening a project.
- Recovery: remove the pre-existing conflicting profile with a DRS-aware tool, then create the exact-path profile through NVIDIA App.
- Inspection only: NPI or legacy NVCP can show the hidden profile, subject to their support limitations.

There is no verified method inside NVIDIA App to expose an arbitrary DRS-only profile after the fact.

### N2: NVIDIA App tries to create before adopting

When `Godot Engine` already existed, NVIDIA App's manual-add path:

1. added a manual catalog record;
2. tried to create a new DRS profile/application association; and
3. attempted existing-profile resolution only after creation.

The logs captured:

- `NVAPI_EXECUTABLE_ALREADY_IN_USE` (`-167`); and
- `NVAPI_PROFILE_NAME_IN_USE` (`-164`).

NVIDIA App already had a `GetProfileNameFromExe` capability that could resolve `Godot Engine`, but invoked it too late.

#### Verified recovery procedure

1. Close Godot and NVIDIA App.
2. Delete the conflicting Godot profile/association using NPI or legacy NVCP.
3. Confirm no Godot DRS association remains.
4. Open NVIDIA App.
5. Manually add the exact executable.
6. Set **Monitor Technology** to **Fixed Refresh**.
7. Close NVIDIA App and verify the DRS profile before launching Godot.
8. Launch Godot.

This produces the working exact-path profile before Godot can recreate its basename profile.

### N3: NVIDIA App cannot expose both profiles

After the successful sequence, DRS contains:

| Profile | Application rule |
|---|---|
| `Godot_v4.6.3-stable_win64.exe` | Exact full path |
| `Godot Engine` | Executable basename |

NPI and NVCP show both. NVIDIA App shows one Godot row because it has one Godot application-catalog record (`LocalId=963528738`) and binds that row to one DRS profile name.

#### Workaround

- Keep the NVIDIA App row pointed at the exact-path Fixed Refresh profile.
- Use NPI/NVCP only when it is necessary to inspect or change the second profile.
- Do not interpret NVIDIA App's one row as proof that only one DRS profile exists.

No complete NVIDIA App-only workaround exists for managing both profiles.

### N4: Official replacement has incomplete profile-management parity

NVIDIA App is NVIDIA's stated replacement for the relevant Control Panel functionality, but in this case:

- legacy NVCP sees both profiles;
- third-party NPI sees both profiles; and
- NVIDIA App cannot expose one of them or adopt it when it exists first.

This turns N1-N3 into a support-path problem, not just a cosmetic UI difference.

#### Workarounds

- Use the verified NVIDIA App-first creation order for this executable.
- Preserve the resulting working profile.
- Report the minimal reproduction in `nvidia-app-profile-visibility-findings.md` to NVIDIA.
- Use NVCP/NPI only with the explicit understanding that NVCP is retiring/unsupported for fixes and NPI is third-party.

### I1: Godot creates a duplicate, basename-wide profile

Godot checks for a profile by its chosen profile name and creates/updates a basename application association. It does not first adopt the already applicable exact-path profile. Consequently it creates `Godot Engine` even though the driver already has a working exact-path rule.

The basename rule is broader: another executable with the same filename in a different folder can fall back to it.

#### Workaround

For the current installation, leave both profiles. NVAPI's fully qualified lookup and NVIDIA's runtime logs show that the exact-path profile wins, so deleting the working profile would make the broader `Godot Engine` rule applicable again.

When installing a Godot version at a new path or under a new executable name, create the new exact-path Fixed Refresh profile before launching that version.

### I2: Settings from the two profiles are not merged

DRS chooses one application profile and overlays it on global settings. It does not combine the exact-path profile's Fixed Refresh with `Godot Engine`'s disabled threaded optimization.

No OpenGL problem was observed during the successful test, so this is a confirmed architectural side effect but not a demonstrated user-facing failure.

#### Conditional workaround

If threaded-optimization-related rendering problems appear, add `0x20C1221E = 2` to the selected exact-path profile using a DRS-aware interface that exposes the setting. Preserve `0x10A879CF = 4` while doing so. This conditional workaround has not been needed or runtime-tested in this investigation.

### O1: Two-to-three-second monitor blink remains unexplained

Both monitors blinked when the successful Fixed Refresh configuration was first tested. Two events occurred close together:

- the application activated a profile that changes G-SYNC/VRR to Fixed Refresh; and
- Godot created and saved its `Godot Engine` DRS profile.

There was no Windows System event indicating a display-driver crash, TDR, or hardware failure.

#### Diagnostic workaround

Launch the same project again now that both profiles already exist:

- blink repeats -> Fixed Refresh/profile activation is the leading cause;
- blink does not repeat -> one-time `Godot Engine` profile creation/save is the leading cause.

No mitigation is warranted unless the blink repeats or becomes disruptive.

## Current working configuration

The current DRS state is intentionally:

```text
Profile: Godot_v4.6.3-stable_win64.exe
Association: exact full path
G-SYNC application override: Fixed Refresh
Runtime result: no indicator, smooth mouse movement

Profile: Godot Engine
Association: basename only
Runtime result for the tested full path: not selected
```

Recommended action: retain this state.

## Things that are not bugs or workarounds

- `LocalId` is not a DRS profile identifier. It belongs to NVIDIA App's application catalog.
- Godot should not set a NVIDIA App `LocalId`; public NVAPI has no such field or operation.
- The two profiles are not merged, aliased, or duplicates at the DRS identity level. Their profile names and application match keys differ.
- Editing `ApplicationStorage.json` manually is not a supported workaround and would not reliably create/adopt the required DRS state.
- NVIDIA App's `2/2 Programs` count means two visible application rows, not two DRS profiles.

## Workaround checklist

### Keep the current fix working

- Do not remove `Godot_v4.6.3-stable_win64.exe` while the basename `Godot Engine` profile remains.
- Verify NVIDIA App continues to show **Fixed Refresh** for the exact executable.
- Recreate an exact-path profile when the Godot executable moves or its versioned filename changes.

### Recover a machine where Godot was launched first

- Remove the conflicting Godot DRS profile with NPI/NVCP.
- Verify a clean no-profile baseline.
- Add the executable in NVIDIA App and set Fixed Refresh.
- Verify the profile before launching Godot.
- Allow Godot to recreate `Godot Engine`; retain both afterward.

### Report upstream

- NVIDIA: report DRS discovery, create-before-adopt, and multi-profile visibility defects N1-N3.
- Godot: report that the basename-wide DRS creation does not account for an already applicable exact-path profile and that its G-SYNC mode setting does not suppress G-SYNC in the observed editor scenario.
