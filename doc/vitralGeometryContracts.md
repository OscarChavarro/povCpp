# Native solid ray classification contract

`Geometry::classifyRay(const Ray&, const RayCrossingQuery&,
RaySolidClassification&) const` is a mathematical operation in VITRAL base.
The default implementation returns `Unsupported`. `Sphere` implements the
operation using its existing `intersectionRoots` kernel. No application-specific
geometry interface, material binding, parser, or renderer is required.

## Result and ownership

For N ordered `RaySolidBoundary` entries, `insideSegments` has N+1 membership
values. The result contains the complete IN/OUT classification within the requested
interval, including a single all-inside or all-outside segment when there are no
boundaries. A boundary contains the ray parameter, borrowed originating geometry,
and a normal reversal flag for future Boolean combination. The geometry must
outlive consumers of the result. Scene is responsible for material lookup.

The result stores boundaries and membership in `java::ArrayList`, with template
declarations included from `.h` and definitions from `.txx` only in `.cpp` files.
Lists are cleared on every query without discarding their capacity. Callers may
reuse a result within a worker; concurrent queries must use separate results.
Failure leaves both arrays empty. Result storage is caller-owned.

`RaySolidQueryStatus` distinguishes:

| Status | Meaning |
|---|---|
| `Supported` | Complete result for the supported mathematical domain, including no boundaries |
| `Unsupported` | This geometry does not implement solid classification; no interior is inferred |
| `InvalidInput` | Invalid ray/domain/interval or incompatible surface-only query policy |
| `Indeterminate` | Arithmetic range prevents the implemented computation from producing a classification |

These statuses do not change the legacy surface-event API or claim that all
geometries now provide volumetric semantics.

## Parameter and endpoint semantics

The mathematical parameter is `p(t) = origin + t * direction`. Classification
preserves direction magnitude; `t` is not necessarily a distance. For the current
`Ray` API, use `setOriginAndDirection` to retain an affine direction: constructors
still normalize it. Fixing the broader ray-copy/first-hit contract is pending.

The query must have `minT < maxT`; infinite limits are supported, NaNs are not.
Boundary events at either endpoint are included. The resulting zero-length
endpoint segment stores the limiting state immediately outside the interval.
For example, a unit sphere with origin `(-3,0,0)`, direction `(2,0,0)`, and interval
`[1,2]` produces boundaries `[1,2]` and segment states `[OUT,IN,OUT]`.
Clipping that query to `[1.25,1.75]` produces no boundaries and `[IN]`.

Point tangency is a surface event but contributes no IN segment to this IN/OUT
classification, following the scope of Roth §3.3. ON/neighborhood extensions and
regularized contact policies are not implemented by this addition. The existing
`emitRayCrossings` operation continues to expose a tangent surface event.

## Query policy and numerical limits

The sphere requires a positive finite radius and a finite, nonzero ray direction.
The legacy `minimumChordSquared` and `minimumExteriorApproach` surface cutoffs
must retain their mathematical defaults (`0` and negative infinity). Discarding
real roots through appearance/compatibility policy would invalidate the segment
classification, so such queries return `InvalidInput` here. Surface queries remain
available to applications that require those cutoffs.

The implementation obtains the complete analytic sphere interval before clipping.
It does not reconstruct an interior by toggling clipped events, sampling with a
fixed epsilon, or repeatedly issuing nearest-hit queries. Floating-point range
checks report `Indeterminate`; this is not an exact-arithmetic solver or a promise
to solve arbitrary ill-conditioned inputs. Further numerical robustness remains
part of stage B.

## Verification and remaining scope

Eight native test groups cover the six analytic sphere fixtures: nonunit direction,
interior start, clipped all-inside interval, endpoint events, tangency and miss.
They also cover unbounded queries, leaf provenance, invalid intervals and cutoffs,
zero directions, arithmetic overflow/underflow, unsupported geometry, and stale-result
clearing. Additional checks verify list capacity reuse, independent copies, and
`java::Double` handling of special values. Existing tests and example image comparisons remain in force.

Stage B is partial. Shared bounds states, richer surface locators/instance chains,
detail availability, general workspace integration, affine ray copying, and a
unified nearest-hit implementation are still pending. Boolean combination is
stage E; the two existing POV evaluators have not been switched to this API.
