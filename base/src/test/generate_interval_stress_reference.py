#!/usr/bin/env python3
"""Recreate the deterministic exact-rational interval stress fixtures."""
from fractions import Fraction
from math import inf, isfinite, ldexp, nextafter
from random import Random

random = Random(417892)
count = 0
while count < 64:
    exponent_x = random.randint(-530, 530)
    exponent_y = random.randint(-530, 530)
    x = ldexp(random.choice((-1.0, 1.0)) * (1.0 + random.getrandbits(52) / 2**52), exponent_x)
    y = ldexp(random.choice((-1.0, 1.0)) * (1.0 + random.getrandbits(52) / 2**52), exponent_y)
    # Exercise exact cancellation and adjacent representable operands too.
    if count % 11 == 0:
        y = nextafter(x, inf)
    a, b = Fraction.from_float(x), Fraction.from_float(y)
    operation = count % 4
    exact = (a + b, a - b, a * b, a / b)[operation]
    try:
        value = float(exact)
    except OverflowError:
        continue
    if not isfinite(value):
        continue
    represented = Fraction.from_float(value)
    lower = nextafter(value, -inf) if represented > exact else value
    upper = nextafter(value, inf) if represented < exact else value
    if not isfinite(lower) or not isfinite(upper):
        continue
    print(f"        {{{x!r}, {y!r}, {operation}, {lower!r}, {upper!r}}},")
    count += 1
