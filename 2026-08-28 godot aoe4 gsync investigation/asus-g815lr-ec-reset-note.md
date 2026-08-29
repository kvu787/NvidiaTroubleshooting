# ASUS ROG Strix G18 G815LR EC-reset note

Captured: 2026-08-29 PDT

## Machine and firmware

Read-only local registry state:

```text
SystemManufacturer: ASUSTeK COMPUTER INC.
SystemProductName:  ROG Strix G18 G815LR_G815LR
BIOSVersion:        G815LR.338
BIOSReleaseDate:    06/04/2026
```

ASUS's G815LR support page lists BIOS 338 as the current critical release dated 2026-07-03. The investigated machine is already on that BIOS version.

## Official ASUS procedure

ASUS documents an Embedded Controller/Real-Time Clock/hard-reset procedure for notebooks. It is similar in concept to Lenovo's long power-button reset, but ASUS specifies a 40-second hold by default rather than 30 seconds:

1. save work and shut the notebook down;
2. remove all external devices;
3. connect the ASUS power adapter;
4. hold the power button for 40 seconds; and
5. release it, then power the notebook on normally.

ASUS notes that some models use a 20-second design: after about 15 seconds, the power indicator flashes rapidly for approximately 3-5 seconds, and the button is released after the flashing stops. No G815LR-specific official document was found that replaces the general notebook procedure with a different duration, so the general 40-second instruction is the defensible default.

ASUS also warns that EC/RTC reset can cause full memory training at the next boot. The machine may remain on the ROG logo or show no display for several minutes; keep AC connected and do not force it off while training completes.

Official sources:

- <https://www.asus.com/us/support/faq/1050239/>
- <https://www.asus.com/us/support/faq/1042613/>
- <https://www.asus.com/us/supportonly/g815lr/helpdesk_bios/>

## Relevance to this G-SYNC defect

An EC reset may reinitialize firmware-controlled USB-C/TB power-delivery, mux, or retimer state, so it is a reasonable one-time experiment after the controlled application-placement A/B is complete. There is no current evidence that the EC owns the failed state observed here:

- the primary DP target alone changes from `displayInVrrMode=1` to `0`;
- topology, timing, DRS, and application associations remain unchanged; and
- a global G-SYNC off/on cycle restores the target immediately without an EC reset or reboot.

Therefore an EC reset should not yet be described as a fix. It also cannot be attributed cleanly if performed directly from a failed state, because the required shutdown and driver restart can independently clear volatile VRR state. Treat it as an optional low-level reset experiment, not the next causal test.
