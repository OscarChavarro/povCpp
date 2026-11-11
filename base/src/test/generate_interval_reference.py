#!/usr/bin/env python3
"""Print exact-rational binary64 bounds used by FloatingInterval tests."""
from fractions import Fraction
from math import inf, nextafter

pairs = [(0.1, 0.2), (-0.1, 0.2), (1e-300, 1e-20), (1e150, 1e140),
         (1.0000000000000002, 0.9999999999999999), (-1e-200, -1e-110),
         (1.0, 3.0), (1e-200, 1e100)]
for x, y in pairs:
    a, b = Fraction.from_float(x), Fraction.from_float(y)
    for operation, exact in enumerate((a + b, a - b, a * b, a / b)):
        value = float(exact)
        represented = Fraction.from_float(value)
        lower = nextafter(value, -inf) if represented > exact else value
        upper = nextafter(value, inf) if represented < exact else value
        print(f"{{{x!r}, {y!r}, {operation}, {lower!r}, {upper!r}}},")
