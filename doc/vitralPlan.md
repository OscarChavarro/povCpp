# Geometry and scene convergence between VITRAL and PovRay

## 1. Objective

Provide **a single implementation of each mathematical geometry and a shared scene
model**, usable by VITRAL applications—particularly `ShadersExample` and
`RaytracingOfflineExample`—and PovRay. Add the operations VITRAL needs to express
both PovRay's CSG heuristic and Roth's ray classification and interval combination
algorithm.

VITRAL is a toolkit and a testbed for algorithms from the literature. It should
provide mathematical objects, operations, and selectable algorithms; each
application decides how to compose them, which materials to evaluate, how to
generate rays, and how to produce its image. A historical PovRay decision does not
automatically become a property of geometry. Likewise, an existing naive VITRAL
implementation is not considered definitive simply because it is already there.

The goal is not to maintain two geometry families under a common interface.
`Sphere` and `RayCastingSphere`, or a “simple” geometry and a “POV” geometry
representing the same mathematical object, must not coexist permanently.
Alternative algorithms for an operation may coexist as documented, reusable
strategies without duplicating the geometric object or its state.

The `./raycasting` directory must disappear. Each component will have a single home:
VITRAL for generic functionality, PovRay for application-specific functionality.
Renaming or moving the entire library without resolving its duplication does not
complete this plan.

**Scope of this document:** planning. Future implementation will require
coordinated changes to `vitral/cpp/base`, its consumers and examples, and PovRay.

## 2. Starting point

- VITRAL contains `Geometry`, primitives, surfaces and solids, `SimpleBody`,
  `SimpleScene`, `SimpleSceneSnapshot`, cameras, lights, and materials. Its
  renderers and examples consume these types and some primitive-specific paths.
- PovRay has a local extension in `./raycasting`, containing geometries, bodies,
  CSG operands, bounds, crossing queues, and two Boolean strategies.
  VITRAL does not depend on this library.
- Shared query contracts exist, but their semantics are not fully unified:
  detail representations, numerical policies, TRS and affine placement, and
  equivalent geometry variants coexist.
- `ShadersExample` has software and OpenGL paths tied to the sphere. Its model,
  snapshot construction, and drawable representation generation are in scope;
  compiling the executable alone is insufficient.
- `RaytracingOfflineExample` loads MIT scenes and uses `SimpleRaytracer` or
  `ParallelRaytracer`. It must preserve both paths and support examples of the
  added geometries without requiring the `.pov` parser.
- PovRay maintains its parser, materials, shaders, and a derived `bakedScene`
  representation. These consumers must adapt to the final model.

The names `SimpleSolid` and `PovraySolid` below are design alternatives for **scene
elements**, not assertions that types with those names already exist. The current
concrete integration point is `SimpleBody`.

## 3. Responsibility boundaries

| Component | Destination | Responsibility and boundary |
|---|---|---|
| Geometries and intrinsic data | VITRAL `environment/geometry` | Radii, coefficients, domains, meshes, height samples, and implicit fields; one implementation per mathematical object |
| Intersection, containment, and geometric detail | VITRAL | Queries independent of materials, the parser, POV quality settings, and shading ray categories |
| CSG algorithms | VITRAL, alongside geometric processing | Boolean tree and evaluation strategies over the same primitives; explicit references and limitations |
| Instance transforms and scene composition | VITRAL `environment/scene` | Shared placement, identity, resources, lifetime, queries, and snapshots |
| PovRay-specific scene element | PovRay, if needed | Per-object texture rules, quick color, `noShadow`, material bindings, and construction compatibility |
| POV materials, pigments, appearance normals, and shaders | PovRay | Application-specific appearance model and evaluation, without reverse dependencies from VITRAL |
| `.pov` parser, defaults, symbols, declarations, and syntax order | PovRay | Construction of the shared model and conversion of application data |
| Renderers and application policies | Their application or corresponding rendering module | Sampling, antialiasing, secondary ray generation, shadows, media, and algorithm selection |
| Geometric algorithms used by `bakedScene` | VITRAL when generic | A single mathematical kernel; renderer-specific organization and scheduling may remain in PovRay |
| Image loading for heightfields or textures | Corresponding IO module | Geometry receives decoded data; it does not interpret GIF, TGA, or POV syntax |

