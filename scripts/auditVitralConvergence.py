#!/usr/bin/env python3
"""Check migration inventory coverage and byte equality of the local base mirror."""
import argparse
import json
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--vitral-root', type=Path,
                        default=Path.home() / 'VITRAL/vitral/cpp')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    inventory = json.loads((root / 'doc/vitralConvergenceInventory.json').read_text())
    entries = inventory['files']
    listed = [entry['source'] for entry in entries]
    tracked_or_new = set(subprocess.check_output(
        ['git', 'ls-files', '--cached', '--others', '--exclude-standard', 'raycasting'],
        cwd=root, text=True).splitlines())
    # Deleted tracked files remain in the index until staged; audit the working tree.
    actual = {path for path in tracked_or_new if (root / path).is_file()}
    errors = []
    if len(listed) != len(set(listed)):
        errors.append('Duplicate source entries in migration inventory')
    for path in sorted(actual - set(listed)):
        errors.append('Unmapped raycasting source: ' + path)
    for path in sorted(set(listed) - actual):
        errors.append('Retired source still in active inventory: ' + path)
    for entry in entries:
        if entry['destination'] not in ('VITRAL', 'PovRay', 'none'):
            errors.append('Invalid destination: ' + entry['source'])
        if not all(entry.get(key) for key in ('action', 'target', 'acceptance', 'policy')):
            errors.append('Incomplete migration entry: ' + entry['source'])
    mirror = root / 'base/src/main'
    files = sorted(p for p in mirror.rglob('*') if p.is_file())
    if not files:
        errors.append('Local base mirror is missing or empty')
    for source in files:
        relative = source.relative_to(mirror)
        upstream = args.vitral_root / 'base/src/main' / relative
        if not upstream.is_file() or source.read_bytes() != upstream.read_bytes():
            errors.append('Base mirror differs or upstream file is missing: ' + str(relative))
    for error in errors:
        print(error, file=sys.stderr)
    if errors:
        return 1
    print(f'Mapped {len(entries)} raycasting files; {len(files)} base files identical to VITRAL.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
