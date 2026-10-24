"""Validate and compare the unchanged CCUX-23b-1 baseline with final probe CSVs.

No absolute timing assertion: the 50/100 ms engineering targets are reported.
Run after the Release probe with --probe-all-types (one warmup + 20 samples).
"""
import argparse
import csv
import hashlib
import json
import math
import platform
import statistics
import subprocess
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path


def read_rows(path):
    with path.open(encoding="utf-8-sig", newline="") as stream:
        return list(csv.DictReader(stream))


def timing(rows, key):
    values = sorted(float(row[key]) for row in rows)
    return {"median": statistics.median(values), "p95": values[math.ceil(.95 * len(values)) - 1]}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def validate(directory, final):
    rows = read_rows(directory / "raw.csv")
    summary = json.loads((directory / "summary.json").read_text(encoding="utf-8"))
    assert summary["failures"] == 0 and summary["repetitions"] >= 20
    assert not read_rows(directory / "failures.csv")
    groups = defaultdict(list)
    for row in rows:
        assert row["success"] == "1", row
        assert all(row[key] == "0" for key in ("provider_capabilities", "provider_creates", "discoveries"))
        assert int(row["tools_after"]) == int(row["tools_before"]) + (row["operation"] == "add")
        if row["operation"].startswith("save"):
            assert row["dirty_after"] == "0"
            assert row["atomic_write_calls"] == str(int(row["operation"] == "save_dirty"))
        else:
            assert row["dirty_after"] == "1"
        if final:
            assert all(row[key + "_calls"] == "1" for key in ("workspace", "editor", "context", "display_environment", "calibration_identity"))
            assert all(row[key + "_calls"] == "0" for key in ("calibration", "runtime", "catalog_decode", "model_decode", "plane_replay"))
        groups[(row["case"], row["operation"])].append(row)
    measured = {}
    for key, samples in groups.items():
        assert sum(row["warmup"] == "1" for row in samples) == 1
        measured[key] = [row for row in samples if row["warmup"] == "0"]
        assert len(measured[key]) == summary["repetitions"]
        assert {int(row["iteration"]) for row in samples} == set(range(summary["repetitions"] + 1))
        assert len({row["tools_before"] for row in samples}) == 1
    for item in summary["results"]:
        samples = measured[(item["case"], item["operation"])]
        assert item["successfulMeasuredSamples"] == len(samples)
        for key, prefix in (("operation_ms", "operation"), ("first_editor_paint_ms", "paint"), ("queued_event_ms", "queuedEvent")):
            values = timing(samples, key)
            assert abs(values["median"] - item["timings"][prefix + "MedianMs"]) < .01
            assert abs(values["p95"] - item["timings"][prefix + "P95Ms"]) < .01
    return rows, measured, summary


