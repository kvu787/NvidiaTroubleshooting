# Post-reboot clone and connector-route validation

Captured: 2026-08-29 01:35 PDT, after the user's functional tests.

## User observations

The user rebooted with the 2560x1440 internal-eDP-plus-HDMI clone workaround configured.

After reboot:

- the mixed-route clone continued to provide smooth per-application behavior;
- the first ordinary Godot 4.6.3 editor start caused one monitor blink;
- successive Godot editor open/close cycles before another reboot were smooth and produced no further blink; and
- later G-SYNC behavior remained functional.

The user also tested the same general two-desktop/three-target clone concept with both external PA278QGV monitors connected through the two TB5/USB-C DisplayPort outputs. The G-SYNC problems returned. Moving one external PA back to the laptop's native HDMI output restored smooth behavior.

This validates one reboot for the mixed-route workaround while narrowing its remaining defect to one observed cold Godot transition after that reboot. It is consistent with a once-per-boot effect but does not yet establish that cadence across multiple reboots. It also proves that clone mode and active internal eDP are not sufficient by themselves: the physical external-output route remains decisive.

## Live topology after returning to the working route

The read-only post-test capture reports three active NVIDIA target paths and two Windows source IDs:

```text
source id 0, \\.\DISPLAY1, source 2560x1440 at (2560,0)
  -> internal target 8449, embedded DisplayPort, 2560x1600 at about 240 Hz
  -> HDMI PA target 8448 clone-path representation, 2560x1440 at 119.998 Hz

source id 2, \\.\DISPLAY3, source 2560x1440 at (0,0)
  -> external DP PA target 8452, 2560x1440 at 119.998 Hz
```

The internal panel and native-HDMI PA share source ID 0, GDI source, desktop position, and 1440p source mode. The TB5/DisplayPort PA remains the separate primary desktop. Windows therefore restored the intended two-source/three-target clone structure after reboot.

All three paths remain owned by the RTX 5070 Ti Laptop GPU. The external DisplayPort target is connector instance 1 and is not MST.

## NVIDIA live state

Driver: 596.49, branch `r596_25`.

```text
target 8448, HDMI PA:     VRR possible=1, displayInVrrMode=0
target 8449, internal:    VRR possible=1, displayInVrrMode=0
target 8452, primary DP:  VRR possible=1, displayInVrrMode=1
```

This was captured after the user's application tests rather than as a clean pre-application baseline. The clone targets being outside live VRR mode while the primary DP target remains in it is not treated as a failure; the user's functional G-SYNC result is authoritative. All queries succeeded.

## Persistent profile state

The exhaustive read-only audit remains structurally stable:

```text
Total DRS profiles: 7837
Matching profile: Godot Engine
Associations:
  godot_v4.4.1-stable_win64.exe
  godot_v4.6.3-stable_win64.exe
Enumeration failures: 0
```

Current DRS files after the Godot repetitions:

```text
nvdrsdb1.bin
  last write: 2026-08-29 01:29:33 PDT
  SHA-256: 8D9F05343088E780BDBF1E5A2AF434559BC8357C88A68ED43EEA7AFBD717AF30

nvdrsdb0.bin
  last write: 2026-08-29 01:30:01 PDT
  SHA-256: F25BBAD98000E57DFB9171DE5CB1468498CB7F2BDD7B0A004D66B6151C842E26

nvdrssel.bin
  SHA-256: 6E340B9CFFB37A989CA544E6BB780A2C78901D3FB33738768511A30617AFA01D
```

The selector and profile/application structure remain stable. The database writes are the expected Godot native-OpenGL saves.

## Newly surfaced black-screen live diagnostics

At approximately 01:28, Windows Error Reporting surfaced a backlog of `LiveKernelEvent` reports. The newest report groups were created at approximately 01:26:31 and 01:28:24 and contain:

- code `0x1A8`, `VIDEO_DXGKRNL_BLACK_SCREEN_LIVEDUMP`; and
- code `0x1B8`, `VIDEO_MINIPORT_BLACK_SCREEN_LIVEDUMP`, with two miniport dump records per group.

Microsoft documents these as black-screen **live dumps**, not fatal bugchecks. They are not conventional TDR codes such as `0x117` or `0x141`, and no System event 4101 was found in the queried window.

The WER signatures record parameter 1 as `1`. Microsoft's documentation maps source value `0x1` to the black-screen hotkey. The user has been asked whether `Win+Ctrl+Shift+B` or another utility shortcut was invoked. Until that is resolved, the source attribution must not be assumed to be an autonomous driver watchdog.

The 01:26/01:28 dump groups precede the current DRS writes at 01:29/01:30, so they are not evidence that those later Godot saves created the dumps. They may correspond to the dual-TB5/topology-switch tests, but exact action-to-dump correlation is unavailable from the current record.

Many older dump filenames correspond approximately to earlier investigation periods. This supersedes the earlier blanket statement that no LiveKernelReports accompanied the investigation: WER had not exposed this backlog in the earlier captured event queries. It does not establish that every observed blink created a dump.

Official descriptions:

- <https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/bug-check-0x1a8--video-dxgkrnl-black-screen-livedump>
- <https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/bug-check-0x1b8--video-miniport-black-screen-livedump>

## Revised causal implications

### Established

- The native-HDMI clone workaround and its two-source topology persist across reboot.
- One cold Godot blink occurred after the tested reboot, but it did not poison later VRR and did not recur during that boot.
- A blink is therefore not equivalent to sticky VRR failure.
- Active/cloned eDP is not sufficient for arbitrary two-external configurations.
- Two external TB5/DisplayPort paths remain susceptible even when the internal target is active and cloned.
- Routing one external PA through native HDMI selects a stable allocation across reboot and repeated editor transitions.
- Logical desktop count, clone mode, active eDP, total active-target count, and simple aggregate bandwidth are individually insufficient explanations.

### Best current mechanism class

The result strengthens the route-dependent NVIDIA display-engine allocation model. Two simultaneously active external DisplayPort-over-USB-C paths select a susceptible Fixed Refresh/VRR transition path; moving one external target to native HDMI selects a stable transmitter/head/timing allocation.

The public evidence cannot identify a specific DP transmitter, PLL, clock domain, Type-C mux, retimer, or private driver object. “Native HDMI route plus its associated NVIDIA target/resource allocation” is the precise tested discriminator; “HDMI protocol itself fixes G-SYNC” would overstate the evidence.

### Remaining cold-blink ambiguity

The first-post-reboot Godot blink is consistent with lazy first-use programming of volatile display state. It is not yet isolated between:

- the first Fixed Refresh application transition of the boot;
- Godot-specific native OpenGL initialization;
- Godot's first post-boot DRS save/reload; or
- another cold presentation/display-path promotion.

A future `Unity first after reboot` versus `Godot first after reboot` A/B would distinguish a general cold Fixed Refresh transition from a Godot-specific first-use effect. This is optional because the current state is functionally usable and the sticky-loss workaround is already reboot-validated.
