#!/usr/bin/env python3
"""Reject new Roth discrepancies relative to the recorded starting baseline."""
import json
import os
from pathlib import Path
import re
import subprocess
import sys


def main():
    root = Path(__file__).resolve().parents[1]
    baseline = json.loads((root / 'doc/vitralConvergenceBaseline.json').read_text())['roth']
    pattern = r'^RMSE above threshold: (.+) \(RMSE=([^,]+), threshold=([^)]+)\)$'
    allowed = {m[0] for m in re.findall(pattern, baseline['comparison'], re.MULTILINE)}
    env = dict(os.environ, RMSE_THRESHOLD=str(baseline['comparison_threshold']))
    result = subprocess.run(['bash', 'scripts/testCsgRoth.sh'], cwd=root, env=env,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    print(result.stdout, end='')
    current = set()
    unexpected = []
    for line in result.stdout.splitlines():
        match = re.fullmatch(pattern, line)
        if match:
            current.add(match[1])
        elif not re.fullmatch(r'Test (passed \(RMSE threshold [^)]+\)|failed with \d+ issue\(s\))\.', line):
            unexpected.append(line)
    new = current - allowed
    # Only the comparator's expected discrepancy exit is acceptable. Missing
    # images, invalid metrics and other diagnostics must never become exceptions.
    expected_exit = 1 if current else 0
    if result.returncode != expected_exit or unexpected or new or not result.stdout.strip():
        print('Roth regression check FAILED; new scenes:', sorted(new),
              'other errors:', unexpected, file=sys.stderr)
        return 1
    print(f'Roth regression check passed: {len(current)} known discrepancies, no new scenes.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
