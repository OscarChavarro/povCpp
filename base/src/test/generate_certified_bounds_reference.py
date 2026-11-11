#!/usr/bin/env python3
"""Generate independent SLSQP distance fixtures for certified-bound tests.

NumPy and SciPy are only needed to regenerate fixtures, not to build the suite.
"""
import numpy as np
from scipy.optimize import minimize

matrices = [np.array([[1., .9, .2], [0., 1., .6], [0., 0., 1.]]),
            np.array([[.2, 0., 0.], [0., 3., .4], [0., 0., 1.]]),
            np.array([[1., .8, .4], [.3, 1., -.2], [.1, .5, 1.2]])]
points = [(0., 0., 0.), (.1, .3, -.2), (.6, -.2, .1),
          (.3, .6, .2), (1.2, .3, -.1), (-.4, -1.1, .6), (.2, .1, 1.4), (-.9, .3, -.4)]
seeds = [(1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0),
         (0, 0, 1), (0, 0, -1), (1, 1, 1), (-1, 1, -1)]
for a in matrices:
    values = []
    for point in points:
        world = a @ np.array(point)
        def objective(u):
            d = a @ u - world
            return float(d @ d)
        candidates = []
        for seed in seeds:
            start = np.array(seed, dtype=float)
            start /= np.linalg.norm(start)
            result = minimize(objective, start, method='SLSQP',
                              constraints={'type': 'eq', 'fun': lambda u: u @ u - 1},
                              options={'ftol': 1e-14, 'maxiter': 400})
            if result.success and abs(result.x @ result.x - 1) < 1e-8:
                candidates.append(result.fun)
        if not candidates:
            raise RuntimeError(f'No converged solution for {point}')
        values.append(float(np.sqrt(min(candidates))))
    print('{' + ', '.join(f'{value:.15g}' for value in values) + '},')
