# G815LR-IS97 external-display port specifications

Research date: 2026-08-29

Machine: ASUS ROG Strix G18 (2025), `G815LR-IS97`

This note separates ASUS's explicit SKU/family specifications, the capabilities of the interface standards, and properties actually observed on the investigated machine. Those categories are not interchangeable.

## ASUS-documented ports

The exact US SKU page identifies `G815LR-IS97` and lists this I/O configuration for the G815LR variants:

| Port | ASUS specification |
|---|---|
| Native HDMI | `1x HDMI 2.1 FRL` |
| USB-C | `2x Thunderbolt 5 with support for DisplayPort / power delivery / G-SYNC (data speed up to 120Gbps)` |

All three connectors are on the notebook's left edge. The family manual describes the HDMI connector as an audio/video output and describes each of the two Thunderbolt 5 connectors separately as a USB Type-C-compatible port with display output and Power Delivery.

Sources:

- [ASUS exact G815LR-IS97 configuration page](https://rog.asus.com/us/laptops/rog-strix/rog-strix-g18-2025/spec/?config=90NR0LC1-M00460)
- [ASUS 2025 G815 family user manual](https://dlcdnets.asus.com/pub/ASUS/GamingNB/G815LW/E24196_G815L_J_EM.pdf?model=ROG+Strix+G18%282025%29+G815)
- [ASUS 2025 G815 product page](https://rog.asus.com/us/laptops/rog-strix/rog-strix-g18-2025/)

## Native HDMI port

### What is established

- One standard, full-size HDMI output.
- ASUS labels it **HDMI 2.1 FRL**. `FRL` means Fixed Rate Link, the newer HDMI signaling method used for modes above the older TMDS ceiling. The port remains backward compatible with TMDS HDMI devices.
- It carries HDMI video and audio. It is not a USB/data/charging port.
- On this machine, Windows enumerates the native-HDMI PA278QGV as a separate HDMI target owned by the NVIDIA GeForce RTX 5070 Ti Laptop GPU.
- The investigated PA has operated through it at 2560x1440 and 119.998 Hz.

### What ASUS does not publish

ASUS does not state any of the following for the exact port:

- the implemented FRL mode or lane rate (`FRL3` through `FRL6`);
- whether its maximum raw link rate is 24, 32, 40, or 48 Gbit/s;
- a per-port maximum resolution/refresh/chroma/bit-depth matrix;
- whether DSC is exposed through this laptop port; or
- an explicit G-SYNC guarantee for the HDMI connector.

Consequently, `HDMI 2.1 FRL` must not be rewritten as `48 Gbit/s`, `4K120`, or `8K60` as a machine-specific guarantee. HDMI 2.1 defines capabilities up to 48 Gbit/s, but an implementation may expose a lower FRL rate or only a subset of optional features. The exact negotiated ceiling would require an HDMI protocol analyzer, a driver/API that reports the active FRL mode, or a mode-by-mode validation with a suitable sink.

The NVIDIA driver has reported the PA's HDMI target as VRR-possible in some captures, including one capture where the monitor OSD said MediaSync was off. That public bit is therefore not sufficient to certify HDMI G-SYNC behavior. ASUS explicitly attaches the G-SYNC claim to the two Thunderbolt/DisplayPort ports, not to HDMI, in its I/O table.

HDMI standard reference:

- [HDMI Licensing Administrator: HDMI 2.1b overview](https://www.hdmi.org/spec/hdmi2_1/index.aspx)

## Two Thunderbolt 5 USB-C ports

### What is established for each connector

- USB Type-C receptacle.
- Thunderbolt 5 and USB Type-C compatibility.
- DisplayPort output.
- NVIDIA G-SYNC support, explicitly stated by ASUS.
- USB Power Delivery input for charging the notebook.
- ASUS-advertised data rate: **up to 120 Gbit/s** for a Thunderbolt 5 device/connection.
- Backward compatibility with earlier Thunderbolt and USB devices follows the Thunderbolt 5 standard.

The ASUS manual says to use a `20 V / 5 A` source for USB-PD charging. ASUS's G815 product page caps Type-C charging at **100 W**. This is power accepted by the notebook, not a published 100-W source capability for peripherals. The two ports do not combine into 200 W.

### Meaning of the 120-Gbit/s number

Thunderbolt 5 normally provides **80 Gbit/s in each direction**. Bandwidth Boost can rebalance one connection for display-heavy traffic to **120 Gbit/s transmit and 40 Gbit/s receive**. It is not 120 Gbit/s simultaneously in both directions, and it is not an application payload rate.

ASUS does not publish a guaranteed aggregate bandwidth when both receptacles are heavily loaded at once. The investigated machine enumerates one Intel USB4 2.0 host-router complex for the pair:

```text
PCI 8086:5780 Intel switch complex
  -> PCI 8086:5781 USB4(TM) Host Router (Microsoft)
     -> USB4 Root Router (2.0)
  -> PCI 8086:5782 USB 3.20 xHCI controller
```

This proves that the notebook does not expose two independent USB4 root-router domains. It is consistent with Intel's dual-port Barlow Ridge host-controller design. Intel's public JHL9580 specification describes a dual-port Thunderbolt 5 host controller with a PCIe Gen 4 x4 host interface and DisplayPort 2.1 tunnel/re-drive support. ASUS does not name the controller IC in its public documentation, so `JHL9580` should be treated as a strong platform inference rather than an ASUS-confirmed bill-of-materials fact.

Intel references:

- [Intel Thunderbolt 5 technology brief](https://www.intel.com/content/dam/www/central-libraries/us/en/documents/2023-09/thunderbolt-5-technology-brief.pdf)
- [Intel JHL9580 Thunderbolt 5 controller specifications](https://www.intel.com/content/www/us/en/products/sku/225921/intel-jhl9580-thunderbolt-5-controller/specifications.html)

### DisplayPort version and display limits

Thunderbolt 5 as a standard supports DisplayPort 2.1 transport, and the likely JHL9580 controller supports DP 2.1 tunnel/re-drive. The RTX 5070 Ti Laptop GPU silicon supports DisplayPort 2.1b. These facts do **not** establish that ASUS wired every DP 2.1 rate to both connectors or that both can simultaneously drive every mode supported by the standards.

NVIDIA's own laptop-GPU comparison tells buyers to check with the laptop manufacturer for the capabilities implemented by a particular notebook:

- [NVIDIA GeForce laptop GPU comparison](https://www.nvidia.com/en-us/geforce/laptops/compare/)

ASUS only promises `support for DisplayPort`; it does not publish:

- a DisplayPort revision for the G815LR ports;
- UHBR10, UHBR13.5, or UHBR20 support;
- the number of DisplayPort source streams routed into the controller;
- a per-port or simultaneous maximum mode table; or
- whether the two receptacles have fully independent display bandwidth.

For the PA278QGV connection actually captured in this investigation, NVAPI reported:

```text
DisplayPort external
4 lanes
HBR2, 5.4 Gbit/s per lane
8 bits per component
2560x1440 at 119.998 Hz
MST dynamic/root flags: 0/0
```

That is a 21.6-Gbit/s raw DP link (17.28 Gbit/s payload under HBR2's 8b/10b coding). It is the negotiated requirement/capability of this monitor, cable, and mode—not the maximum of the laptop port.

Both PAs, when attached through the two USB-C-to-DisplayPort connections, have appeared as distinct NVIDIA external-DP connector instances on the RTX 5070 Ti. They are not MST children. The PA278QGV is a DisplayPort/HDMI monitor and exposes no downstream USB4 router, so this use case is a DisplayPort output through a Thunderbolt-5-capable USB-C connector, not an 80/120-Gbit/s Thunderbolt data tunnel terminating at the monitor.

## Relevance to the G-SYNC investigation

The native HDMI connection and USB-C/DisplayPort connections are materially different paths:

- native HDMI is an NVIDIA HDMI target/transmitter path;
- each connected USB-C-to-DP PA is an NVIDIA external DisplayPort target;
- the two USB-C receptacles sit in one USB4/TB5 controller/root-router domain; and
- two simultaneous TB5/DP targets select a different display-engine/topology allocation from one DP target plus one native-HDMI target.

This supports the investigation's empirically proven route discriminator without pretending to identify an unpublished failing component. The current evidence proves that `two external DP targets through the dual-port TB5 side` is the susceptible topology and `one external DP target plus native HDMI` is the stable topology. It does not prove whether the defect is specifically in the NVIDIA display engine, the DP source routing into the Intel controller, the controller firmware, Type-C mux/retimer programming, or coordination among those components.

## Concise specification table

| Property | Native HDMI | Each TB5 USB-C |
|---|---|---|
| Count | 1 | 2 total |
| Physical connector | Full-size HDMI | USB Type-C |
| ASUS protocol label | HDMI 2.1 FRL | Thunderbolt 5 |
| Display transport | HDMI audio/video | DisplayPort; TB5 also supports DP 2.1 transport at the standard/controller level |
| G-SYNC | Not explicitly promised in ASUS I/O table | Explicitly supported by ASUS |
| Advertised link/data rate | Exact FRL rate not published | 80 Gbit/s bidirectional; up to 120 Tx / 40 Rx with Bandwidth Boost |
| Notebook charging input | No | USB PD, 20 V / 5 A, up to 100 W |
| Published peripheral power output | Not applicable | Not specified by ASUS |
| Published maximum display mode | Not specified | Not specified |
| Local routing observation | Direct NVIDIA HDMI target | Direct NVIDIA external-DP targets; separate connector instances, not MST |
| Shared-resource fact | Separate HDMI route | One USB4 root-router/controller domain for the pair; simultaneous aggregate limit unpublished |