Inheritance or implementation of a common interface by `SimpleSolid` and
`PovraySolid` is acceptable in **scene**. Both will reference exactly the same
`Geometry` types. A scene specialization must not hide a copy of the intersector,
solver, or intrinsic geometry data.

Add interfaces only when they separate necessary responsibilities. Do not create
an interface per application, renderer, or CSG strategy that forces every primitive
to be implemented twice.

### C++ implementation conventions

Use the `java::` abstractions in VITRAL base for data structures and their
operations, including `java::ArrayList` instead of STL containers. Include template
`.h` declarations from headers and `.txx` definitions from implementation files.
Each new class belongs in its own `.h`/`.cpp` module. Keep helpers and state as
class members; do not introduce namespaces, structs, or standalone global
functions/variables. Reuse the existing `java` package without introducing new
namespace blocks. These conventions also apply to new test helpers.

## 4. Mathematical Geometry contract

### 4.1. Required operations

Review and complete the `Geometry` API and existing mathematical specializations
—surface, solid, etc.—before introducing new hierarchies.

| Operation | Required contract | Consumers |
|---|---|---|
| First hit | First admissible event within an explicit ray interval; an efficient path without requiring all crossings to be materialized | Existing renderers, picking, visibility |
| All intersection events | Ordered parameters, boundaries, and multiplicity/contact where applicable; coverage of the requested interval | CSG heuristic, Roth, geometric analysis |
| Containment | Inside, outside, or boundary for a solid, with defined tolerance and domain | CSG heuristic, initial interval state, validation |
| Ray–solid classification | Inside/outside segments and their boundaries, including the state at the start of the interval | Roth CSG |
| Deferred detail | Point, geometric normal, parametric/UV coordinates, and tangent where defined; explicit availability | Both rendering approaches and geometric operations |
| Bounds | Empty, finite, or unbounded, with a conservative volume and defined result lifetime | Acceleration, scene, visualization |

Separate **surface hits** from **solid membership transitions**. A tangent contact
may be visible to a surface query without changing the inside/outside state.
A plane as a surface and a half-space as a solid are not interchangeable. An open
mesh or patch must not acquire a fictitious interior to satisfy volumetric CSG.

Capabilities may be declared within the common contract or through the existing
mathematical hierarchy. “Unsupported,” “no hit,” and “numerically indeterminate
result” must be distinguishable. `Unclassified` may be an intermediate surface
result; it is insufficient for combining solid intervals and claiming correctness.

### 4.2. A single geometric kernel

First-hit and all-crossing operations will share root/candidate solving and
geometric evaluation. Early exits, event sinks, and specialized traversals are
allowed; independent copies of the formulas are not. Do not construct all
crossings through repeated first-hit queries with artificial origin offsets.

Retain solution strategies from the literature when they have meaningful
differences in robustness, cost, or domain. They must operate on one geometry and
its data, have names describing the algorithm, and include tests of their
preconditions. Do not encapsulate two complete implementations behind a “PovRay
mode” flag.

### 4.3. Ray parameter and numerical policy

Define `p(t) = o + t d`, the query interval, and endpoint treatment. `t` is a
parameter, not necessarily a distance. An affine ray transform preserves this
parameter if its direction is not renormalized; any normalization must explicitly
preserve its conversion factor.

Document repeated roots, ordering, coincident event grouping, very small or large
scales, parallel rays, and singular matrices. Separate geometric tolerance,
solver tolerance, and query limits: they are not interchangeable.

World-space containment tolerance under nonuniform affine transforms is addressed
in stage E before the CSG evaluators in G. A scalar world-space tolerance generally
cannot be converted to an equivalent scalar object-space tolerance. Until E provides
an appropriate operation and policy, `Indeterminate` must be propagated; it is
never implicitly converted into inside or outside.

Geometry will not inspect `isShadowRay`, `isPrimaryRay`, quality levels, or renderer
configuration. For example, shadow-root cutoffs in blobs and polynomials must be
expressed through query limits/policies chosen by the application, without changing
the definition of the surface.

### 4.4. Attribution and detail

Geometric results will preserve leaf identity, boundary/surface locator, parameter,
and the orientation needed to evaluate detail. Instance or CSG results must allow
the local space and transform chain to be reconstructed. Boolean difference must
preserve the originating leaf and adjust its boundary orientation where necessary.