def main():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument("--baseline", type=Path, default=Path("build/ccux/ccux-23b-1/baseline"))
    parser.add_argument("--final", type=Path, default=Path("build/ccux/ccux-23b-6/final"))
    parser.add_argument("--output", type=Path, default=Path("build/ccux/ccux-23b-6"))
    args = parser.parse_args()
    baseline_rows, baseline, _ = validate(args.baseline, False)
    final_rows, final, summary = validate(args.final, True)
    assert len(final) == 30 and len(baseline) == 20
    assert {item["applicationType"] for item in summary["cases"]} == {"robot_guidance", "defect_inspection", "measurement"}
    cold = json.loads((args.final / "cold-preparation.json").read_text(encoding="utf-8"))
    assert {item["case"] for item in cold["cases"]} == {item["case"] for item in summary["cases"]}
    for item in cold["cases"]:
        counters = item["counters"]
        calibrated = item["case"].startswith(("framed", "legacy"))
        assert counters["catalog_decode"]["calls"] == 1
        assert counters["model_decode"]["calls"] == (2 if calibrated else 0)
        assert counters["plane_replay"]["calls"] == int(calibrated)
    original_meta = json.loads((args.baseline.parent / "run-metadata.json").read_text(encoding="utf-8"))
    for filename, sha in original_meta["inputSha256"].items():
        assert digest(Path("tests/fixtures/editor-responsiveness") / filename) == sha
    report = []
    for (case, operation), samples in final.items():
        before = baseline.get((case, operation))
        item = {"case": case, "operation": operation, "samples": len(samples), "baselineComparable": bool(before)}
        for key in ("operation_ms", "first_editor_paint_ms", "queued_event_ms", "project_save_inclusive_ms", "atomic_write_inclusive_ms", "context_inclusive_ms", "catalog_read_inclusive_ms", "catalog_bytes_read_inclusive_ms"):
            values = timing(samples, key)
            item[key + "_median"] = values["median"]
            item[key + "_p95"] = values["p95"]
        item["engineeringTargetMet"] = item["operation_ms_median"] < 50 and item["operation_ms_p95"] < 100
        item["baseline_operation_median"] = timing(before, "operation_ms")["median"] if before else None
        item["baseline_operation_p95"] = timing(before, "operation_ms")["p95"] if before else None
        item["operation_median_speedup"] = item["baseline_operation_median"] / item["operation_ms_median"] if before else None
        for key in samples[0]:
            if key.endswith("_calls"):
                item[key + "_min"] = min(int(row[key]) for row in samples)
                item[key + "_max"] = max(int(row[key]) for row in samples)
        report.append(item)
    args.output.mkdir(parents=True, exist_ok=True)
    with (args.output / "comparison.csv").open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(report[0]))
        writer.writeheader()
        writer.writerows(report)
    evidence = {
        "sourceHead": subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip(),
        "platform": platform.platform(),
        "machine": platform.node(),
        "build": "Release / Qt 6.12.0 / MinGW 13.1.0 / OpenCV 5.0.0 / UI ON / ZXing ON",
        "command": "visionflow.exe --editor-responsiveness-probe --probe-all-types",
        "environment": {"QT_QPA_PLATFORM": "offscreen", "VISIONFLOW_TEST_ARTIFACTS": str(args.final.resolve())},
        "baselineMetadata": original_meta,
        "baselineRows": len(baseline_rows),
        "finalRows": len(final_rows),
        "baselineMeasured": sum(map(len, baseline.values())),
        "finalMeasured": sum(map(len, final.values())),
        "executableSha256": digest(Path("build/reference-ui/visionflow.exe")),
        "probeSourceSha256": digest(Path("tests/editor_responsiveness_probe.cpp")),
        "baselineRawSha256": digest(args.baseline / "raw.csv"),
        "finalRawSha256": digest(args.final / "raw.csv"),
        "sameTrackedInputs": True,
        "allSamplesAndSummariesValidated": True,
        "warmMeaning": "all timed edits have a prepared workspace/catalog; one warmup discarded for each action",
        "coldMeaning": "cold-preparation.json: one first-use workspace per new project path, no edit latency or OS-cold claim",
        "target": {
            "medianMsLessThan": 50,
            "p95MsLessThan": 100,
            "metGroups": sum(item["engineeringTargetMet"] for item in report),
            "totalGroups": len(report),
        },
        "results": report,
    }
    evidence["recordedAtUtc"] = datetime.now(timezone.utc).isoformat()
    evidence["rawCreatedUtc"] = datetime.fromtimestamp((args.final / "raw.csv").stat().st_ctime, timezone.utc).isoformat()
    evidence["rawLastWriteUtc"] = datetime.fromtimestamp((args.final / "raw.csv").stat().st_mtime, timezone.utc).isoformat()
    (args.output / "evidence-validation.json").write_text(json.dumps(evidence, indent=2), encoding="utf-8")
    print(json.dumps({key: evidence[key] for key in ("baselineRows", "finalRows", "finalMeasured", "sameTrackedInputs", "allSamplesAndSummariesValidated", "target")}, indent=2))


if __name__ == "__main__":
    main()
