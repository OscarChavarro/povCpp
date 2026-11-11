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
preserves direction magnitude; `t` is not necessarily a distance. Use
`Ray::fromAffine(origin, direction, t)` to construct a ray without normalization.
`withOrigin`, `withDirection`, `withT`, and `copyOf` preserve every component they
do not explicitly replace. Setters also preserve the supplied direction. Legacy
constructors retain normalization for existing callers that expect a unit ray.

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

The remaining geometry queries and consumers must migrate to the affine/interval
contract in the following stages. Boolean combination is stage E; the two
existing POV evaluators have not been switched to this API.


## Sphere first-hit and scene query contract

Both legacy `Sphere::doIntersectionFirstHit` overloads now use the same
`intersectionRoots` kernel as crossing enumeration and interval classification.
The native `SimpleBody` translation-only sphere fast path also uses that kernel;
its independent sphere formula has been removed. An interior ray selects the
positive exit instead of being reported as a miss.

The sphere overload taking `RayCrossingQuery` selects the first admissible root
inside the explicit inclusive interval. Unlike a solid interval classification,
a surface query may have equal endpoints and return that single event. A clipped
interval can select the far root when the near root precedes its lower bound.
The old overloads query `[0,+infinity)` and continue to treat `Ray::t` as stored
ray data, not an implicit maximum distance. As with existing Boolean nearest-hit
APIs, output data is only meaningful when the return value is true; this overload
does not provide the typed failure statuses of solid classification.

Deferred detail preserves the original affine parameter and ray direction.
`SimpleBody` preserves the ray when translating and when exporting a world-space
hit. Its general TRS path still explicitly normalizes the local direction and
converts `t` back using that normalization factor. This increment verifies sphere
queries through those paths; it does not claim affine support for every legacy
primitive or remove the remaining limitations of scene placement.

Eight additional native tests cover copy semantics (including signed zero),
legacy normalized construction, first-hit/crossing agreement, an interior exit,
clipped roots, tangent and distance-only results, translated instances, negative
nonuniform scale, and the analytic exit of a shadow ray biased into a sphere.

The last case deliberately exercises a visible consequence of correcting the
mathematics: a ray starting just inside a sphere can occlude a light. The old
near-root-only code missed that exit. Appearance policy remains in the renderer;
the geometry does not inspect a shadow-ray flag to recover the old behavior.

## Typed bounds (initial implementation)

`Geometry::getBounds(GeometryBounds&) const` returns a caller-owned local-space
result with an explicit `Unsupported`, `Empty`, `Finite`, `Unbounded`, or
`Indeterminate` kind. Finite bounds contain owned minimum and maximum coordinates;
invalid, reversed, or non-finite coordinates are rejected as indeterminate.
Changing kind clears the coordinates, so a reused result does not expose stale
finite values as a new result. Consumers must inspect the kind before the values.

`Sphere` supplies its exact local axis-aligned bounds for a positive finite
radius. Invalid sphere parameters yield `Indeterminate`. Other geometry types
currently return `Unsupported` by default; the legacy `getMinMax()` interface
remains available to existing consumers. This first increment does not infer
unbounded or empty semantics from legacy arrays and does not yet replace scene or
acceleration queries. No STL containers, namespace blocks, or structs are added.

## Detail availability (initial implementation)

`RayHit` now keeps the requested mask separately from an available-detail mask.
`isDetailAvailabilityKnown()` must be checked before interpreting that mask:
legacy producers leave availability unknown. A producer calls
`beginDetailEvaluation()` and marks only fields actually calculated and finite.
Reset/clear returns the result to unknown; cloning preserves the availability
state. The mask does not allocate storage or depend on application materials.

The sphere is the first migrated producer. For example, at a pole its current UV
and tangent calculation can be non-finite; those fields are not reported available
although they were requested. Existing fields and rendering behavior are retained.
Other geometries and instance/CSG detail propagation remain to be migrated, so an
unknown result must not be interpreted as a definitive absence of detail.

## Generic query contracts

`Geometry::queryFirstHit` accepts an inclusive `RayCrossingQuery` and returns
`Hit`, `Miss`, `Unsupported`, `InvalidInput`, or `Indeterminate`. It can populate
a `RayHit`, a `RayBoundary`, both or neither. Non-hit results clear the supplied
outputs. `Geometry::classifyPoint` distinguishes inside, boundary, outside,
unsupported, invalid and indeterminate. `emitRayCrossings` can likewise report
invalid or numerically indeterminate input separately from a supported empty
sequence. Sphere implements these operations with the shared root kernel; legacy
methods remain available. Unmigrated geometry explicitly reports `Unsupported`.

A `RayBoundary` carries the parameter, borrowed leaf geometry, orientation and
an optional `RayInstancePath`; `RaySolidBoundary` remains a source-compatible
alias, not a second implementation. `RayCrossing` shares the same boundary value
and adds event kind. Its existing body/material pointers are a transitional scene
compatibility bridge: generic sphere events do not bind application materials.
A path holds an optional surface index and an extensible sequence of instance
identities and copied child/parent transformation pairs, ordered leaf outward.
Adding an invalid instance or querying an invalid index reports failure. The
caller supplies mutually inverse, valid matrices; the path does not silently
invent an inverse. Identities and the leaf are borrowed and must outlive use.

`SimpleBody::classifyRay` transforms the ray without renormalizing and appends its
instance transform to each boundary. Zero or non-finite scales and non-finite
placements are invalid; nonzero scales at or below the legacy conditioning cutoff
are indeterminate. The decomposed scene type verifies that its rotation is
orthogonal and consistent with its inverse within `1e-8`; singular, inconsistent
or shear-like matrices are indeterminate rather than silently treated as TRS.
Bodies that cannot expose the decomposed placement return unsupported. For point containment,
a positive world-space tolerance is converted exactly under uniform scale;
nonuniform scale returns indeterminate rather than approximating a scalar local
tolerance. The general affine scene model and the existing POV bridge are migrated
in later stages.

`GeometryQueryWorkspace` is caller-owned and not shared concurrently. It contains
reusable result values and a dynamically growing pool of nested solid scratch
values. Acquired pointers remain stable across pool growth; scratch must be
released in reverse order, and incorrect release is reported. `clear()` invalidates
active acquisitions and clears results while retaining pool storage. Copies are
prohibited. Separate workers use separate workspaces; tests exercise parallel use,
32 nested scratch values and 32 provenance steps. The implementation uses
`java::ArrayList` with `.h` declarations and `.txx` only in implementation files.

These are the phase B contracts and a tested sphere/scene reference implementation.
They do not assert that the full primitive catalog, legacy scene adapters, or either
CSG evaluator has already migrated. ON-contact extensions, coincident-event CSG
combination, and regularization remain part of the CSG stage; no exact arithmetic
claim is made for tolerance heuristics.