Material binding belongs to scene/application code. The CSG kernel does not receive
a `PovRayMaterial` or decide texture inheritance. Scene resolves these bindings
from event provenance.

The requested mask and detail availability are distinct concepts. Do not present
zero-initialized UVs/tangents as computed data. Normals come from geometry; bump
perturbation belongs to the appearance pipeline. Unify the detail vocabulary
without forcing unnecessary computation.

Workspaces will be reusable and local to a query/worker. Do not embed PovRay-specific
media stacks, shaders, or statistics in the mathematical ray. The provenance chain
must have an extensible representation or explicit limits that report errors;
never silently lose owners.

## 5. Two CSG algorithms, the same geometries

### 5.1. PovRay heuristic based on crossings and containment

Use the behavior of `ConstructiveSolidGeometryByMorganRules` as the initial
specification: obtaining operand crossings, filtering by membership for
intersection, and the composition/inversion used by the difference constructor.

Separate three aspects: De Morgan's Boolean identities, the numerical heuristic
for deciding membership near boundaries, and application semantics when building
the tree. The current concatenation of crossings for a union does not by itself
guarantee a regularized boundary: it may retain internal surfaces.

VITRAL will provide the geometric algorithm with its preconditions and limitations.
The application will decide whether it needs that surface selection to preserve
its behavior. Any rule specific to the POV language or renderer will remain a
composition policy in PovRay, using shared operations. Do not require equality
with Roth where the two strategies define different queries; document those
differences through examples.

### 5.2. Roth interval classification

Primary reference: Scott D. Roth, *Ray Casting for Modeling Solids* (1982),
[local PDF](references/%5BROTH1982%5D_RayCastingForModelingSolids.pdf), particularly
§3.3, Figure 8, and Table 3 for classification/combination, and §6.1 for boundaries.

Conceptually, a classification result will contain:

- `N` ordered intersection parameters.
- The surfaces that originate those boundaries.
- The classification of the `N + 1` segments delimited by them.

The evaluator will traverse the CSG tree, classify the ray against each primitive
solid, and combine child classifications through union, intersection, or
difference. Combination must preserve provenance and orientation, rather than
merely sorting hits and toggling an assumed parity.

Explicitly implement the initial state for rays starting inside a solid, clipped
intervals, and unbounded solids. Group coincident events before deciding state
changes; distinguish contact, transition, and zero-length segments. Do not resolve
these cases by always shifting the ray with a fixed application epsilon.

“Complete method” means representing and combining the full ray classification,
not just the visible surface. It does not mean attributing a universal solution
to degeneracies to the paper: §3.3 uses IN/OUT, discards point tangency for that
classification, and treats certain ON cases as IN; §6.1 explains the limitations
and an extension using neighborhood models and regularized operators.

The implementation must distinguish the paper's IN/OUT algorithm from any
extension handling ON cases. Define expected results for face/edge contacts and
declare indeterminate cases; do not call a tolerance heuristic “exact.” Containment
and segment membership must be consistent under the chosen policy.

### 5.3. Algorithm location and selection

Use a common CSG expression model with evaluators/strategies operating on it.
Primitives will not know which strategy queries them. The application or VITRAL
experiment will select the heuristic or Roth algorithm.

Ray-based CSG will not be replaced by VITRAL's existing polyhedral modeling Boolean
operations: these may coexist as different algorithms, sharing the mathematical
concepts that actually apply.

## 6. Single geometry catalog

Inventory every construction accepted by the POV parser and assign it a VITRAL
mathematical type or a composition of types. This table is the minimum; the parser
inventory must identify any additional objects.

