import json

from scripts.compare_reports import compare_load, compare_resources


def _write(path, value):
    path.write_text(json.dumps(value), encoding="utf-8")
    return str(path)


def test_compare_load_detects_latency_and_delivery_regression(tmp_path):
    baseline = _write(tmp_path / "baseline.json", {"results": [{
        "scenario": "media", "users": 5, "p95_latency_ms": 10, "delivery_ratio": 1.0,
    }]})
    current = _write(tmp_path / "current.json", {"results": [{
        "scenario": "media", "users": 5, "p95_latency_ms": 20, "delivery_ratio": 0.8,
    }]})
    issues = compare_load(current, baseline, 25, 0.02, 0.02)
    assert len(issues) == 2


def test_compare_resources_accepts_small_variation(tmp_path):
    baseline = _write(tmp_path / "baseline.json", {"summary": {
        "peak_cpu_percent": 20, "peak_rss_kb": 1000, "peak_threads": 10, "peak_tcp_connections": 20,
    }})
    current = _write(tmp_path / "current.json", {"summary": {
        "peak_cpu_percent": 25, "peak_rss_kb": 1100, "peak_threads": 11, "peak_tcp_connections": 22,
    }})
    assert compare_resources(current, baseline, 50) == []
