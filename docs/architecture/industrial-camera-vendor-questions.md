# COE-U / OECS — technical clarification draft

17 October 2026. Prepared by Codex for ICM-04d. **Draft only; not sent.**
Internal decision: [trigger correlation](industrial-camera-trigger-correlation.md).

Subject: COE-050-M-USB-050-IR-C — clean Mono8 metadata and software trigger readiness

We are integrating one COE-U camera through the OECS C API. We need a documented
way to associate each accepted software trigger with exactly one new frame while
preserving every full-sensor Mono8 pixel. Please answer for this device/firmware:

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

1. **Metadata without replacing sensor pixels.** Enabling FrameSpecInfo Timestamp,
   Framecounter and ExtTriggerCount produces counters in `MV_FRAME_OUT_INFO_EX`,
   but also replaces image bytes: offsets 4–7 match nFrameCounter and 8–11 match
   nTriggerIndex in 100/100 samples. With metadata disabled, those counters were
   zero in earlier diagnostics. Is there a supported out-of-band/chunk/event API
   that supplies frame and accepted software trigger counters with a fully intact
   Mono8 raster? Please specify the exact feature/API, supported firmware and
   metadata layout. We cannot replace, interpolate or crop the affected pixels.
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
4. **Counter epoch.** What resets nFrameCounter and nTriggerIndex: acquisition
   start/stop, ClearImageBuffer, close/open, device reset or power cycle? How can we
   read a baseline before the first accepted trigger without modifying image pixels?
   Are counts modulo 2^32? Does nTriggerIndex count software commands, accepted
   triggers or external edges? Explain rejected commands, burst=1 and cache=false.
5. **Timestamp alternative.** DeviceTimestamp is described in ns; the embedded
   timestamp in manual §3.10.2 uses second/cycle/offset fields. What event, units,
   rollover and reset rules apply to nDevTimeStampHigh/Low? Is DeviceTimestampLatch
   in the same clock domain, and is its completion ordered with exposure start and
   software trigger acceptance? Please provide a supported synchronization sequence.

The reference manual is [COE-U User Manual V1.0](https://www.opto-e.cn/repository/_shared/COE-U-series-USB3.0-Camera-User-Manual-20190412.pdf),
especially §3.6 and §3.10.2. Please provide documentation or a minimal C API example
for this exact firmware, or identify an officially supported firmware/SDK path.
We have not changed firmware or saved a UserSet. Native production trigger support
remains disabled pending these guarantees.