| Family | Convergence work and exposure in VITRAL |
|---|---|
| Sphere, box | One class per primitive; shared data and kernels for first hit, crossings, containment, detail, and bounds |
| Planes and unbounded regions | Distinguish surfaces, half-spaces, inversion, and complements without replacing infinity with a large box |
| Triangles, smooth triangles, and meshes | Share intersection and data; distinguish geometric from interpolated normals and declare closure/orientability before operating as solids |
| Quadrics | Generic coefficients and domain; interior classification and conservative bounds as appropriate |
| Quartics and polynomials | Degree-four implicit surfaces and degrees supported by PovRay; shared coefficients, multiple roots, and gradients. A `Quartic` facade may specialize the degree without duplicating the solver or storage |
| Blobs | Reusable implicit field, components, threshold, evaluation, gradient, roots, and bounds; no shadow-ray semantics |
| Heightfields | Height data and domain independent of the input file; define interpolation, base/closure, and containment before treating them as solids |
| Bicubic patches | Parametric domain, intersection, derivatives, UVs, and bounds; distinguish an open patch from a solid constructed from surfaces |
| CSG and composites | Distinguish Boolean solid composition from scene instance grouping; reuse primitives and placement |
| Primitives currently exclusive to VITRAL | Keep them within the same API, with explicit capabilities; do not degrade them to accommodate only the POV catalog |

All mathematical objects originating in PovRay must be constructible from VITRAL
code, with VITRAL materials and without linking PovRay, its parser, or `raycasting`.
Class names should describe the geometry, not the application of origin.

## 7. Scene model and consumers

### 7.1. Placement and specializations

Define a scene element contract with identity, a `Geometry` reference, affine
placement/inverse, appearance resources, and access to instance detail. Evaluate
extending `SimpleBody` or extracting a minimal contract without multiplying wrappers.

`SimpleSolid` and `PovraySolid`, if needed, may implement this same contract with
different material and composition policies. Geometry must be interchangeable
between them without cloning it into another geometric type.

An authoritative affine placement must support compositions that produce shear.
TRS controls will be a way to edit that placement where meaningful, not another
state that can diverge. Preserve the ordering of geometry and object layers
required by PovRay through explicit composition. Syntax logs and texture transform
rules remain in the application.

Geometric bounding/clipping may be a generic operation; translating
`bounded_by`/`clipped_by` and their inheritance rules belongs to the parser. Define
ownership of shared geometries, materials, images, and clones, cache invalidation,
versions, and snapshot lifetime before changing destructors.

### 7.2. ShadersExample

Adapt `ShadersModel`, `SoftwareRaycaster`, `OpenGlOfflineSphereRenderer`, and the
involved OpenGL renderers to the single geometry implementation. Eliminate
constructions that replicate a sphere solely to switch backends where its
definition and an instance with its own placement can be shared.

Add geometry/fixture selection for blobs, heightfields, quartics, and CSG while
preserving the sphere's material, bump, and shader demonstrations. Software and
OpenGL paths must represent the same selected geometry.

Separate exact geometry from drawable representations. Patch triangulation,
implicit surface extraction, or CSG conversion to a mesh will be VITRAL processing
algorithms with declared resolution/error. A derived visualization mesh will not
be a second authoritative solid implementation. Do not promise pixel-for-pixel
equality between rasterization and ray tracing.

### 7.3. RaytracingOfflineExample and PovRay

Preserve MIT scenes and both offline execution paths. Provide fixtures or
programmatic scene selection for the expanded catalog; extend MIT only if its
format warrants it, without making the example depend on `.pov`.

The POV parser will directly construct VITRAL geometries and compatible scene
instances. It will resolve materials, defaults, and language decisions outside
primitives. POV shaders will retain their model; VITRAL renderers will retain
theirs. Unsupported materials must be explicitly rejected.

Review `SurfaceRayIntersection`, type-specific optimizations, render caches,
`bakedScene`, and detail callbacks. The renderer may retain buffers and baked data,
but not a second intersection formula or another editable graph. Camera, sampling,
antialiasing, and image output remain consumer decisions. Rendering parameters
will be separated from geometric content.

## 8. Execution sequence

Each future stage must leave both projects buildable. Record the compatible pair
of revisions and keep changes reversible. Temporary facades will contain no
duplicated algorithms and will have an explicit removal stage.

