# NVIDIA App omits the Godot DRS profile

Investigation date: 2026-08-28 (PDT)

Environment:

- NVIDIA App: `11.0.8.299`
- Driver: `616.56` (`r616_41`)
- GPU: GeForce RTX 5070 Ti Laptop GPU
- Target: `C:\Users\k\Program\Godot_v4.6.3-stable_win64.exe\Godot_v4.6.3-stable_win64.exe`

## Conclusion

This is a confirmed NVIDIA App profile-discovery/adoption defect, not a missing driver profile and not an NVIDIA Profile Inspector display artifact.

The `Godot Engine` profile exists in NVIDIA's live Driver Settings (DRS) database. NVIDIA Control Panel and NVIDIA Profile Inspector expose that database entry, but NVIDIA App's Program Settings page does not enumerate DRS profiles. It starts with a separate NVIDIA App application catalog and only resolves a DRS profile for catalog entries that pass its program-list filter.

Godot is absent from that application catalog, so its valid DRS profile is invisible in NVIDIA App. NVIDIA App's manual-add path is also unable to adopt this existing association: for a manually added executable without a cached profile name, the app first tries to create a new profile named after the executable. That conflicts with the executable already associated with the differently named `Godot Engine` profile.

## What the screenshots establish

- [`godot-profile-nvcp.png`](screenshots/godot-profile-nvcp.png): NVIDIA Control Panel lists `Godot Engine (godot_v4.6.3-st...)` under Manage 3D Settings > Program Settings.
- [`godot-profile-npi.png`](screenshots/godot-profile-npi.png): NVIDIA Profile Inspector independently shows the user profile `Godot Engine`, application `godot_v4.6.3-stable_win64.exe`, and 7,958 total profiles.
- [`godot-profile-missing-nvidia-app.png`](screenshots/godot-profile-missing-nvidia-app.png): NVIDIA App shows `1/1 Programs`, with only Age of Empires IV.

The screenshots were captured at the same investigation state. Their SHA-256 hashes are:

| File | SHA-256 |
| --- | --- |
| `godot-profile-nvcp.png` | `5524B2127284D6892992ED9D73B87F634120AE626FF843FF2A98EAD61A6A72DF` |
| `godot-profile-npi.png` | `30D91B763F08BC97F3EAF9843E3692E31AC1A035D996186A627206BAC47E61B6` |
| `godot-profile-missing-nvidia-app.png` | `B8D61F8327442E5BD03429A7679986917C8E91EBF044524EFD8C289BCE91EF03` |

## The live driver profile still exists

A fresh read-only NVAPI query at 13:06 PDT returned:

- `FindApplicationByName(full path)`: success
- `FindApplicationByName(basename)`: success
- profile: `Godot Engine`
- profile type: user-defined (`predefined=0`)
- application: `godot_v4.6.3-stable_win64.exe`
- total DRS profile count: 7,958

The profile contains only:

| Setting | ID | Value |
| --- | --- | --- |
| G-SYNC mode | `0x1194F158` | `1` (`fullscreen only`) |
| Threaded optimization | `0x20C1221E` | `2` (`disabled`) |

It does **not** contain the per-application G-SYNC override `0x10A879CF`; that setting is inherited from the global profile as `0` (`allow`). The current profile therefore does not disable G-SYNC for Godot. This is separate from the NVIDIA App visibility defect.

The raw result is in [`godot-after-project-launch-query-output.txt`](godot-after-project-launch-query-output.txt).

## Why NVIDIA App shows only one program

NVIDIA App has a separate application database at:

`C:\Users\k\AppData\Local\NVIDIA Corporation\NVIDIA App\NvBackend\ApplicationStorage.json`

At the time of the screenshot, it contained only:

1. Age of Empires IV (`regularOpsSupported=true`)
2. Steam (`regularOpsSupported=false`)

It contained no Godot entry and had not changed since 12:09:47 PDT.

The installed frontend in `C:\Program Files\NVIDIA Corporation\NVIDIA App\www\main.543f77ff0dcbc629.js` shows the selection logic:

1. `ScanApisService.getAllScannedApps()` gets entries from the NVIDIA App application database.
2. `AppListService.getLocalUpdatedApps()` filters those entries through `getOptimizableAppsFromCache()`.
3. Its `optimizable` predicate accepts either a supported NVIDIA catalog entry or a manually added entry.
4. `FingerprintAndDRSProgramsService` uses that filtered stream to populate Program Settings.

Age of Empires IV passes the filter. Steam does not. Godot never reaches the filter because it is absent from the application database. This exactly produces the UI's `1/1 Programs` result.

There is no code path here that enumerates the 7,958 DRS profiles and adds their applications to the NVIDIA App list.

