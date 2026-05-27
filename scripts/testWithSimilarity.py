#!/usr/bin/env python3
"""
Compare generated images with references using similarity metrics instead of exact match.
Supports: mean absolute error (MAE), pixel-level difference.
"""

import os
import sys
from pathlib import Path
from collections import defaultdict

try:
    import numpy as np
    from PIL import Image
    HAVE_PIL = True
except ImportError:
    HAVE_PIL = False
    print("Warning: PIL/Pillow not available", file=sys.stderr)
    sys.exit(1)

def read_tga(path):
    """Read a TGA file and return pixel data as numpy array."""
    img = Image.open(path)
    return np.array(img)

def compare_images_mae(img1_path, img2_path):
    """Calculate Mean Absolute Error between two images."""
    try:
        img1 = read_tga(img1_path)
        img2 = read_tga(img2_path)

        # Convert to same dtype for comparison
        img1 = img1.astype(float)
        img2 = img2.astype(float)

        # Calculate MAE
        mae = np.mean(np.abs(img1 - img2))
        return mae
    except Exception as e:
        print(f"Error comparing {img1_path} and {img2_path}: {e}", file=sys.stderr)
        return None

def test_with_threshold(mae_threshold):
    """Run tests with given MAE threshold."""
    output_dir = Path("output")
    reference_dir = Path("../referenceTestImages")

    passed = 0
    failed = 0
    results = []

    # Find all generated TGA files
    for generated_path in sorted(output_dir.rglob("*.tga")):
        rel_path = generated_path.relative_to(output_dir)
        reference_path = reference_dir / rel_path

        if not reference_path.exists():
            failed += 1
            continue

        mae = compare_images_mae(str(generated_path), str(reference_path))

        if mae is not None and mae < mae_threshold:
            passed += 1
            results.append((str(rel_path), mae, True))
        else:
            failed += 1
            results.append((str(rel_path), mae, False))

    total = passed + failed
    return passed, failed, total, results

def main():
    output_dir = Path("output")
    reference_dir = Path("../referenceTestImages")

    if not output_dir.exists():
        print(f"Missing output directory: {output_dir}", file=sys.stderr)
        sys.exit(1)

    if not reference_dir.exists():
        print(f"Missing reference directory: {reference_dir}", file=sys.stderr)
        sys.exit(1)

    print(f"\n{'='*70}")
    print(f"Testing with different MAE (Mean Absolute Error) thresholds")
    print(f"{'='*70}\n")

    thresholds = [2.0, 5.0, 10.0, 15.0, 20.0, 30.0, 50.0, 100.0]

    for threshold in thresholds:
        passed, failed, total, results = test_with_threshold(threshold)
        pct = (100 * passed) // total if total > 0 else 0
        print(f"MAE < {threshold:5.1f}:  {passed:3d}/{total:3d} pass ({pct:3d}%)")

    # Show details for reasonable threshold
    print(f"\n{'='*70}")
    print(f"Detailed results with MAE < 10.0 threshold:")
    print(f"{'='*70}\n")

    passed, failed, total, results = test_with_threshold(10.0)

    # Group by pass/fail
    passed_imgs = [r for r in results if r[2]]
    failed_imgs = [r for r in results if not r[2]]

    print(f"PASSED ({len(passed_imgs)}):")
    for img, mae, _ in sorted(passed_imgs):
        print(f"  ✓ {img:50s} MAE={mae:6.2f}")

    if failed_imgs:
        print(f"\nFAILED ({len(failed_imgs)}):")
        for img, mae, _ in sorted(failed_imgs)[:20]:
            print(f"  ✗ {img:50s} MAE={mae:6.2f}" if mae is not None else f"  ✗ {img}")
        if len(failed_imgs) > 20:
            print(f"  ... and {len(failed_imgs) - 20} more")

    return 0

if __name__ == '__main__':
    sys.exit(main())
