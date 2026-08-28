# Identifiers for the two Godot NVIDIA profiles

## Short answer

The two profiles are distinguished at two levels:

1. Each DRS profile has a unique `profileName`.
2. Each profile contains an application match record. In this case the decisive application field, `appName`, differs: one stores a fully qualified path and the other stores only the executable basename.

There is no public persistent profile GUID or numeric profile ID in `NVDRS_PROFILE`. NVAPI uses an opaque `NvDRSProfileHandle` while a DRS session is open, but that is a transient API handle rather than a stable identifier to store or display.

## Exact values

| Layer/field | NVIDIA App-created profile | Godot-created profile |
|---|---|---|
| DRS `profileName` | `Godot_v4.6.3-stable_win64.exe` | `Godot Engine` |
| DRS `appName` | `c:/users/k/program/godot_v4.6.3-stable_win64.exe/godot_v4.6.3-stable_win64.exe` | `godot_v4.6.3-stable_win64.exe` |
| DRS `userFriendlyName` | `C:\Users\k\Program\Godot_v4.6.3-stable_win64.exe\Godot_v4.6.3-stable_win64.exe` | empty |
| `launcher` | empty | empty |
| `fileInFolder` | empty | empty |
| command-line qualification | disabled/empty | disabled/empty |
| `isPredefined` | `0` | `0` |
| NVIDIA App catalog `LocalId` | `963528738` | none |
| NVIDIA App catalog display name | `Godot_v4.6.3-stable_win64.exe` | none |

## Profile identity in DRS

The public `NVDRS_PROFILE` structure contains:

- `profileName`;
- GPU support flags;
- predefined/user-defined state;
- application count; and
- setting count.

It contains no persistent numeric ID or GUID. NVAPI exposes `NvAPI_DRS_FindProfileByName`, which returns an opaque `NvDRSProfileHandle` for the current session. The handle is then used to enumerate applications or read settings.

The name functions as the public persistent unique key:

- creating another profile with an existing name returns `NVAPI_PROFILE_NAME_IN_USE` (`-164`);
- looking it up uses `NvAPI_DRS_FindProfileByName`; and
- the public `NvAPI_DRS_SetProfileInfo` documentation says the name cannot be changed with that function.

Therefore `Godot_v4.6.3-stable_win64.exe` and `Godot Engine` are both human-readable labels and unique DRS profile identifiers. Their text is not merely decorative.

This conclusion is about the public NVAPI architecture. The proprietary binary database may have internal implementation keys, but NVAPI does not expose a stable one to callers.

## Application-association identity

A profile does not match a process because of its profile name. It matches through one or more `NVDRS_APPLICATION` records stored under the profile.

The structure contains:

- `appName`, documented as the application string/name;
- `userFriendlyName`, a display field;
- optional `launcher` and `fileInFolder` qualifiers; and
- optional command-line matching fields.

For these two records, every qualifier is empty, so `appName` alone distinguishes their match specifications:

```text
exact-path rule
  appName = c:/users/k/program/godot_v4.6.3-stable_win64.exe/godot_v4.6.3-stable_win64.exe

basename rule
  appName = godot_v4.6.3-stable_win64.exe
```

These are distinct DRS application keys even though they can both describe the same physical executable. The earlier `NVAPI_EXECUTABLE_ALREADY_IN_USE` (`-167`) result demonstrates that DRS prevents the same application association from being added to another profile, while the current coexistence demonstrates that a full-path association and basename association are considered different.

NVAPI documents that `NvAPI_DRS_FindApplicationByName` with a fully qualified path returns the profile the driver will apply when that application on that path runs. In the current database:

- the full-path lookup returns `Godot_v4.6.3-stable_win64.exe`; and
- the basename lookup returns `Godot Engine`.

Thus the application association, not the profile label, is what distinguishes runtime matching. For the tested full path, the exact-path association resolves to the Fixed Refresh profile.

## Human-readable identifiers and whether they are unique

### DRS `profileName`

Human-readable: yes.

Architecturally unique: yes, in the public DRS profile namespace. It is both display text and the persistent lookup key.

### DRS `appName`

Human-readable: usually. It can be a basename or normalized full path.

Architecturally significant: yes. It is an application match key, not merely a caption. Optional launcher, folder, and command-line fields can further qualify other application records. In these two records, only `appName` differs.

### DRS `userFriendlyName`

Human-readable: yes.

Architecturally unique: no. It is presentation metadata. The NVIDIA App-created entry stores the original Windows-style full path there; the Godot-created record leaves it empty.

### NVIDIA App `LocalId`

Human-readable: no.

Architecturally unique: it identifies NVIDIA App's local application-catalog record, not a DRS profile. `963528738` belongs to the manually added Godot row. `Godot Engine` has no NVIDIA App catalog row and therefore no corresponding `LocalId`.

### NVIDIA App display/short name

Human-readable: yes.

Architecturally unique: not as a DRS identifier. NVIDIA App happens to display `Godot_v4.6.3-stable_win64.exe`, the same text it chose for the DRS profile name, but these are fields in different data models.

### Setting IDs

Values such as `0x10A879CF` identify setting types such as the G-SYNC application override. They are reused across profiles and do not identify a profile.

## Why the repeated filename is confusing

The text `Godot_v4.6.3-stable_win64.exe` appears in several independent roles:

1. the unique `profileName` of the NVIDIA App-created DRS profile;
2. the final filename component of that profile's full-path `appName`;
3. the complete basename `appName` under the separate `Godot Engine` profile; and
4. NVIDIA App's catalog display name and short name.

These fields occupy different namespaces. A `profileName` equal to another profile's `appName` is not a collision. The actual profile-name keys remain `Godot_v4.6.3-stable_win64.exe` and `Godot Engine`, while the runtime application rules remain full path and basename.

## How the interfaces form their labels

- NVIDIA Profile Inspector primarily shows DRS `profileName`, so it displays the two unique profile keys directly.
- NVIDIA Control Panel combines profile/application presentation, producing labels resembling `profileName (appName or friendly path)`.
- NVIDIA App shows its catalog display name, then internally requests one DRS `profileName` for that catalog row. Its catalog `LocalId` is separate from DRS identity.

This explains both why NPI/NVCP can show two entries and why NVIDIA App shows only one row despite the two valid DRS profiles.

## Local NVAPI references

The relevant public definitions are in `C:\Users\k\Repository\External\PresentMon_2-5-1\IntelPresentMon\ControlLib\nvapi.h`:

- `NVDRS_APPLICATION_V4`: lines 21588-21602;
- `NVDRS_PROFILE_V1`: lines 21612-21621;
- profile creation and lookup: lines 21742-21758 and 21863-21880;
- application lookup semantics: lines 22042-22065; and
- duplicate-name/application statuses: lines 1130-1134.