## NVIDIA App nevertheless knows about the DRS profile

The backend log proves that NVIDIA App can query DRS independently of its application catalog. While Godot was running, it logged:

```text
ChooseClassificationResult: r.application is NULL. Using DRS classificationResult for drsData.profileName=Godot Engine
```

This occurred at 12:49:23 and again at 12:50:23. In other words:

- `r.application is NULL`: there is no NVIDIA App application-catalog record.
- `drsData.profileName=Godot Engine`: the backend successfully found the existing driver profile.

NVIDIA App uses that fallback for process classification/telemetry, but not to populate Graphics > Program Settings.

## Why manual add does not adopt the existing profile

The installed NVIDIA App frontend implements the manual path in this order:

1. Add a manual application record.
2. If it has no cached DRS profile name, call `createNewProfile(fullExePath, shortName)`.
3. Only after profile creation succeeds, call `getProfileNameFromExe()`.

For this executable, the proposed profile name is `Godot_v4.6.3-stable_win64.exe`, while its existing driver profile is named `Godot Engine`.

The earlier NVIDIA App log captured during the add attempt agreed with this control flow:

- 12:09:33: `NvAPI_DRS_CreateApplication` for the exact Godot path failed with `-167`.
- 12:09:44: `NvAPI_DRS_CreateProfile(Godot_v4.6.3-stable_win64.exe)` failed with `-164`.

NVIDIA's NVAPI definitions identify these as:

- `-167`: `NVAPI_EXECUTABLE_ALREADY_IN_USE` — the application already exists in another profile.
- `-164`: `NVAPI_PROFILE_NAME_IN_USE` — the profile name is duplicated.

The manual catalog entry was later removed, and `ApplicationStorage.json` currently has no Godot entry. The relevant log rotated after the NVIDIA App restart, but these exact events and line numbers were captured before rotation during this investigation.

The design flaw is the ordering: the app already has a `GetProfileNameFromExe` API that resolves `Godot Engine`, but the manual-add path calls it only after attempting to create a new profile. An already-associated executable therefore enters the collision path instead of the adoption path.

## Scope of the defect

The evidence supports this narrower statement:

> NVIDIA App 11.0.8.299 does not reliably display or adopt an existing DRS application profile when the executable is absent from NVIDIA App's own application catalog, especially when the profile was created externally and its profile name differs from the executable name.

It does not establish that every custom profile is invisible. A profile that NVIDIA App itself successfully creates can be represented by a manual catalog record and pass the UI filter.

## Why this is a migration-blocking issue

NVIDIA stated on 2026-05-26 that:

- all actively supported NVIDIA Control Panel features for GeForce users have moved to NVIDIA App;
- classic NVIDIA Control Panel is retiring and will receive no features, fixes, or other changes; and
- Graphics > Program Settings replaces Control Panel's Manage 3D Settings.

Sources:

- [NVIDIA: Control GPU Settings From The NVIDIA App](https://www.nvidia.com/en-in/geforce/news/007-first-light-geforce-game-ready-driver/#control-gpu-settings-from-the-nvidia-app)
- [NVIDIA App feature overview](https://www.nvidia.com/en-us/geforce/news/nvidia-app-download-and-features/#graphics)
- [NVIDIA App 11.0.8 release highlights](https://www.nvidia.com/en-eu/software/nvidia-app/release-highlights/)
- [NVAPI status definitions](https://docs.nvidia.com/nvapi/group__nvapistatus.html)

The observed behavior contradicts the replacement claim for this valid user-defined DRS profile: the retiring official UI can see it, but its stated official replacement cannot.

## Minimal NVIDIA bug report

### Reproduction

1. Associate an executable with a user-defined DRS profile whose name differs from the executable name. Godot 4.6.3 does this itself by creating `Godot Engine` for `godot_v4.6.3-stable_win64.exe`.
2. Confirm the profile exists in NVIDIA Control Panel or through read-only NVAPI DRS enumeration.
3. Open NVIDIA App > Graphics > Program Settings.
4. Observe that the profile is absent when the executable is not in NVIDIA App's application catalog.
5. Use NVIDIA App's `Add program` action and select the same executable.
6. Observe that NVIDIA App attempts to create a new profile instead of adopting the existing association, resulting in an executable/profile collision.

### Expected

NVIDIA App should either enumerate existing user DRS application profiles or resolve the selected executable with `GetProfileNameFromExe` before trying to create a profile. It should then display and edit the existing `Godot Engine` profile.

### Actual

The existing profile remains invisible, and manual add takes a create-first path that cannot adopt the existing association.

## Safety

All checks in this phase were read-only. No NVIDIA setting or application record was changed.
