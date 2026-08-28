# Godot profile selection and NVIDIA App `LocalId`

## Runtime profile selection

NVIDIA's driver does not select a profile by its human-readable profile name, NVIDIA App `LocalId`, creator, creation time, or number of settings. It selects a DRS application association that matches the process image.

The two current application rules are:

```text
Godot_v4.6.3-stable_win64.exe profile
  appName = c:/users/k/program/godot_v4.6.3-stable_win64.exe/godot_v4.6.3-stable_win64.exe

Godot Engine profile
  appName = godot_v4.6.3-stable_win64.exe
```

When the editor at the tested path opens, the driver knows its fully qualified image path. The exact-path rule matches that specific file; the basename rule is the broader fallback. NVIDIA's public documentation for `NvAPI_DRS_FindApplicationByName` says that a fully qualified path returns the profile the driver will apply when the application on that path runs.

The current live lookup returns:

- full path -> `Godot_v4.6.3-stable_win64.exe`;
- basename -> `Godot Engine`.

NVIDIA App's backend independently observed the running full image path twice and resolved it to `Godot_v4.6.3-stable_win64.exe`. The absent G-SYNC indicator and smooth pointer movement agree with that selection.

The effective sequence is:

1. The process image is `C:\Users\k\Program\Godot_v4.6.3-stable_win64.exe\Godot_v4.6.3-stable_win64.exe`.
2. DRS evaluates application associations for that image.
3. The exact-path association selects the NVIDIA App-created profile.
4. That profile overlays its settings on the global profile, including Fixed Refresh.
5. DRS does not merge settings from the separate basename profile.

Consequently the `Godot Engine` profile's disabled OpenGL threaded-optimization setting is not layered into the exact-path profile. Settings absent from the selected application profile come from the global/default path, not from another matching application profile.

The practical matching cases are:

| Launched executable | Selected profile |
|---|---|
| Exact tested path, both profiles present | `Godot_v4.6.3-stable_win64.exe` |
| Exact tested path, exact-path profile removed | `Godot Engine` basename fallback |
| Different folder, same executable basename | `Godot Engine` basename fallback |
| Different executable basename | Neither Godot association, unless another rule matches |

Godot creates `Godot Engine` during its OpenGL initialization if that named profile is absent. On the successful test it created the basename profile after the exact-path profile already existed. That creation did not change which rule the full path resolves to, and subsequent launches continue to select the exact-path rule.

## What NVIDIA App `LocalId` identifies

`LocalId` is not a field in `NVDRS_PROFILE` or `NVDRS_APPLICATION`. It belongs to NVIDIA App's separate `ApplicationStorage.json` catalog.

For the manually added executable, NVIDIA App created this catalog identity:

```text
LocalId = 963528738
DisplayName = Godot_v4.6.3-stable_win64.exe
DetectedFiles = [exact executable path]
IsManuallyAdded = true
```

NVIDIA App uses the local ID to request catalog/application state and to associate frontend state with the application row. It separately resolves or caches a DRS `profileName`, then calls the profile-settings API with both values. The current log shows a request containing:

```text
ProfileName = Godot_v4.6.3-stable_win64.exe
ApplicationId = 963528738
CmsId = 0
```

Those are two identities from two systems:

- `ApplicationId`/`LocalId` identifies the NVIDIA App catalog record.
- `ProfileName` identifies the DRS profile.

They happen to be joined for this UI row, but neither is stored inside the other object.

## Why `Godot Engine` has no NVIDIA App `LocalId`

Godot creates its profile by calling public NVAPI DRS functions:

1. create or find the `Godot Engine` DRS profile;
2. add the basename application association;
3. set two DRS settings; and
4. save DRS.

None of those calls touches NVIDIA App's application catalog. NVAPI DRS has no `LocalId` field and exposes no operation for assigning one.

NVIDIA App also does not import every newly created DRS profile into its application catalog. Because the exact executable is already represented by local application ID `963528738`, NVIDIA App keeps one catalog row and binds it to the exact-path profile it created. It does not create a second catalog application merely because Godot later created another DRS profile with a broader association.

Thus `Godot Engine` lacks a `LocalId` because it is a DRS-only profile and there is no separate NVIDIA App catalog record for it.

## Is a profile shown if and only if it has a `LocalId`?

No. The premise conflates profiles with application rows.

NVIDIA App's Program Settings sidebar shows application-catalog rows, not a list of DRS profiles. In the current implementation, a visible row originates from an accepted catalog record and therefore has a `LocalId`, but possession of a `LocalId` is not sufficient for display and a DRS profile itself never possesses one.

Current counterexamples are decisive:

- `Godot Engine` is a valid DRS profile without an NVIDIA App catalog record. It is invisible in Program Settings.
- Steam has a valid NVIDIA App catalog record and `LocalId=1088017781`, but NVIDIA App filters it out of the current `2/2 Programs` list. Therefore `LocalId` is not sufficient for visibility.
- During the earlier failed manual-add attempt, NVIDIA App created a local Godot catalog record before DRS profile creation succeeded. Therefore a `LocalId` does not even guarantee that a corresponding new DRS profile was successfully created.

The closer description of the current UI predicate is:

```text
visible Program Settings row
  = NVIDIA App catalog record
  + passes NVIDIA App's program-list/filter logic
  + can be associated with settings/profile state the UI supports
```

It is not:

```text
visible profile = DRS profile has LocalId
```

## Should Godot have set a `LocalId`?

No.

- Godot cannot set one through public NVAPI DRS because the field does not exist there.
- `LocalId` is generated and owned by NVIDIA App's private application-catalog subsystem.
- Writing NVIDIA App's private storage directly would be unsupported, brittle, and incorrect.
- One application catalog record per executable is reasonable; creating a second application row merely to represent an alternate DRS profile would not fix NVIDIA App's one-row/one-profile UI model.

The responsibility for exposing the existing DRS state belongs to NVIDIA App. It should enumerate DRS application profiles or, when given an executable, adopt and expose all relevant associations instead of assuming one catalog row maps to only one manageable DRS profile.

Godot could independently reconsider its DRS behavior—for example, whether creating a basename-wide profile is appropriate when a full-path profile already exists—but that is a separate compatibility/design question. It should not manufacture an NVIDIA App `LocalId`, and doing so would not solve the underlying visibility architecture.

## Evidence references

- `godot-nvapp-postlaunch-query-output.txt`: both application rules and their settings.
- `godot-nvapp-postlaunch-log-excerpt.txt`: running full-path resolution to the exact profile.
- `godot-profile-identifiers.md`: DRS and NVIDIA App identity namespaces.
- `two-godot-profiles-ui-explanation.md`: UI data-model comparison.
- Local NVAPI header, `NvAPI_DRS_FindApplicationByName`: `C:\Users\k\Repository\External\PresentMon_2-5-1\IntelPresentMon\ControlLib\nvapi.h`, lines 22042-22065.
