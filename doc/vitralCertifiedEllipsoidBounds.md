# Certified affine-sphere distance enclosures

This note describes the initial certified path in stage E. It does not certify
all existing containment paths or the numerical nearest-distance estimator.

## Target and assumptions

Inputs are the finite binary64 values stored in the point, radius and forward
affine matrix. The mathematical solid is `{c + A u : ||u|| <= r}`, where those
stored values are interpreted as exact real numbers. Radius must be positive;
the interval determinant must exclude zero. The caller supplies a finite,
nonzero local direction for the surface witness.

The implementation requires IEEE-754 binary64 without excess evaluation precision,
round-to-nearest, gradual underflow, correctly rounded binary64 square root, and no change in the calling
thread's floating-point environment during the query. It rejects unsupported
compiler/environment combinations, non-default rounding, non-finite inputs,
overflow and a determinant enclosure containing zero. The interval arithmetic,
its caller, the scene entry point and finite-value checks are compiled without
fast-math and floating-point contraction on supported GCC/Clang builds.

## Enclosure construction

Each interval operation evaluates its binary64 endpoints and steps one
representable number outwards. Under the assumptions above, the resulting
interval contains the exact real result. Multiplication and division evaluate
all endpoint combinations; division rejects intervals containing zero. Square
and square root intersect their domain with the known nonnegative quantities
used here. An overflow, invalid intermediate or unavailable environment returns
an explicit failure, without changing the caller's outputs.

The supplied direction `v` defines the exact surface witness
`w = c + r A v / ||v||`. Interval evaluation encloses every component of `w`
and hence encloses `||p-w||`. The upper endpoint is an upper bound on the
minimum surface distance, irrespective of whether this witness is closest.

For an exterior lower bound, any finite nonzero vector `n` gives the supporting
half-space `n dot (x-c) <= r ||A^T n||`. When the interval evaluation proves
`n dot (p-c) - r ||A^T n|| > 0`, its positive lower endpoint divided by an
upper bound for `||n||` bounds the distance from below. The implementation tries
the represented result of `p-c` and a direction estimated from inverse-interval
midpoints and the witness seed. These are only direction choices; neither needs
to be an exact surface normal because each plane is checked using intervals.

For an interior lower bound, interval cofactors and determinant enclose the
inverse of the stored `A`, without assuming the separately cached inverse is
exact. Let `x = A^-1 (p-c)` and `M` be an upper bound for `||A^-1||_F`. If the
interval upper bound for `||x||` is less than `r`, the point is inside and
`(r - ||x||) / M` is a lower bound on its world-space distance to the surface.
This follows from the triangle inequality and `||A^-1 d|| <= M ||d||`.
Another lower bound uses interval upper bounds for `||g||` and the spectral
norm of `Q`, then solves the scalar inequality
`q(p+d) <= q(p) + 2 ||g|| ||d|| + ||Q||_2 ||d||^2`. The spectral bound is
the maximum absolute row sum of the symmetric matrix `Q`.

A tighter interior lower bound encloses `Q = A^-T A^-1` and `g = A^-T x`.
For a chosen `lambda` above an interval-verified spectral upper bound for `Q`,
`P = lambda I - Q` is positive definite. For any candidate `z`, completing
the square bounds `g^T P^-1 g` above by
`2 g^T z - z^T P z + ||g-Pz||^2 / delta`, where `delta` is a positive lower
bound for the smallest eigenvalue of `P`. Interval evaluation encloses that
expression; an approximate fixed-point iteration proposes `z` but is not
trusted for the inequality. The implementation tries a finite set of lambda
values and retains only bounds that interval arithmetic verifies.

The side result is inside or outside only if an interval comparison proves it;
otherwise it is indeterminate. The returned lower bound can be zero and the
upper bound can be loose. `classifyPointCertified` returns boundary only when
`upper <= tolerance`, returns the proven side only when `lower > tolerance`,
and otherwise returns `Indeterminate`. These rules include the threshold, so a
rounded comparison cannot silently move a point across the tolerance band.

## Minimum-distance enclosure

For `B = r A`, `d = p-c`, `H = B^T B` and `g = B^T d`, the squared minimum
distance is the minimum of `||B u-d||^2` subject to `||u||=1`. For any
`lambda` such that `P = H + lambda I` is positive definite, the corresponding
Lagrange dual value `||d||^2 - lambda - g^T P^-1 g` is a lower bound on that
minimum. An approximate solver proposes a surface point and lambda; neither is
trusted as a bound.

Interval-evaluated leading principal minors prove positive definiteness. The
interval determinant divided by an upper bound for `||P||^2` bounds the
smallest eigenvalue from below. An interval residual certificate at an arbitrary
candidate bounds `g^T P^-1 g` from above, giving a verified squared-distance
lower bound. The normalized surface witness gives the upper bound described
above. `encloseMinimumBoundaryDistance` reports `Valid` only if the verified
enclosure is no wider than the caller's requested absolute width; otherwise it
returns `Indeterminate` and leaves all outputs unchanged. The dual can be
inconclusive near singular or ill-conditioned cases, which is not a miss.

## Remaining scope

The normal positive-tolerance scene-sphere path now uses the certified query and
returns `Indeterminate` if it cannot decide. Zero-tolerance legacy queries are
unchanged. The nearest-distance solver's estimated error is not a certified interval.
Tighter enclosures, independent auditing of floating-point assumptions, other
primitive families, and CSG propagation remain open in stages E and F.

The intrinsic `Sphere::classifyPoint` uses outward intervals for positive
tolerance as well. Its legacy integer `doContainmentTest` cannot express every
typed failure, so it raises an explicit exception instead of silently mapping
uncertainty to outside. This guarantee does not cover every legacy entry point:
POV's existing integer CSG and bounding paths retain their own policies.
Stage F/G integration must audit those paths and must not map an indeterminate
typed result silently to inside or outside.
