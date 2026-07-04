# VITRAL / povCpp parallel learning experience

## Statement of intent

This document is a **declaration of intent**, not a work plan with dates or
phases. It records why two source trees are being developed side by side, what
has already converged between them, and the direction future changes should
take to keep reconciling them. Use it as a compass for individual changes
(commits, small refactors), not as a checklist to execute in order.

## Two trees, one direction

Two source trees are being developed in parallel:

1. [VITRAL](https://github.com/OscarChavarro/vitral) — the target architecture:
   a general-purpose toolkit (`vsdk`) meant to centralize rendering primitives
   across several client applications and language ports (C++ and Java).
2. `povCpp` (this repository) — a faithful C++ adaptation of POV-Ray 1.0
   (1991/1992), whose design objectives are recorded in
   `doc/designObjectives.md`.

Both trees are studied continuously, and whenever a concept is understood well
enough in one of them, it feeds the other:

- What is learned studying VITRAL improves povCpp.
- What is learned studying povCpp improves VITRAL.

Once a concept is understood completely and at good quality, it is meant to be
integrated into VITRAL — the architecture that centralizes shared components —
and that same integrated version is mirrored back into povCpp. The end state
is povCpp shrinking down to an application that depends on VITRAL as a
library, with no duplicated implementation of shared primitives left in this
repository.

## The `base/` folder

`povCpp/base` holds the subset of VITRAL's `vsdk` toolkit that povCpp already
depends on. It is kept in manual sync with the VITRAL repository at
https://github.com/OscarChavarro/vitral — when a class in `base/` is improved
here, the same improvement is expected to be ported back upstream, and vice
versa.

Structures/classes already unified through `base/` include:

1. The `::java` namespace layer, offering JDK-style common classes such as
   `ArrayList` and `Math` (`base/src/main/java/**`).
2. The VITRAL linear-algebra layer, with classes such as `Vector3Dd` and
   `Matrix4x4d` (`base/src/main/vsdk/toolkit/common/...`).
3. The solid-texture layer based on Perlin noise
   (`base/src/main/vsdk/toolkit/media/solidTexture/procedural/ProceduralNoise.*`).
4. The polynomial-solver layer (`base/src/main/vsdk/toolkit/numericalAnalysis`).

`base/src/main/vsdk/toolkit/environment/geometry` currently only carries
`element/Ray.h`, `element/Ray.cpp` and `element/Intersection.h` — the ray and
geometric-hit primitives already shared with VITRAL. VITRAL's own
`environment/geometry` package additionally has `curve`, `geometricProcessing`,
`surface` and `volume` subpackages that are **not yet** mirrored into
`base/` — that gap is exactly the next phase described below.

## Next phase: `environment/geometry`

The next unification target is the `environment/geometry` package. The
concrete objectives are:

1. Express povCpp's `src/environment/geometry` in a shape as close as
   possible to VITRAL's equivalent package.
2. Identify operations that exist in povCpp but not in VITRAL, so they can be
   migrated to VITRAL in the near future.
3. Move those operations into VITRAL, so VITRAL eventually absorbs the
   `environment/geometry` package entirely — at which point povCpp's own copy
   of that package can be deleted and povCpp becomes an application built on
   top of the VITRAL library.
4. Select whichever implementation is more efficient between the two: where a
   VITRAL implementation is superior, povCpp adopts it; where povCpp's
   implementation is superior, it replaces the VITRAL one.
5. Preserve VITRAL-only operations that do not exist in povCpp, for example
   the ability of a geometry to export a `PolyhedralBoundedSolid` model.
6. VITRAL also supports painting geometries through JOGL/OpenGL and maintains
   1:1 parity between its C++ and Java ports, plus persistence to and from
   several file formats. Any change made to shared VITRAL code must preserve
   that port parity.
7. Add CSG support to VITRAL rays, for both the De Morgan's-laws strategy and
   the Roth/ray-segment strategy that povCpp already implements.

A detailed, verified comparison of the current intersection/geometry models —
covering rays, hit records, transform ownership, scene bodies, CSG,
containment, detail masks, renderer configuration, bounding information and
camera snapshots — is kept in `doc/vitralNormalizationAnalysis.md`. That
document's section 14 ("Required alignment work") is the authoritative,
itemized list of what needs to be decided before geometry classes can
converge; this document only summarizes the direction, not the details.

Before moving `Geometry` classes to VITRAL, several supporting models must
first be reconciled, in particular `Material` and the hit-record model
(`RayHit` vs. `IntersectionCandidate`/`PovRayHit`, see
`vitralNormalizationAnalysis.md` section 3). The `PovRayMaterial` model, the
`io` layer, and the render/`bakedScene` layer are considered povCpp
application-specific and are **not** planned to move to VITRAL for now.

## Worked example: unifying the statistics model

Statistics are a good concrete illustration of the kind of divergence this
effort needs to resolve, because both trees already solve the same problem —
counting raytracing work — with genuinely different designs:

- **povCpp** (`src/common/statistics/Statistics.h`) is an instance-based
  counter bag: per-primitive test/success pair counters (sphere, box, blob,
  plane, triangle, quadric, poly, bicubic, height field), bounding/clipping
  region counters, shadow/reflected/refracted/transmitted ray counters, pixel
  counters and elapsed time. Each render task owns one `Statistics` instance,
  and a dedicated constructor (`Statistics(ArrayList<Statistics*> *partsPerThread)`)
  sums per-thread instances back into one report after a `-parallel` join.
  It embeds a `SolidTextureStatistics` sub-object for texture-specific counts.
- **VITRAL** (`base/src/main/vsdk/toolkit/common/statistics/RaytraceStatistics.h`)
  is a static, event-based recorder: `recordPrimaryRay()`,
  `recordShadowRay()`, `recordReflectionRay()`, `recordSceneTraversal()`,
  `recordObjectIntersectionTest()`, `recordRayWithT()`,
  `recordRayHitInstance()`, `recordHitInfoClone()`,
  `recordGeometryDetailComputation()`, gated by `isEnabled()`, with a static
  `printSummary()`. `SolidTextureStatistics` already exists as its own class
  in VITRAL too.

Neither model is a strict subset of the other: VITRAL's event taxonomy is
coarser per-primitive but already accounts for allocation-adjacent events
(hit-info clones, detail computation) that povCpp does not track; povCpp's
per-primitive test/success pairs are finer-grained but tied to instance
ownership for thread aggregation, which a static recorder cannot express
without its own thread-local/reduction design.

The intended direction — not yet designed in detail — is:

1. Agree on one shared event taxonomy that a static recorder (VITRAL style)
   can expose, keeping per-primitive granularity as parameters/tags rather
   than one method per primitive, so adding a primitive does not require a
   new method on both sides.
2. Decide the ownership model for multi-threaded aggregation once, and make it
   the shared answer for both trees — povCpp's parts-summing constructor
   pattern is the only one of the two that already solves this, so it is the
   natural starting point rather than a from-scratch design.
3. Keep `SolidTextureStatistics` as the shared sub-model; it is already
   independently present, unchanged, in both trees.

This same three-step shape (agree on the shared taxonomy → agree on the shared
ownership model → keep whatever sub-model is already common) is the template
to reuse for every other model in section 14 of
`vitralNormalizationAnalysis.md`, not just statistics.

## Divergences discovered along the way

- **Scene baking.** Studying povCpp surfaced a "baked scene" layer
  (`src/render/bakedScene`) built up through several rounds of performance
  work (coefficient baking, `bakedScene` rebuild, `RaySharedCache`, fused
  kernels). VITRAL has no comparable concept today. This is intentionally out
  of scope for the geometry-unification phase; see `doc/performanceNotes.md`
  for the performance-driven history behind it.
- **Bounding volume hierarchies.** Neither VITRAL nor the original POV-Ray
  model implements BVHs for scene traversal. This is identified as
  future work, independent of the VITRAL/povCpp reconciliation.
- **Shaders.** Shading is expected to eventually exist in three forms: CPU
  compute (current povCpp/VITRAL state), and GPU compute in both GLSL and
  SPIR-V (see `doc/designObjectives.md` objective #5). This third leg is
  explicitly out of scope for now and is not being worked on yet.