| Stage | Deliverable | Exit condition |
|---|---|---|
| A. Specification and baselines | Complete inventory of types/operations and consumers; mapping of every `raycasting` file to VITRAL or PovRay; numerical corpus and baselines for all three programs | Every component has a unique destination, an identified policy, and an acceptance test |
| B. Mathematical contracts | Minimal API for hits, events, solid classification, containment, available detail, bounds, and workspace; provenance model | Interior, boundary, tangency, degeneracy, and transform cases defined and tested without depending on PovRay |
| C. First vertical slice | A single VITRAL sphere used by ShadersExample, the offline example, and PovRay; sufficient scene support and bindings for all three | The `raycasting` sphere variant disappears; first hit and crossings share a kernel, and both ShadersExample backends work |
| D. Basic primitives and composition | Unique box, plane/half-space, triangle, and quadric implementations; authoritative affine placement, ordinary bounds, common CSG structure and zero-tolerance primitive crossings | The basic primitive paths and their placements are shared and tested, including provenance and explicit indeterminate states. The remaining scene-policy and catalog cases are recorded in F and do not block this stage |
| E. Containment tolerance under affine transforms | Certified world-space distance and containment operations for affine spheres, with explicit failure states and an efficient zero-tolerance path; expose the operations needed by both CSG strategies on VITRAL and supported POV sphere leaves | The original ellipsoid regression passes unchanged; analytical and independent numerical tests cover placement and numerical edge cases. Typed operations preserve uncertainty and are available to both strategies; full composition and evaluator adoption belong to F/G |
| F. Finish tolerance and scene-policy integration | Extend E's certified operations to remaining primitives and scene policies, and propagate typed results, attribution and conservative bounds through composed scene structures | The affected scene paths preserve unsupported, indeterminate and singular states, provenance and zero-tolerance behavior; future geometries meet these contracts as introduced in H |
| G. CSG evaluators | Adopt the shared composition and query contracts in the POV heuristic and Roth evaluators, including their distinct classification, provenance and boundary policies | Tests cover both algorithms and their legitimate differences; no primitive requires an implementation per strategy. Any indeterminate result is propagated or handled by an explicitly specified alternative, never silently classified |
| H. Complete catalog | Generic blobs, heightfields, quartics/polynomials, meshes, and patches; fixtures and visualization representations | All POV objects available in VITRAL and consumed by examples without a PovRay dependency |
| I. Final scene model and consumers | Scene contract, minimal specializations, separate materials, adapted snapshots, parser, renderers, and bake | A single geometric graph, consistent placement, and shared kernels in optimized paths as well |
| J. Removal and completion | Relocate remaining components; remove transitional wrappers, algorithm copies, and the `raycasting` target/directory | Full gates and uniqueness/dependency audit pass, with no components awaiting a destination |

Stage D is complete for the basic-primitive scope above. The explicitly listed
scene-policy and future-catalog follow-ups remain open in F; the evaluators and
their offline CSG examples remain in G.

CSG adoption is divided by responsibility; completion of an earlier subphase
does not imply that a later evaluator is already operational:

| Subphase | Scope | Status and exit condition |
|---|---|---|
| E.1 — Primitive query contract | Provide certified distance and containment operations, explicit uncertainty, and the zero-tolerance path for affine spheres. Make these operations available through VITRAL scene bodies and supported POV sphere leaves, including placement and inversion. | **Complete.** The original ellipsoid regression and independent numerical tests pass; unsupported or indeterminate cases remain explicit. This subphase supplies the operations, not composite evaluation. |
| F.1 — Composition and scene policies | Extend the shared operations to other placed primitives and POV clipping/bounding policies. Propagate typed results through scene instances and CSG composition, including attribution, difference orientation and conservative tolerance-aware culling. Audit legacy integer adapters and prevent silent conversion of uncertainty. | **Pending.** Composition and scene-policy paths preserve the contract end to end; unsupported and indeterminate cases are explicit. |
| G.1 — Evaluator adoption | Integrate the shared composition and typed queries into both the original POV heuristic and Roth evaluator, with their respective event combination, boundary, provenance and uncertainty policies. | **Pending.** Both evaluators are exercised against shared primitives and compositions, with their documented differences and no silent classification of uncertainty. |

Stage E is complete. The optional numerical estimator remains explicitly
approximate; certifying or using its error as a guarantee is an F follow-up,
not a condition for the already-certified enclosure delivered in E.

The partially completed D work deferred specifically to F consists of:

- Positive world-space tolerance for the shared box, half-space and quadric
  containment paths when placement is nonuniform or sheared, and for POV sphere
  bodies with clipping or owned bounds. Unclipped native-sphere scene queries
  are covered by E. Quadric point containment currently supports zero tolerance only.
