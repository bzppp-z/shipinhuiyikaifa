"""Compare two pytest JUnit XML files without depending on a CI server."""

import argparse
import xml.etree.ElementTree as ET


def summary(path: str) -> dict[str, int]:
    root = ET.parse(path).getroot()
    suites = [root] if root.tag == "testsuite" else list(root)
    result = {"tests": 0, "failures": 0, "errors": 0, "skipped": 0}
    for suite in suites:
        for key in result:
            result[key] += int(suite.attrib.get(key, 0))
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("current")
    parser.add_argument("--baseline")
    args = parser.parse_args()
    current = summary(args.current)
    print(f"current: {current}")
    if args.baseline:
        baseline = summary(args.baseline)
        print(f"baseline: {baseline}")
        if current["failures"] + current["errors"] > baseline["failures"] + baseline["errors"]:
            print("REGRESSION: failure/error count increased")
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
