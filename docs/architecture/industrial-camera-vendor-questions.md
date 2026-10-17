# COE-U / OECS — technical clarification draft

18 October 2026. Prepared by Codex for ICM-04d. **Draft only; not sent.**
Internal decision: [trigger correlation](industrial-camera-trigger-correlation.md).

Subject: COE-050-M-USB-050-IR-C — clean Mono8 metadata and software trigger readiness

We are integrating one COE-U camera through the OECS C API. We need a documented
way to associate each accepted software trigger with exactly one new frame while
preserving all pixels in an explicitly defined measurement region. The full sensor
is 2448×2048; excluding sensor row 0 and using a 2448×2047 image with sensor origin
(0,1) is now acceptable to our product owner. Please answer for this device/firmware:

**Clarified scope:** we need one supported correlation strategy, not all of the
features below simultaneously. A documented quiescent stream barrier plus exclusive
single-outstanding-trigger operation could suffice without a dedicated ready node
or an accepted-trigger counter. Alternatively, a documented exposure timestamp
fence or actual-counter strategy may work. Our current software implements only
the counter strategy; it is not a requirement imposed by the camera standard.

| Item | Observed value |
| --- | --- |
| Manufacturer / model | Opto Engineering Srl / COE-050-M-USB-050-IR-C |
| Serial | 02A2220000627 |
| DeviceVersion | V1.4.8 190411 262291 |
| DeviceFirmwareVersion | V1.4.8 190411,19012902 |
| C API GetSDKVersion | 0x03050200 |
| Actual raster | 2448 × 2048 Mono8 |
| TriggerSelector supported values | FrameBurstStart only (6) |
| Diagnostic configuration | Continuous acquisition, burst count 1, Software, cache disabled |

1. **Metadata layout / optional out-of-band path.** Enabling FrameSpecInfo Timestamp,
   Framecounter and ExtTriggerCount produces counters in `MV_FRAME_OUT_INFO_EX`,
   but also replaces image bytes: offsets 4–7 match nFrameCounter and 8–11 match
   nTriggerIndex in 100/100 samples. With metadata disabled, those counters were
   zero in earlier diagnostics. Is there a supported out-of-band/chunk/event API
   that supplies frame and accepted software trigger counters with a fully intact
   Mono8 raster? Please specify the exact feature/API, supported firmware and
   metadata layout. Alternatively, please confirm the exact embedded layout when
   only Timestamp, Framecounter and ExtTriggerCount are enabled, and that later
   rows are intact. We tested 12 frames with those fields and explicitly excluded
   row 0, retaining the sensor origin. Out-of-band metadata is therefore optional;
   replacing or interpolating missing sensor values is still not acceptable.
2. **Ready and acceptance.** AcquisitionStatusSelector is missing and
   AcquisitionStatus unavailable. The XML makes TriggerSoftware access depend
   only on TriggerMode and TriggerSource. What supported signal/API proves
   FrameBurstStart readiness? What happens to software commands during exposure
   or readout with cache disabled: rejected, dropped or deferred? Does MV_OK mean
   the device accepted the command? What counter/event identifies that acceptance?
3. **Quiescence barrier.** Does MV_CC_StopGrabbing wait for an active exposure and
   all pending USB transfers to finish or cancel them? XML describes AcquisitionStop
   as stopping at the end of the current frame. Does its API return wait for that
   point? Please provide the supported stop/clear/start sequence and completion
   signal that guarantee no pre-barrier exposure can deliver a post-barrier buffer.
   With that barrier established, On/Software, FrameBurstStart, burst=1,
   cache=false and one outstanding command, can the SDK return a duplicate or
   unsolicited frame? If a command is ignored while busy, can it execute later?
   We can fault the session on any timeout/cancel/command error and require a new
   barrier before another request. A ready node is optional for this strategy.
   New exact-device observations at 5 ms exposure / zero TriggerDelay: clearing
   immediately after a software command still delivered that command's frame
   without another command in 3/3 trials. Clear/Stop/Start delivered none in six
   350 ms observation windows (three immediate, three after 150 ms). This does
   not prove quiescence. Which documented completion condition closes this gap?
4. **Counter epoch.** What resets nFrameCounter and nTriggerIndex: acquisition
   start/stop, ClearImageBuffer, close/open, device reset or power cycle? How can we
   read a baseline before the first accepted trigger without modifying image pixels?
   Are counts modulo 2^32? Does nTriggerIndex count software commands, accepted
   triggers or external edges? Explain rejected commands, burst=1 and cache=false.
   In all six Clear/Stop/Start trials on the same open handle, the first subsequent
   command returned nFrameCounter=1, nTriggerIndex=1 and nFrameNum=0. Please confirm
   the reset point and whether any delayed pre-stop command/frame can survive it.
5. **Timestamp alternative.** DeviceTimestamp is described in ns; the embedded
   timestamp in manual §3.10.2 uses second/cycle/offset fields. What event, units,
   rollover and reset rules apply to nDevTimeStampHigh/Low? Is DeviceTimestampLatch
   in the same clock domain, and is its completion ordered with exposure start and
   software trigger acceptance? Please provide a supported synchronization sequence.

   **New exact-device measurements:** DeviceTimestampLatch succeeds and advances.
   With all eight supported FrameSpecInfo banks disabled, 24 frames still have a
   nonzero SDK device timestamp. Over this short run, DeviceTimestamp advanced at
   approximately 148,502,301 units per host monotonic second, while the combined
   nDevTimeStampHigh/Low advanced at approximately 1,250,014,323 units per second.
   DeviceTimestampIncrement returned 0. One example is latch-before-command
   `714446788598`, frame timestamp `6013832292930`, latch-after-receive
   `714455392146`. These are measurements, not proposed conversion constants.
   In 12 trials we waited 150 ms after a trigger, latched, and retrieved that
   trigger's buffer without sending any new trigger; all 12 frame timestamps were
   numerically greater than the intervening latch. Please clarify the clocks,
   scaling, origins and event semantics; raw numerical comparison is unusable.

The reference manual is [COE-U User Manual V1.0](https://www.opto-e.cn/repository/_shared/COE-U-series-USB3.0-Camera-User-Manual-20190412.pdf),
especially §3.6 and §3.10.2. Please provide documentation or a minimal C API example
for this exact firmware, or identify an officially supported firmware/SDK path.
We have not changed firmware or saved a UserSet. Native production trigger support
remains disabled pending these guarantees.

The [2023 manual](https://www.opto-e.cn/repository/_shared/COE-U-series-USB3.0-Camera-User-Manual_1.2.0_20230428.pdf)
also describes embedded information (§3.11.2); we are not assuming its event
features are implemented on this 2019 firmware. No firmware upgrade was attempted.