- Propagating that tolerance and its indeterminate outcomes through instance and
  common CSG composition, including difference orientation and provenance.
- Conservative bounds and culling decisions that depend on the same world-space
  tolerance after affine placement; uncertain cases must not be culled.
- Applying the placement, bounds and query contracts to future geometry families;
  complete each family as it is introduced in H, without blocking D on objects
  that are not yet part of its basic primitive catalog.
- Complete typed crossing and clipping/bounding-region policies for POV bodies
  that still use the historical scene traversal, preserving attribution and
  explicit failure states. Integrate those results with the G evaluators.
- Support arbitrary affine edits of POV bodies with separately owned bounding or
  clipping regions, and richer decomposed editing of arbitrary affine placements,
  once ownership and editor semantics are defined. The current APIs explicitly
  reject these unsupported edits rather than silently changing only part of a scene.
- Audit the optional approximate nearest-distance estimator's fixed error value
  before using it as a guarantee. Certified queries instead use interval-verified
  bounds and return an explicit indeterminate or unsupported result when needed.

Continue and close all independent D work before E: primitive uniqueness,
zero-tolerance solid queries, authoritative affine placement and inverse,
ordinary shared bounds, structural scene/CSG composition, and their tests.
Before starting E, review the remaining D items and finish anything that does not
actually require E. Do not defer an item to F merely because it is incomplete.

Begin stage E by constructing an ellipsoid as a nonuniformly scaled sphere in a
scene and writing a unit test against independently established world-space
expectations. Cover points inside, outside and on both sides of the tolerance band,
including a positive tolerance. Run it before implementing the fix and record the
expected failure; retain those same assertions until it passes. Include multiple
axes and an anisotropic scale so the test cannot pass by dividing the tolerance by
one arbitrary scale component. Follow with rotated, reflected and sheared cases,
and explicitly test singular and numerically indeterminate placements.

Stage E is complete for its stated affine-sphere distance and tolerance scope.
The original ellipsoid regression was observed failing before the fix; its
unchanged assertions now pass. Positive-tolerance intrinsic and placed-sphere
queries use outward-rounded interval certificates and explicitly preserve
`Indeterminate`, invalid and unsupported cases. The legacy integer sphere
adapter raises an explicit error when a typed result cannot be represented.
The existing zero-tolerance fast path remains unchanged.

`AffineEllipsoidMinimumDistance` and
`SimpleBody::encloseMinimumBoundaryDistance` provide a certified world-space
distance enclosure with a caller-requested maximum width. Approximate nearest
points only propose candidates; verified surface-witness and Lagrange-dual
bounds determine the result. The numerical environment assumptions, derivation,
and failure semantics are recorded in `doc/vitralCertifiedEllipsoidBounds.md`.
An inconclusive computation returns `Indeterminate` without changing outputs.

Analytical normals, interior hard cases, repeated axes, singular and
ill-conditioned transforms, and independent surface sampling and constrained
minimization tests cover rotations, reflections, shear and different scales.
The unchanged ellipsoid regression and the POV typed-binding tests preserve
uncertainty explicitly. Unclipped and unbounded POV native-sphere leaves expose
the same operations through the shared scene API, including layered placement
and inversion. Their legacy integer CSG traversal is not switched to the typed
contract in E: clipped/bounded bodies, other primitives, CSG propagation and
policy selection remain explicitly in F/G.

The optional `estimateBoundaryDistance` API still reports an *estimated*, not
certified, error. It is never used to force a containment or CSG decision; any
future use as a guarantee requires the F audit above. Further portability and
adversarial testing are valuable ongoing validation and do not turn an
indeterminate result into a definite one.

The scene foundations of stage C are consolidated in I; scene adaptation is not
entirely deferred until the end. Each family in H will be integrated individually
with its tests. Do not move all of `raycasting` as a block and call that convergence.

The stage D CSG structural scaffold does not itself evaluate expressions or
render them. In stage G, connect the shared expression to the evaluators and the
offline reader/renderer, and add `.ray` scenes demonstrating union,
intersection and difference of spheres. Verify the actual surfaces and boundary
orientation, including sequential and parallel renders; do not count silently
ignored CSG syntax or placeholder primitive renders as these examples.

