"""Compare regression, load and resource reports without a CI server.

Only the JUnit path is required for backwards compatibility.  Supplying load
or resource JSON adds performance checks suitable for a release gate.
"""

import argparse
import json
import xml.etree.ElementTree as ET
from pathlib import Path


def summary(path: str) -> dict[str, int]:
    root = ET.parse(path).getroot()
    suites = [root] if root.tag == "testsuite" else list(root)
    result = {"tests": 0, "failures": 0, "errors": 0, "skipped": 0}
    for suite in suites:
        for key in result:
            result[key] += int(suite.attrib.get(key, 0))
    return result


def load_results(path: str) -> list[dict]:
    document = json.loads(Path(path).read_text(encoding="utf-8"))
    if isinstance(document, dict) and isinstance(document.get("results"), list):
        return [item for item in document["results"] if isinstance(item, dict)]
    raise ValueError(f"load report has no results array: {path}")


def _scenario_key(result: dict) -> tuple:
    return (
        result.get("scenario"),
        result.get("users"),
        result.get("rooms"),
        result.get("users_per_room"),
    )


def _match_baseline(current: dict, baseline: list[dict]) -> dict | None:
    exact = _scenario_key(current)
    for item in baseline:
        if _scenario_key(item) == exact:
            return item
    for item in baseline:
        if item.get("scenario") == current.get("scenario"):
            return item
    return None


def _rate(result: dict) -> float | None:
    success = result.get("success")
    failed = result.get("failed")
    if not isinstance(success, (int, float)) or not isinstance(failed, (int, float)):
        return None
    total = success + failed
    return success / total if total else None


def compare_load(current_path: str, baseline_path: str, max_latency_increase: float,
                 max_success_drop: float, max_delivery_drop: float) -> list[str]:
    current = load_results(current_path)
    baseline = load_results(baseline_path)
    regressions: list[str] = []
    for item in current:
        old = _match_baseline(item, baseline)
        if old is None:
            continue
        old_rate, new_rate = _rate(old), _rate(item)
        if old_rate is not None and new_rate is not None and old_rate - new_rate > max_success_drop:
            regressions.append(
                f"{item.get('scenario')} success rate dropped {old_rate:.3f} -> {new_rate:.3f}"
            )
        old_p95, new_p95 = old.get("p95_latency_ms"), item.get("p95_latency_ms")
        if isinstance(old_p95, (int, float)) and isinstance(new_p95, (int, float)) and old_p95 > 0:
            increase = (new_p95 - old_p95) / old_p95 * 100
            if increase > max_latency_increase:
                regressions.append(
                    f"{item.get('scenario')} P95 latency increased {old_p95}ms -> {new_p95}ms ({increase:.1f}%)"
                )
        old_delivery, new_delivery = old.get("delivery_ratio"), item.get("delivery_ratio")
        if isinstance(old_delivery, (int, float)) and isinstance(new_delivery, (int, float)):
            if old_delivery - new_delivery > max_delivery_drop:
                regressions.append(
                    f"{item.get('scenario')} delivery ratio dropped {old_delivery:.4f} -> {new_delivery:.4f}"
                )
        old_errors = set(old.get("errors") or [])
        new_errors = set(item.get("errors") or [])
        added = sorted(str(error) for error in new_errors - old_errors)
        if added:
            regressions.append(f"{item.get('scenario')} new errors: {', '.join(added)}")
    return regressions


def resource_summary(path: str) -> dict:
    document = json.loads(Path(path).read_text(encoding="utf-8"))
    result = document.get("summary") if isinstance(document, dict) else None
    if not isinstance(result, dict):
        raise ValueError(f"resource report has no summary object: {path}")
    return result


def compare_resources(current_path: str, baseline_path: str, max_resource_increase: float) -> list[str]:
    current, baseline = resource_summary(current_path), resource_summary(baseline_path)
    regressions: list[str] = []
    metrics = {
        "peak_cpu_percent": "CPU",
        "peak_rss_kb": "RSS",
        "peak_threads": "threads",
        "peak_tcp_connections": "TCP connections",
    }
    for key, label in metrics.items():
        old, new = baseline.get(key), current.get(key)
        if not isinstance(old, (int, float)) or not isinstance(new, (int, float)) or old <= 0:
            continue
        increase = (new - old) / old * 100
        if increase > max_resource_increase:
            regressions.append(f"{label} peak increased {old} -> {new} ({increase:.1f}%)")
    if current.get("process_exited"):
        regressions.append("server process exited during monitored run")
    return regressions


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("current")
    parser.add_argument("--baseline")
    parser.add_argument("--current-load")
    parser.add_argument("--baseline-load")
    parser.add_argument("--current-resource")
    parser.add_argument("--baseline-resource")
    parser.add_argument("--max-latency-increase", type=float, default=25.0)
    parser.add_argument("--max-success-drop", type=float, default=0.02)
    parser.add_argument("--max-delivery-drop", type=float, default=0.02)
    parser.add_argument("--max-resource-increase", type=float, default=50.0)
    args = parser.parse_args()
    regressions: list[str] = []
    current = summary(args.current)
    print(f"current: {current}")
    if args.baseline:
        baseline = summary(args.baseline)
        print(f"baseline: {baseline}")
        if current["failures"] + current["errors"] > baseline["failures"] + baseline["errors"]:
            regressions.append("failure/error count increased")
    if bool(args.current_load) != bool(args.baseline_load):
        parser.error("--current-load and --baseline-load must be supplied together")
    if args.current_load:
        regressions.extend(compare_load(args.current_load, args.baseline_load, args.max_latency_increase,
                                        args.max_success_drop, args.max_delivery_drop))
    if bool(args.current_resource) != bool(args.baseline_resource):
        parser.error("--current-resource and --baseline-resource must be supplied together")
    if args.current_resource:
        regressions.extend(compare_resources(args.current_resource, args.baseline_resource, args.max_resource_increase))
    if regressions:
        for regression in regressions:
            print(f"REGRESSION: {regression}")
        return 1
    print("comparison: no regressions detected")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
