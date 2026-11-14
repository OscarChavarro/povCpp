#!/usr/bin/env python3
"""Check the migration inventory, the retired raycasting module, VITRAL's independence from
PovRay, class uniqueness across both trees and byte equality of the local base mirror."""
import argparse
import json
from pathlib import Path
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
    errors = []
    if (root / 'raycasting').exists():
        errors.append('The retired raycasting module directory exists again')
    if len(listed) != len(set(listed)):
        errors.append('Duplicate source entries in migration inventory')
    for path in listed:
        if not (root / path).is_file():
            errors.append('Inventory entry without a source file: ' + path)
    for entry in entries:
        if entry['destination'] not in ('VITRAL', 'PovRay', 'none'):
            errors.append('Invalid destination: ' + entry['source'])
        if not all(entry.get(key) for key in ('action', 'target', 'acceptance', 'policy')):
            errors.append('Incomplete migration entry: ' + entry['source'])
    # VITRAL never depends on PovRay code.
    pov_roots = ('environment/', 'render/', 'io/', 'app/')
    for source in sorted((args.vitral_root / 'base/src').rglob('*')):
        if source.suffix not in ('.h', '.cpp', '.txx') or not source.is_file():
            continue
        for line in source.read_text(errors='replace').splitlines():
            if line.startswith('#include "') and line[10:].startswith(pov_roots):
                errors.append('VITRAL includes PovRay code: ' + str(source))
                break
    # One implementation per class: no PovRay class repeats a VITRAL class name.
    def class_names(directory):
        names = set()
        for header in directory.rglob('*.h'):
            for line in header.read_text(errors='replace').splitlines():
                words = line.replace(':', ' ').replace('{', ' ').split()
                if len(words) >= 2 and words[0] == 'class' and not line.rstrip().endswith(';'):
                    names.add(words[1])
        return names
    for name in sorted(class_names(root / 'src') & class_names(args.vitral_root / 'base/src/main')):
        errors.append('Class defined in both PovRay and VITRAL: ' + name)
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
    print(f'{len(entries)} components pending in stage K; no raycasting module; '
          f'no reverse dependency or duplicated class; {len(files)} base files identical to VITRAL.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
