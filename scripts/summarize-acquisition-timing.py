"""Summarize CCUX-22 inclusive spans. Never add parent and child durations."""
import argparse
import collections
import json
import math
from pathlib import Path
import statistics


def stats(values):
    ordered = sorted(values)
    if not ordered:
        return {"count": 0}
    return {"count": len(ordered), "medianMs": statistics.median(ordered),
            "p95Ms": ordered[math.ceil(.95 * len(ordered)) - 1],
            "minMs": ordered[0], "maxMs": ordered[-1]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    groups = collections.defaultdict(lambda: collections.defaultdict(list))
    files = sorted(args.directory.rglob("*.jsonl"))
    for path in files:
        for line in path.read_text(encoding="utf-8-sig").splitlines():
            if not line.strip():
                continue
            row = json.loads(line)
            if row.get("schema") != "visionflow.acquisition-timing/1":
                if path.name == "runs.jsonl":
                    continue
                raise ValueError(f"Unknown timing schema: {path}")
            groups[(str(path.relative_to(args.directory)), row["flow"], row["stage"])][row["outcome"]].append(row["durationMs"])
    result = {"scope": "Inclusive intervals per stage; no nested duration sum. UI paint is Qt paint completion, not display scanout.",
              "traceFiles": len(files), "stages": [], "instrumentationComparison": [], "physicalServiceRuns": []}
    lines = ["# CCUX-22a timing summary", "", result["scope"], "", "| Evidence / flow / stage | Success | Median ms | p95 ms | Min ms | Max ms | Other outcomes |", "| --- | ---: | ---: | ---: | ---: | ---: | --- |"]
    for (path, flow, stage), outcomes in sorted(groups.items()):
        success = stats(outcomes.get("success", []))
        other = {name: stats(values) for name, values in outcomes.items() if name != "success"}
        result["stages"].append({"file": path, "flow": flow, "stage": stage, "success": success, "other": other})
        counters = ", ".join(f"{name}: {values['count']}" for name, values in other.items())
        columns = [f"{success[name]:.3f}" if name in success else "—" for name in ("medianMs", "p95Ms", "minMs", "maxMs")]
        lines.append(f"| {path} / {flow} / {stage} | {success['count']} | {' | '.join(columns)} | {counters} |")
    # The fixture interleaves three off/on rounds; no hardware conclusions from it.
    for path in args.directory.rglob("samples.json"):
        data = json.loads(path.read_text(encoding="utf-8-sig"))
        for flow in sorted({r["flow"] for r in data["samples"]}):
            pair = {str(enabled): stats([r["durationMs"] for r in data["samples"] if r["flow"] == flow and r["timingEnabled"] == enabled]) for enabled in (False, True)}
            off, on = pair["False"], pair["True"]
            pair.update({"flow": flow, "kind": data["kind"], "medianDeltaMs": on["medianMs"] - off["medianMs"]})
            result["instrumentationComparison"].append(pair)
    physical = collections.defaultdict(list)
    for path in args.directory.rglob("*-report.json"):
        data = json.loads(path.read_text(encoding="utf-8-sig"))
        if "environment" not in data or "samples" not in data:
            continue
        mode = data["environment"]["timingEnabled"]
        for flow in ("editor", "runtime", "calibration"):
            if path.name.startswith(flow + "-"):
                values = [r.get("serviceTotalMs", r.get("milliseconds")) for r in data["samples"]]
                if not values or any(v is None for v in values):
                    continue
                result["physicalServiceRuns"].append({"file": str(path.relative_to(args.directory)), "flow": flow, "timingEnabled": mode,
                    "firstCaptureMs": values[0], "repeatedCaptures": stats(values[1:]), "allCaptures": stats(values)})
                physical[(flow, mode)].extend(values)
    for flow in ("editor", "runtime", "calibration"):
        if physical[(flow, False)] and physical[(flow, True)]:
            off, on = stats(physical[(flow, False)]), stats(physical[(flow, True)])
            result["instrumentationComparison"].append({"flow": flow, "kind": "physical-service-baseline", "False": off, "True": on,
                "medianDeltaMs": on["medianMs"] - off["medianMs"]})
    (args.directory / "summary.json").write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    (args.directory / "summary.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"{len(files)} traces, {len(groups)} stage groups; summary.json / summary.md")


if __name__ == "__main__":
    main()
