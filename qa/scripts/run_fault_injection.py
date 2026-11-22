"""Run stage-eight protocol fault probes against a running server."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "qa"))

from fault_injection import PROBES, report, run  # noqa: E402


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--scenario", choices=["all", *PROBES], default="all")
    parser.add_argument("--output", default="qa/reports/fault-injection.json")
    args = parser.parse_args()

    names = list(PROBES) if args.scenario == "all" else [args.scenario]
    document = {
        "generated_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        **report(run(names)),
    }
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(document, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps(document, ensure_ascii=False, indent=2))
    return 0 if document["failed"] == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