During stages H–I, investigate and correct the currently failing special cases in
`RaytracingOfflineExample`: textures, bump mapping and `PolyhedralBoundedSolid`.
The `runAll.sh` script may already have been broken before this plan began and
has since been corrected. The examples themselves remain broken: scene content
appears not to be processed completely. Investigate the reader and rendering
pipeline, establish the missing behavior, and restore the examples rather than
assuming that successful script execution implies correct output.
Run `./runAll.sh --parallel` from
`testsuite/OfflineExamples/RaytracingOfflineExample` and verify those cases
visually and with focused checks. These known issues may be addressed when their
geometry, scene or render integration is reworked, but must be resolved before
the final gate.

## 9. Verification and gates

### Mathematical correctness

- Compare the first hit with the first admissible event of the same geometry.
- Verify containment and segment classification, including repeated roots,
  tangencies, coincident events, an interior starting point, and interval limits.
- Verify normals, difference orientation, provenance, and coordinate spaces under
  nonuniform/negative scaling, shear, and nested CSG.
- Use analytical references and examples reconstructed from Roth's paper;
  do not validate the two algorithms solely by comparing them with each other.
- Test known heuristic limitations and Roth ON cases with policy-specific expected
  results. Compare strategies only where their semantics are equivalent.
- Cover polynomial singularities and roots, blob thresholds, heightfield closure,
  open surfaces, and empty/unbounded bounds.
- Verify destruction, cloning, shared instantiation, and reconstruction with
  sanitizers; check that caches and workspaces introduce no races.

### Applications and performance

| Consumer | Mandatory gate |
|---|---|
| PovRay | Complete `./scripts/renderAll.sh`, with no omitted scenes or failures and no degradation from baseline performance; compare before/after images |
| PovRay CSG | Explicit corpus using both strategies, including CSG/Roth scripts and degenerate cases. Preserve normal-flow references. The seven recorded Roth RMSE exceptions are accepted starting conditions; reject any additional discrepant scene, missing output, or render failure |
| VITRAL | Build testsuite consumers and run relevant base tests. VGA programs depend on base and do not require a separate autonomous validation campaign |
| RaytracingOfflineExample | MIT corpus and new fixtures in sequential/parallel execution, including `./runAll.sh --parallel`; verify images and geometric queries, especially textures, bump mapping and `PolyhedralBoundedSolid` |
| ShadersExample | Software and OpenGL execution, material/bump controls, new geometries, and representation checks; record graphics environment limitations separately |

Establish baselines from the starting state before implementation. Record commands,
resolution, policies, compiler, timings, CPU, memory, and hashes. Measure without
concurrent builds and with enough repetitions to distinguish noise from regression.
Pay particular attention to per-ray allocations, full classification when only a
first hit is requested, and lost bake optimizations.

Aim for identical images when changing structure. Any necessary numerical
difference must be explained and validated against defined criteria before being
accepted; do not automatically regenerate baselines or retain a second private
kernel to hide it. Existing lighting/quality modes and materials are part of
compatibility, alongside the default render.

## 10. Source uniqueness and definition of done

VITRAL will be the authoritative source of mathematical code. While retained for
distribution or review, the local `base/src/main` copy will be a mechanical mirror
of its subset, identical in diff—not an independently maintained implementation
or a second source of headers/binaries in the same build. Do not expand that mirror
to preserve local geometry variants.

The plan is complete only when:

1. There is one authoritative implementation per geometry, reused by all three
   programs. Algorithm variants are separate from the mathematical object's
   identity and state.
2. VITRAL provides the operations and algorithms needed by both CSG approaches,
   with documented domains, limitations, and boundary handling.
3. Blobs, heightfields, quartics, and the rest of the POV catalog can be constructed
   and used in VITRAL without POV parsers, shaders, or materials.
4. Application specializations reside in scene, IO, or rendering/materials, without
   requiring duplicated primitives or permanent graph conversions.
5. `./raycasting` and its target disappear. Its entire inventory has a final
   destination, and each implementation is maintained only once.
6. No references from VITRAL to PovRay remain, nor duplicated geometric kernels in
   the POV renderer or examples. Derived representations are caches or explicit
   approximations, never another source of truth.
7. ShadersExample, RaytracingOfflineExample, all examples, and the POV gates meet
   the criteria above; public APIs and their correspondence to algorithms in the
   literature are documented.
