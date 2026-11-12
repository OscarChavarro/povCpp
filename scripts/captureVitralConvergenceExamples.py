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
        if name in ('offline-cow-sequential', 'offline-cow-parallel'):
            parts = image.read_bytes().split(b'\n', 4)
            if len(parts) != 5 or parts[0] != b'P6' or parts[3] != b'255':
                raise RuntimeError(f'{name} is not a supported PPM image')
            pixels = parts[4]
            background = pixels[:3]
            non_background = 0
            cow_colored = 0
            for i in range(0, len(pixels) - 2, 3):
                red, green, blue = pixels[i:i+3]
                if pixels[i:i+3] != background:
                    non_background += 1
                if red > 40 and red * 2 > green * 3 and blue * 2 > green * 3:
                    cow_colored += 1
            if non_background < 1000 or cow_colored < 100:
                raise RuntimeError(f'{name} does not visibly contain the cow mesh')
        if name.startswith(('offline-quadric_', 'offline-box_', 'offline-halfspace_')):
            parts = image.read_bytes().split(b'\n', 4)
            if len(parts) != 5 or parts[0] != b'P6' or parts[3] != b'255':
                raise RuntimeError(f'{name} is not a supported PPM image')
            pixels = parts[4]
            colored = sum(1 for i in range(0, len(pixels) - 2, 3)
                          if pixels[i] > 60 and pixels[i] * 2 > pixels[i+1] * 3
                          and pixels[i] * 2 > pixels[i+2] * 3)
            if colored < 100:
                raise RuntimeError(f'{name} does not visibly contain the colored solid')

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
