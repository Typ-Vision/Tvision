# Editor responsiveness probe (CCUX-23b-1)

`copy.vfproject*` is the unchanged three-file copy of the CCUX-23a valid fixture
from `build/ccux/ccux-23a/case/`. It contains two image-file sources and two
experimental calibration records (homography + uniform scale), with a remembered
source-scoped selection and no restored acquisition permission. It is not the
user's live project. Original evidence is never written by this probe.

`tests/editor_responsiveness_probe.cpp` runs the actual MainWindow Save,
Properties parameter signal, Add and ROI commit methods through the existing
in-process smoke convention. It uses the real header, without changing private
access. Narrow owner-thread counters in `EditorObservation.hpp` are opt-in and
do not cache or change product decisions.

After the README Release build, run from PowerShell with the README DLL PATH:

```powershell
$env:QT_QPA_PLATFORM = 'offscreen'
$env:VISIONFLOW_TEST_ARTIFACTS = 'C:/Projects/Tvision/build/ccux/ccux-23b-1/baseline'
& build/reference-ui/visionflow.exe --editor-responsiveness-probe
$LASTEXITCODE
```

The default is one discarded warmup plus 20 measured samples per operation and
case. `--probe-contract` uses one warmup plus two samples, registered as
`editor_responsiveness_contract`. CTest asserts observer isolation, actual
serialized mutations, equivalent tool counts, dirty/no-op Save disk-write
semantics, save/reopen equivalence, and (since CCUX-23b-4) zero catalog/model
decode and plane replay for warmed unchanged content, independently positive
catalog reads for calibrated cases, and zero discovery/provider
calls through an injected counting factory. It has no absolute timing threshold. CCUX-23b-2 additionally requires exactly one
Workspace/Editor refresh per measured Save/Add/parameter/ROI action. The original
23b-1 CSV remains the unchanged baseline (Save/Add had two refreshes).
CCUX-23b-3 requires zero hidden Calibration/Runtime rebuilds for all measured
Editor actions; page-entry/project-switch checks are in `ui_editor_usability`.
CCUX-23b-4 preserves inclusive `catalog_read` semantics and adds
`catalog_bytes_read` for actual bounded byte-read count/time. Hash/JSON/replay
time is excluded from the latter; missing files do not count as byte reads.

CCUX-23b-5 requires exactly one context, display environment and calibration
identity build per refresh. `display_environment` counts the UI view factory,
not environment reads performed inside exact calibration service checks. The
context and environment are stack-local and explicitly passed to Properties,
overlay, flow and result rendering. No display view is captured by callbacks or
used as execution admission. Independent refresh/generation calls build fresh
views; a generation change emitted directly during Properties destruction must
retire the outer view before flow. Context, environment and calibration-identity
counter durations remain inclusive and must not be summed.

`ui_editor_usability` also runs `editor_property_commits.cpp`: no-op controls
produce no refresh and preserve exact hash/status/legacy parameters; pending final
values reach Save/Add, conflicts retain dirty drafts, invalid values stop both
actions, and geometry operation changes retain only the image connection. It
records `property-commits.json` and normal/small screenshots.

Cases are distinct: the copied 23a multi-source fixture; the same fixture with a
structural 49-centre framed 7x7 model (not raster detection or physical evidence);
an uncalibrated single source; and five uncalibrated sources. The calibrated
cases add a fixed preprocess and code-reader tool before the baseline is saved.
Every sample restores that saved recipe and disk state outside the timed region.
Dirty Save changes the name before timing; no-op Save starts clean. Parameter
always changes its channel, Add always adds exactly one preprocess, and ROI
always commits a fresh code-reader search rectangle using a synthetic frame.
Add/ROI never accumulate tools or retain the preceding edit. Setup is excluded.
No camera is opened and no result/score/mm acceptance is invented.

Outputs: `raw.csv` (including warmups, success flags and per-sample counters),
`failures.csv` (failed samples only), `summary.json` (successful measured samples,
median, nearest-rank p95, case paths and metric definitions), and normal/small
window PNGs. Exit 0 requires all samples to pass; the process exits normally.

Operation time ends when the synchronous action returns. Paint time is the first
Paint event in the visible Editor subtree. The queued-event metric is elapsed
time to a zero-delay callback posted immediately before the action; it includes
the synchronous action and queue dispatch, not an independent periodic heartbeat.
Offscreen Paint is not physical monitor presentation. Counter durations are
inclusive and overlap; do not sum them. `catalog_read` includes calls that find
no file; `catalog_decode` counts record-list validation; `model_decode` counts
record decoding, and `plane_replay` counts saved plane decode/re-fit. Service Save
includes validation/locking/serialization; `atomic_write` isolates QSaveFile
open/write/commit after serialization, not storage-device durability latency.

The frame is only needed for ROI; the probe is a CPU/UI baseline, not the live
2448x2048 camera or independent physical-mm acceptance. Later cards should use
the same executable, build type, machine, fixtures and metric definitions.

For CCUX-23b-6, use a fresh evidence directory and add `--probe-all-types`.
The original four cases stay equivalent to the baseline. Two additional framed,
two-source cases change only applicationType to measurement/defect_inspection;
the original framed case is robot_guidance. The full run has 630 rows (600
measured), not 420. The added cases have no historical timing comparator.
Normal/small Calibration and unloaded Runtime screenshots are also recorded.
The visual-check draft is saved outside timing before leaving Editor.

All timed actions use a prepared workspace/catalog; the discarded per-action
warmup is not a cold measurement. `cold-preparation.json` records one first-use
workspace creation per unique fixture path, with counters and elapsed time.
This includes registry/catalog setup, has no 20-sample distribution, and does
not measure OS-cold disk, camera preparation or cold edit latency. Cold content
invalidation/replay correctness remains covered by `active_calibration_contract`.

After the full Release run, execute:

```powershell
python scripts/compare-editor-responsiveness.py
```

Defaults compare `build/ccux/ccux-23b-1/baseline` against
`build/ccux/ccux-23b-6/final`. Optional `--baseline`, `--final`, `--output`
select other evidence paths. The script validates sample counts, semantic and
hardware counters, unchanged tracked input hashes, and summary statistics. It
writes `comparison.csv` and `evidence-validation.json`, reporting operation,
paint, queue, Save/disk and context/catalog durations separately, plus failures
to meet the proposed 50 ms median / 100 ms p95 target. Timing targets are reports,
not flaky CTest assertions. Inclusive durations must not be summed.
