#!/usr/bin/env python3
"""Regenerate the independent SLSQP fixtures for the affine distance test.

Requires NumPy and SciPy; not needed to build or run the C++ test suite. The
minimization is intentionally independent of the toolkit's secular solver.
"""
import numpy as np
from scipy.optimize import minimize

transform = np.array([[-2.0, 0.5, 0.0], [0.0, 1.0, -0.3], [0.0, 0.0, 1.0]])
points = [(0.12, 0.27, 0.31), (0.65, 0.1, -0.2), (0.21, -0.72, 0.35),
          (0.35, 0.55, 0.15), (1.1, 0.2, 0.3), (-1.1, 0.8, 0.4),
          (0.1, -1.2, 0.5), (0.3, 0.2, 1.5), (-0.9, -0.4, -0.3),
          (0.01, 0.01, 0.01), (0.0, 0.0, 0.0), (1.5, -1.4, 0.9)]
seeds = [(1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0),
         (0, 0, 1), (0, 0, -1), (1, 1, 1), (-1, 1, -1)]
for point in points:
    world_point = transform @ np.array(point)
    def objective(u):
        delta = transform @ u - world_point
        return float(delta @ delta)
    candidates = []
    for seed in seeds:
        start = np.array(seed, dtype=float)
        start /= np.linalg.norm(start)
        result = minimize(objective, start, method="SLSQP",
                          constraints={"type": "eq", "fun": lambda u: u @ u - 1},
                          options={"ftol": 1e-14, "maxiter": 400})
        if result.success and abs(result.x @ result.x - 1) < 1e-8:
            candidates.append(result.fun)
    if not candidates:
        raise RuntimeError(f"No valid solution for {point}")
    print(f"{{{point[0]:.17g}, {point[1]:.17g}, {point[2]:.17g}, {np.sqrt(min(candidates)):.15g}}},")
