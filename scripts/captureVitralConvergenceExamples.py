#!/usr/bin/env python3
"""Capture independent offline and shader baselines without replacing references."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--vitral-root', type=Path,
                        default=Path.home() / 'VITRAL/vitral/cpp')
    parser.add_argument('--output', required=True, type=Path,
                        help='New directory for images, logs, and examples.json')
    args = parser.parse_args()
    root = args.vitral_root.resolve()
    output = args.output.resolve()
    if output.exists():
        parser.error('--output must not already exist; preserve previous baselines')
    offline = root / 'testsuite/OfflineExamples/RaytracingOfflineExample'
    shaders = root / 'testsuite/OpenGL4Examples/ShadersExample'
    scenes = sorted((root.parent / 'etc/geometry/mitscenes').glob('*.ray'))
    if not scenes:
        parser.error('No MIT scene files found')
    for directory, name in ((offline, 'RaytracingOfflineExample'), (shaders, 'ShadersExample')):
        if not (directory / 'build' / name).is_file():
            parser.error(f'Build {name} before capturing baselines')
    output.mkdir(parents=True)
    records = []

    def run(name, command, cwd, image):
        start = time.monotonic()
        with (output / (name + '.log')).open('w') as log:
            result = subprocess.run(command, cwd=cwd, stdout=log,
                                    stderr=subprocess.STDOUT, timeout=300)
        record = dict(name=name, command=command, cwd=str(cwd),
                      exit_code=result.returncode,
                      wall_seconds=round(time.monotonic() - start, 3))
        if image.is_file():
            record['sha256'] = hashlib.sha256(image.read_bytes()).hexdigest()
        records.append(record)
        (output / 'examples.json').write_text(json.dumps(records, indent=2) + '\n')
        print(name, 'exit:', result.returncode, flush=True)
        if result.returncode or not image.is_file() or image.stat().st_size == 0:
            raise RuntimeError(f'{name} failed; see {output / (name + ".log")}')

    for scene in scenes:
        for mode in ('sequential', 'parallel'):
            name = 'offline-' + scene.stem + '-' + mode
            image = output / (name + '.ppm')
            command = [str(offline / 'build/RaytracingOfflineExample'),
                       '--scene', str(scene), '--output', str(image)]
            if mode == 'parallel':
                command.append('--parallel')
            run(name, command, offline, image)
        if records[-1]['sha256'] != records[-2]['sha256']:
            raise RuntimeError(f'Sequential/parallel images differ: {scene}')

    for method in ('software', 'opengl'):
        for fixture, options in (
                ('default', []),
                ('rotated', ['--rotation', '37', '--light-rotation', '20']),
                ('plain', ['--without', 'texture,bump'])):
            name = 'shaders-' + method + '-' + fixture
            image = output / (name + '.png')
            command = [str(shaders / 'build/ShadersExample'), '--offline', str(image),
                       '--method', method, '--width', '320', '--height', '240'] + options
            if method == 'opengl':
                command = ['xvfb-run', '-a'] + command
            run(name, command, shaders, image)
    print(f'Captured {len(records)} example runs in {output}')


if __name__ == '__main__':
    main()
