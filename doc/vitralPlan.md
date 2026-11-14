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

The VITRAL scene element is `SimpleBody` and the PovRay scene element is
`PovrayBody`; both implement the scene contract `SceneSolid` (§7.1).

## 3. Responsibility boundaries

| Component | Destination | Responsibility and boundary |
|---|---|---|
| Geometries and intrinsic data | VITRAL `environment/geometry` | Radii, coefficients, domains, meshes, height samples, and implicit fields; one implementation per mathematical object |
| Intersection, containment, and geometric detail | VITRAL | Queries independent of materials, the parser, POV quality settings, and shading ray categories |
| CSG algorithms | VITRAL, alongside geometric processing | Boolean tree and evaluation strategies over the same primitives; explicit references and limitations |
| Instance transforms and scene composition | VITRAL `environment/scene` | Shared placement, identity, resources, lifetime, queries, and snapshots |
| PovRay scene element (`PovrayBody`) | PovRay | Per-object texture rules, quick color, `noShadow`, inversion, material bindings, POV transform layers, `bounded_by`/`clipped_by` regions, and construction compatibility, over the shared `SceneSolid` contract |
| POV materials, pigments, appearance normals, and shaders | PovRay | Application-specific appearance model and evaluation, without reverse dependencies from VITRAL |
| `.pov` parser, defaults, symbols, declarations, and syntax order | PovRay | Construction of the shared model and conversion of application data |
| Renderers and application policies | Their application or corresponding rendering module | Sampling, antialiasing, secondary ray generation, shadows, media, and algorithm selection |
| Baking (`bakedScene`) | PovRay | Compilation of the POV scene into render records, specialized plans, render caches, and their scheduling. VITRAL provides no baking service |
| Kernels and CSG steps called by the bake | VITRAL when generic | One application-independent implementation (for example `QuadricRayExpansion`, `InfinitePlane::intersectionDepth`, `CsgMembershipRule`); the bake calls it and never copies it |
| Image loading for heightfields or textures | Corresponding IO module | Geometry receives decoded data; it does not interpret GIF, TGA, or POV syntax |

`SimpleBody` and `PovrayBody` implement the common **scene** contract
`SceneSolid` (§7.1). Both reference exactly the same `Geometry` types. A scene
specialization must not hide a copy of the intersector, solver, placement
logic, CSG structure or evaluator, or intrinsic geometry data.

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

Two evaluator families operate on the same expression and primitives and share
`CsgMembershipRule`:

- **Certified evaluators** (`CsgRothRayClassifier`, `CsgCrossingMembershipFilter`,
  `CsgPointClassification`) answer typed queries with explicit uncertainty. They
  serve VITRAL consumers and the typed queries of POV bodies.
- **Fast evaluators**, PovRay's crossing heuristic and Roth's segment merge over
  binary64 crossings without certification, serve the render paths. The certified
  heuristic costs about forty times the Roth classifier (I-3), so render paths do
  not use the certified evaluators.

The fast evaluators are generic strategies with their own cost and robustness
(§4.2), not POV policies. Their single implementation is in VITRAL, parameterized
by the caller's event type: `CsgSegmentClassification` (Roth's alternating
segments and their merge by Table 3) and `CsgCrossingSelection` (the heuristic's
selection of a crossing by its siblings' containment). The POV evaluators and the
bake call them with POV hit records and containment queries.

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

The scene element contract `SceneSolid` (VITRAL `environment/scene`) holds a
`Geometry` reference with its ownership, the authoritative affine placement and
its inverse (abstract), solid world bounds, first hit and detail (abstract),
typed point and ray queries, and a modification version. It computes the placed
queries once from geometry and placement, including the static
`classifyPlacedPointCertified` and `classifyPlacedBoxCertified`. Box
classification, solid equivalence and the boundary distances use the solid the
body denotes (`queryDenotedSolid`: by default its geometry, only for bodies that
`denotesPlacedGeometry`), so a body that wraps or inverts its geometry
supplies that solid instead of recomputing the queries. Implementations override
a typed query only to apply a scene policy (CSG, regions) around the shared
computation. Appearance stays out of the contract.

- `SimpleBody` implements `SceneSolid` and keeps TRS editing and VITRAL appearance
  (material, texture, normal map).
- `PovrayBody` implements `SceneSolid` without deriving from `SimpleBody`, so it
  carries no TRS state or VITRAL appearance it must disable. It composes POV's
  geometry and object layers into the placement, and holds POV appearance, quick
  color, `noShadow`, inversion, and `bounded_by`/`clipped_by` regions. Its
  denoted solid is the VITRAL solid of its geometry adapter with the adapter's
  inversion, when it has no regions. It lives in PovRay `src/environment/scene`. The POV `Scene`
  owns its `PovrayBody` list; from `SimpleScene` it uses lights and resources.
  VITRAL renderers draw `SimpleBody` bodies, not POV bodies.
- CSG leaves are `SceneSolid`s: `CsgBodyOperand` takes a `SceneSolid`, and VITRAL
  `CsgBody` and POV CSG build the same `CsgTree`. A POV CSG operand
  (`CsgOperand`) is a `PovrayBody` with one placement layer and the operand
  material; a nested POV CSG is an operand whose geometry is another CSG, which
  the shared evaluators traverse because the operand denotes its placed geometry
  (`SceneSolid::isStructuralTraversalSafe`). Generic processing such as
  `SolidBoundaryTessellator` takes a `SceneSolid`.

Geometry is interchangeable between both bodies without cloning it into another
geometric type. POV geometry adapters disappear: their render caches move to the
bake or the per-ray caches, and their inversion to the body.

The POV bake (`bakedScene`) compiles `PovrayBody` and `CsgTree` into PovRay render
records. It may keep composed matrices, specialized plans and per-viewpoint caches,
and calls VITRAL kernels and CSG steps instead of copying them. VITRAL renderers
have no bake.

An authoritative affine placement must support compositions that produce shear.
TRS controls will be a way to edit that placement where meaningful, not another
state that can diverge. Preserve the ordering of geometry and object layers
required by PovRay through explicit composition. Syntax logs and texture transform
rules remain in the application.

Geometric bounding/clipping may be a generic operation; translating
`bounded_by`/`clipped_by` and their inheritance rules belongs to the parser. Define
ownership of shared geometries, materials, images, and clones, cache invalidation,
versions, and snapshot lifetime before changing destructors.

A body owns the geometry, material, texture and normal map given to `setGeometry`,
`setMaterial`, `setTexture` and `setNormalMap`, and only references those given to
`setGeometryReference`, `setMaterialReference`, `setTextureReference` and
`setNormalMapReference`. Images and normal maps shared by several bodies or by a
background are scene resources owned by `SimpleScene` (`addImage`,
`addNormalMap`) and deleted after the bodies and backgrounds that use them.

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

`ShadersGeometryFixture` holds each non-sphere geometry once. The software path
ray traces a frame body that references those geometries (a `CsgBody` with
Roth's classification for the CSG sample), and the OpenGL path draws the mesh
that `SolidBoundaryTessellator` (96 cells along the longest side) or
`BezierPatchTessellator` (48 x 48 steps) derives from that same body or patch.
Textures and bump maps stay on the sphere. The `o` key and the
`--geometry sphere|quartic|blob|heightfield|csg|patch` option select the
geometry, also with `--offline` for either `--method`. Frame bodies, lights and
backgrounds are deleted with their snapshot.

### 7.3. RaytracingOfflineExample and PovRay

Preserve MIT scenes and both offline execution paths. Provide fixtures or
programmatic scene selection for the expanded catalog; extend MIT only if its
format warrants it, without making the example depend on `.pov`. The MIT format
has CSG blocks, `csg union|intersection|difference [roth|pov]` … `end`, which
nest and take `sphere`, `box`, `halfspace`, `quadric`, `poly`, `blob` and
`heightfield` operands; they build a `CsgBody` (see the G status). The stage H
families have their own lines and blocks:

| MIT syntax | Geometry |
|---|---|
| `transform m00 … m23` | Twelve numbers, the rows of a 3×4 affine map applied to the next primitive or outermost `csg` block |
| `poly <order> [sturm]`, `term <coefficient> <x> <y> <z>` lines, `end` | `PolynomialSurface` of order 2 to 7; repeated terms add up |
| `blob <threshold> [sturm]`, `component <strength> <radius> <x> <y> <z>` lines, `end` | `BlobSolid` |
| `heightfield <file> [water <level>]` | `HeightFieldSolid` read by `ReaderAltimetry` from a file resolved from the scene's directory; `water` raises the box floor |
| `patch <uniform\|grid\|adaptive\|tree\|smooth> <uSteps> <vSteps> [flatness <value>]`, 16 `control <x> <y> <z>` lines, `end` | `SubdividedBezierPatch`; not a `csg` operand |

An altimetry file is the header `altimetry <columns> <rows>` followed by the
heights row by row from z = 0, with `#` comments. Malformed lines, a
non-invertible or dangling transform, a degenerate blob component and an
unreadable altimetry file are rejected.

The classic MIT commands follow the Java reader:

| MIT syntax | Scene content |
|---|---|
| `sphere <x> <y> <z> <r>`, `box <xmin> <ymin> <zmin> <xmax> <ymax> <zmax>`, `halfspace <a> <b> <c> <d>`, `quadric <ten coefficients>` | `Sphere`, `Box`, `PlanarHalfSpace`, `QuadricSurface` |
| `cube <x> <y> <z> <size>` | Centered `Box` |
| `cylinder <x> <y> <z> <bottom radius> <top radius> <height>` | `Cone`; not a `csg` operand |
| `torus <x> <y> <z> <major radius> <minor radius>` | `Torus`; not a `csg` operand |
| `polybox <x> <y> <z> <sx> <sy> <sz>`, `appel <1\|2\|3> <x> <y> <z> [scale]` | `PolyhedralBoundedSolid` of a box or of an APPE1967 test object centered on its position; not `csg` operands |
| `triangles` … `end` with `v` and `f` lines, `quads` … `end` with `v` and `q` lines | `TriangleMesh` and `QuadMesh`; a `surface` line inside the block starts a new body sharing the vertices |
| `obj <file> <x> <y> <z> [scale]` | The bodies of an OBJ file, placed by the current rotation, the position and the scale |
| `rotation <yaw> <pitch> <roll>` | Euler rotation of every later primitive about its own origin |
| `texture <file>`, `notexture`, `bumpmap <file> [<sx> <sy> <sz>]`, `nobumpmap` | Texture and normal map of later primitives, shared scene resources |
| `background <r> <g> <b>`, `backgroundcubemap [directory]` | `SimpleBackground` or `CubemapBackground` from `entorno0.jpg` … `entorno5.jpg` (default `../../cubemaps/dorise1`) |

Relative files are looked up from the scene's directory and then from the
working directory. An unknown command, a malformed line or an unreadable file
is rejected.

The POV parser will directly construct VITRAL geometries and compatible scene
instances. It will resolve materials, defaults, and language decisions outside
primitives. POV shaders will retain their model; VITRAL renderers will retain
theirs. Unsupported materials must be explicitly rejected.

Review `SurfaceRayIntersection`, type-specific optimizations, render caches,
`bakedScene`, and detail callbacks. The POV renderer may retain buffers and baked
data (the bake is PovRay's, §3), but not a second intersection formula or another
editable graph. Camera, sampling,
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
| I. Final scene model and consumers | `SceneSolid` contract, `PovrayBody`, separate materials, adapted snapshots, parser, renderers, and the PovRay-side bake | A single geometric graph (`CsgTree` with `SceneSolid` leaves), consistent placement, and shared kernels and CSG steps in the bake as well |
| J. Retirement of `./raycasting` | Relocate the remaining components to their single home, remove algorithm copies and dead code, and remove the `raycasting` target/directory | The module is gone, VITRAL has no reference to PovRay, no class is defined in both projects, and the gates pass |
| K. POV render over VITRAL geometries | The POV scene references VITRAL geometries directly: no `RayCastingGeometry` subclass wraps a VITRAL geometry, POV CSG is a scene body over `CsgTree`, inversion lives in `PovrayBody`, and the POV-specific render state (per-viewpoint caches, historical tolerances, crossing emission) lives in the bake; the bounding-volume package moves to VITRAL | PovRay defines no `Geometry` subclass; the original POV render and the Roth render keep their gates; full uniqueness/dependency audit with no component awaiting a destination |

Stage D is complete for the basic-primitive scope above. The explicitly listed
scene-policy and future-catalog follow-ups are handled in F; the evaluators and
their offline CSG examples are delivered in G.

CSG adoption is divided by responsibility; completion of an earlier subphase
does not imply that a later evaluator is already operational:

| Subphase | Scope | Status and exit condition |
|---|---|---|
| E.1 — Primitive query contract | Provide certified distance and containment operations, explicit uncertainty, and the zero-tolerance path for affine spheres. Make these operations available through VITRAL scene bodies and supported POV sphere leaves, including placement and inversion. | **Complete.** The original ellipsoid regression and independent numerical tests pass; unsupported or indeterminate cases remain explicit. This subphase supplies the operations, not composite evaluation. |
| F.1 — Composition and scene policies | Extend the shared operations to other placed primitives and POV clipping/bounding policies. Propagate typed results through scene instances and CSG composition, including attribution, difference orientation and conservative tolerance-aware culling. Audit legacy integer adapters and prevent silent conversion of uncertainty. | **Complete.** Sphere, box, half-space and quadric leaves are certified at zero and positive world tolerance under any valid affine placement, through one shared placed-solid routine used by VITRAL bodies, POV leaves and CSG leaves. CSG expressions have conservative solid bounds and cull only proven-disjoint leaves. POV `clipped_by` point queries are a shared intersection; `bounded_by` is used only when certified to contain the body. Affine edits move owned regions atomically or are rejected. Legacy integer adapters are documented or throw. Ray classification of clipped or bounded bodies remains `Unsupported` until the G Boolean event combiner exists. |
| G.1 — Evaluator adoption | Integrate the shared composition and typed queries into both the original POV heuristic and Roth evaluator, with their respective event combination, boundary, provenance and uncertainty policies, including ray classification of POV bodies with `clipped_by`/`bounded_by` regions. | **Complete.** VITRAL provides `CsgRothRayClassifier` and `CsgCrossingMembershipFilter` over the shared expression, with the shared Boolean rules in `CsgMembershipRule`; the POV Roth evaluator uses those rules, POV region ray classification uses the Roth classifier, results carry version stamps, and `CsgBody` renders either evaluator in `RaytracingOfflineExample`. Moving the POV render paths onto the typed evaluators belongs to I. |

Stage F status (complete):

- **Complete:** conservative world-space point containment through affine-placed
  shared CSG trees and typed VITRAL/POV body operands. Outward-rounded inner and
  outer tolerance enclosures support nonuniform scale, shear and reflection.
  Exposed single-operand boundaries and single-leaf proof paths retain difference
  orientation; nested expressions, uncertainty and mixed scene bodies are tested.
- **Complete:** multi-operand point evidence with leaf paths, local query values
  and difference orientation. Typed ray leaf intervals and exact-parameter event
  groups retain all coincident contributors, affine placement snapshots and
  inversion parity. Unsupported or malformed leaves clear the whole result;
  unimplemented POV clipping and owned bounds are explicit.
- **Complete for the proven cases:** point-boundary attribution returns every
  certified coincident owner, including nested paths and difference orientation,
  while excluding cancelled or hidden surfaces. Unknown or ambiguous owner sets
  remain unavailable.
- **Complete for the proven cases (further work frozen):** exact equivalence
  recognizes affine spheres via exact Gram
  matrices, affine boxes via corresponding edges and corners, oriented planes and
  quadrics via world coefficients, and spherical/linear quadrics across families.
  The more general plane and quadric conversions require exactly representable
  intermediate coefficients; unavailable proofs remain inconclusive. Compatible
  POV bodies opt in only when inversion is represented in the core geometry and
  no clipping or owned bounds intervene. Other families, general numerical
  certificates and ambiguous boundary configurations remain open; Boolean
  combination of crossing groups belongs to G. Equivalent leaf or structurally
  safe subtree operands can reuse a sibling's certified point result with explicit
  source and identity evidence. A subtree recipient has incomplete attribution
  until its own owners can be resolved; unsupported/indeterminate cases without
  evidence remain explicit. Shared trees also recognize associative/idempotent
  union/intersection and unions of subtractors after an ordered difference
  minuend, flattening only identity-placed known structural nodes.
  Unknown scene views and expressions are not unpacked for equivalence, and
  point/ray traversal requires both to opt in when their structure fully describes
  the solid; unknown subclasses do not inherit that opt-in automatically.
  Event grouping uses equality of reported binary64 parameters, not a proof that
  nearby mathematical roots coincide. The actual evaluator policies belong to G.
- **Complete:** certified point containment for sphere, box, half-space and
  quadric at zero and positive world tolerance. Each family implements
  `Geometry::classifyPointCertified` (local closed ball) and
  `Geometry::classifyBoxCertified` (local box, `Inside`/`Outside` only).
  `SceneSolid::classifyPlacedPointCertified` maps the world ball through the
  placement with `AffineToleranceMap` and tries, in order, the identity path,
  the enclosing local box, the outer and inner balls and a two-point world
  proof; anything unproven is `Indeterminate`. `PovrayBody` uses the same
  routine for its core solid, so a placed sphere has one certificate.
- **Complete:** conservative bounds. `GeometryBounds::transformed` rounds
  outward; `inflated`, `intersecting` and `excludesBall` support culling.
  `CsgSolidBounds` encloses unions, intersects intersections and uses the
  minuend of a difference; `CsgPointClassification` culls a leaf as `Outside`
  only when its solid bounds exclude the tolerance ball.
- **Complete:** POV region policies for point queries. `clipped_by` is the
  shared CSG intersection of the core solid (`RayCastingCoreOperand`) and each
  clip; `bounded_by` is accepted only when the region is certified to contain
  the core bounds, and otherwise the query is `Unsupported`. Solid world bounds
  of clipped bodies are the intersection with the clip bounds.
- **Complete:** placement editing. `PovrayBody::setAffinePlacement` moves
  owned regions by `new * old^-1` in a two-phase, all-or-nothing step; regions
  that cannot be moved make the edit fail without changes.
- **Complete:** adapter and estimator audit. Integer predicates inherited from
  the Java port (`Geometry` default, `Box` band, `InfinitePlane` surface value,
  cones, meshes) keep Java parity and are documented as legacy predicates;
  adapters introduced during convergence (`Sphere`, `PlanarHalfSpace`,
  `QuadricSurface`, `NativeGeometryBinding`) throw instead of converting
  uncertainty. Typed scene and CSG paths never call `doContainmentTest`.
  `estimateBoundaryDistance` and `AffineEllipsoidDistance` only propose
  candidates inside `AffineEllipsoidMinimumDistance`; no decision depends on them.
- **Moved to G:** ray classification of POV bodies with regions (needs Boolean
  event combination), proof versioning (F-R2) and workspace reuse (F-R3).
- **Ongoing through H:** extend these contracts to future geometry families as
  they are adopted. Both CSG event evaluators adopt them in G.
- **Frozen:** further exact-equivalence work (additional families, general
  rotations, certificates beyond exactly representable intermediates). It
  exceeds F's exit condition. Resume an item only when a G evaluator or an H
  family demonstrably needs it.

#### Stage F audit: non-conformities and risks

A code audit of both projects at the start of the remaining F work found:

| ID | Finding | Resolution |
|---|---|---|
| F-NC1 | The CSG point classifier queries leaves through `classifyPointCertified`, which supports only spheres in both `SimpleBody` and `RayCastingBody`. Box, half-space and quadric leaves are `Unsupported` in CSG point queries even at zero tolerance; equivalence-based proof reuse only partially masks this. | F-1 |
| F-NC2 | Two levels of rigor share one name. Sphere positive tolerance uses outward-rounded intervals. `Box` and `PlanarHalfSpace` positive tolerance and the uniform-scale `SimpleBody` path (`tolerance / scale`) use round-to-nearest arithmetic and can misclassify points within rounding of the tolerance band. | F-1 |
| F-NC3 | `RayCastingBody::classifyPointCertified` reimplements the placed-sphere search with a different strategy from `SimpleBody`, so the same placed sphere can be classified differently, which violates §4.2. | F-6 |
| F-NC4 | Legacy integer adapters silently convert uncertainty. `Geometry::doContainmentTest` defaults to `OUTSIDE`. `QuadricSurface::doContainmentTest` compares the implicit value with the tolerance and maps NaN to `OUTSIDE`. `Box::doContainmentTest` uses a per-axis band that differs from `Box::classifyPoint`. `InfinitePlane::doContainmentTest` returns `-INSIDE` with an unnormalized tolerance. POV adapters (`RayCastingBox` ignoring tolerance, implicit-unit quadric/plane/sphere comparisons) are render policy and must remain byte-identical, but must be identified as such. | F-2 |
| F-NC5 | `GeometryBounds::transformed` rounds to nearest, so a placed bound can shrink by an ulp. There is no tolerance inflation and no bounds query for CSG expressions, so "conservative culling" has no certified primitive. | F-3 |
| F-NC6 | The execution record still used the stage letters from before E/F were inserted (evaluators in "F", scene in "H", removal in "I"). | Corrected |
| F-R1 | The equivalence work (dyadic arithmetic, more than 1,300 rational fixtures, structural regrouping) grew faster than the basic per-family contract. | Frozen above |
| F-R2 | Proofs, witnesses and attributions borrow scene identities without recording `modificationVersion`; stale use after an edit is detectable only by convention. | G-5: `CsgVersionStamp` on every cached result |
| F-R3 | CSG point/ray queries allocate `java::ArrayList` storage per node. Putting them on the per-ray path in G without workspace reuse risks the `renderAll.sh` performance gate. | G-5: `CsgRayWorkspace` makes Roth queries allocation-free; point classification still allocates (I) |

#### Stage F exit checklist

F is closed when all of the following hold, with the usual gates (VITRAL
examples and tests, POV integration, example captures, `renderAll.sh` with
baseline-identical images, Roth with only the seven accepted discrepancies,
mirror audit):

- [x] **F-1 Certified containment for basic families.** Sphere, box, half-space and
  quadric support certified point classification at zero and positive
  world-space tolerance under any valid affine placement (nonuniform scale,
  shear, reflection). The same implementation serves VITRAL bodies, compatible
  POV leaves and CSG leaves. A result is definite only when proven; otherwise it is
  `Indeterminate`.
- [x] **F-2 Legacy adapter audit.** Every shared integer adapter either represents
  the typed result or raises an explicit error; none maps unsupported or
  indeterminate results to `OUTSIDE`. POV historical comparisons stay unchanged
  and are documented as application policy.
- [x] **F-3 Conservative bounds and culling.** Outward-rounded world bounds, inflation
  by a world tolerance, bounds for CSG expressions (union encloses all operands;
  intersection lies within each operand; difference lies within its minuend), and a
  culling predicate that culls only when disjointness is proven.
- [x] **F-4 POV region policies.** Typed point classification for bodies with
  `clipped_by` (intersection with the clip regions) and `bounded_by` (a culling
  hint that never changes membership), preserving provenance; unsupported region
  types remain explicit. Ray classification with regions returns `Unsupported`
  and is completed by the G event combiner.
- [x] **F-5 Placement editing.** Arbitrary affine edits of POV bodies with owned
  regions either move the regions with the object or are rejected explicitly, as
  documented; no partial edit.
- [x] **F-6 One placed-sphere certificate.** POV and VITRAL bodies share one
  certified placed-sphere classification routine.
- [x] **F-7 Estimator audit.** `estimateBoundaryDistance` is never used to decide
  containment or CSG results; its error remains documented as an estimate.

#### Stage G status

- **Complete:** `CsgRothRayClassifier` implements [ROTH1982] §3.3 over the
  shared expression. It classifies every leaf with `CsgRayLeafQuery`, groups
  events with exactly equal parameters with `CsgRayEventQuery` and applies each
  group before reading the combined membership, so touching operands and
  coincident faces give the regularized result and point contacts produce no
  boundary. Each combined boundary records its owners: the leaves whose
  transition alone changes membership or, when none does, every changed leaf
  oriented like the change. Difference parity, placement snapshots and leaf
  paths come from the leaf batch. The §6.1 ON extension is not modelled.
- **Complete:** `CsgCrossingMembershipFilter` is the typed POV heuristic: union
  keeps every operand event, an intersection keeps events contained by every
  sibling, and a difference keeps minuend events outside every subtrahend and
  subtrahend events inside the minuend and outside the other subtrahends.
  Membership is the certified classification of a tolerance ball measured in
  the coordinates of the node being filtered; `Boundary` counts as contained.
  An event is dropped when one sibling proves exclusion and kept only when
  every sibling proves membership; otherwise the query fails explicitly.
- **Complete:** `CsgMembershipRule` holds Table 3 and the heuristic's
  containment requirement. Both VITRAL evaluators and the POV
  `ConstructiveSolidGeometryByRaySegment` (merge and point containment) use it.
  The POV Roth merge keeps its historical left-first sequential tie policy,
  which differs from the grouped policy at exactly equal parameters.
- **Complete:** `PovrayBody::classifyRay` supports regions: a verified
  `bounded_by` leaves the core classification unchanged, and `clipped_by` is the
  Roth classification of the shared intersection of the core
  (`RayCastingCoreOperand`) and each clip. Unverified bounds stay `Unsupported`.
- **Complete:** result versions and workspaces. Expressions report a
  structure version (`CsgTree` counts operand changes) and operand views a
  modification version (`CsgBodyOperand` and `RayCastingCoreOperand` use the
  body's, `CsgPlacedExpression` counts placement edits). Views that do not
  track their edits make a stamp incomplete, so their results are never
  reported current. Edits of geometry parameters behind a body are not
  observed. Leaf batches pool their leaves, and assignments of instance paths,
  witnesses and classifications reuse storage.
- **Complete:** POV CSG expressions are shared expressions for the typed
  evaluators. Each POV CSG operand is a `PovrayBody` seen as a `CsgBodyOperand`;
  it classifies its leaf by ray and by certified point through its wrapper's
  shared solid (`PovrayBody::queryWrappedSolid`) and
  `SceneSolid::classifyPlacedPointCertified`, reports a modification version,
  and opts into structural traversal together with the Morgan and Roth
  `ConstructiveSolidGeometry` nodes, whose `CsgTree` reports a structure version.
- **Render policy:** the POV render paths keep their integer containment and
  crossing queues, which are byte-identical to the baseline. Moving them onto
  the typed evaluators changes per-ray cost and belongs to the consumer work
  in I. With a `CsgPointWorkspace` the typed heuristic allocates nothing per
  query (I-3); its cost is the certified point classification of the event
  points.
- **Complete:** `CsgBody` is a VITRAL scene body whose solid is a shared
  expression over leaf bodies it owns, evaluated by `CsgRothRayClassifier` or
  `CsgCrossingMembershipFilter` (`CsgBodyEvaluator`). The visible hit is the
  first boundary or selected event beyond `VSDK::EPSILON`; surface detail comes
  from the owning leaf, with the normal turned outward of the combined solid and
  the body's placement applied. Rays outside the conservative solid bounds are
  culled; failed evaluations report no hit. `ReaderMitScene` builds it from MIT
  CSG blocks. With opaque first-hit shading both evaluators render
  `csg_lens.ray` and `csg_union_pov.ray` identically; `csg_sculpture.ray` and
  `csg_sculpture_pov.ray` differ where the heuristic keeps the internal
  surfaces of a union nested under a difference. `CsgBody` evaluates once for
  the hit and again for its detail, without a workspace: at 640×480 the Roth
  scenes take about twenty times as long as `box_offsets.ray`, and the
  heuristic sculpture about fourteen times the Roth one, so
  `csg_sculpture_pov.ray` is rendered at 320×240.

#### Stage G exit checklist

- [x] **G-1 Shared Roth classifier** with exact-tie grouping, owners,
  difference orientation, interior starts, unbounded operands and explicit
  failures; checked against certified point classification on random trees.
- [x] **G-2 Shared POV heuristic** with explicit membership policy and
  uncertainty handling; tests show its legitimate differences from Roth
  (internal union surfaces, both coincident faces kept).
- [x] **G-3 Shared Boolean rules** used by both VITRAL evaluators and the POV
  Roth evaluator, with deterministic `-csgRoth` renders byte-identical.
- [x] **G-4 POV region ray classification** through the Roth classifier.
- [x] **G-5 Proof versions and workspaces.** `CsgVersionStamp` records the
  structure and modification versions of every node and operand a result
  borrows; Roth classifications, heuristic events, point proofs and
  attributions carry one, and only a complete, unchanged stamp is current
  (F-R2). `CsgRothRayClassifier` accepts a `CsgRayWorkspace` and fills a reused
  result in place, allocating nothing per query after warm-up (F-R3).
- [x] **G-6 Offline CSG examples.** `RaytracingOfflineExample` reads and renders
  union, intersection and difference of spheres, boxes and quadrics through
  both shared evaluators, sequentially and in parallel with identical images;
  tests check hit distances, outward normals, leaf materials, placement and the
  evaluators' difference on nested unions.
- [x] **G-7 Evaluator corpus.** The POV render evaluators and the shared
  evaluators run on the same POV expressions: the Roth render matches
  `CsgRothRayClassifier` and the Morgan render matches
  `CsgCrossingMembershipFilter` for overlapping unions and parser-built
  differences; touching boxes show the Roth render's sequential tie policy
  against the grouped policy, and coincident spheres show the
  boundary-inclusive heuristic against Roth.

#### Stage H status

- **Complete (H-1 polynomials):** `ImplicitSolid` is a `Solid` F < 0 of a
  scalar field whose ray roots a subclass supplies. It provides first hit,
  crossings, ray classification, certified point and box containment, and
  detail with the normal grad F / |grad F|. Solver roots are polished by
  Newton steps on F, and membership between roots is the sign of F inside each
  segment, so tangent double roots do not split segments and a zero sample is
  `Indeterminate`. `PolynomialSurface` holds orders 2 to 7 in the PovRay term
  order, solved by Sturm sequences or, for quartics, in closed form; POV
  `PolynomialShape` (`poly`, `cubic`, `quartic`) adapts it and keeps the
  PovRay shadow and containment tolerances. The quartic ray polynomial uses
  the correct sign of its `xx*y3` term, and the degenerate linear case no
  longer reads outside its coefficients.
- **Complete (H-2 blobs):** `BlobSolid` is F = threshold − density over
  `BlobComponent`s, with the PovRay interval intersection, certified enclosures
  of the exact falloff and bounds from the positive components (unbounded for
  a negative threshold). POV `Blob` adapts it.
- **Complete (H-3 height fields):** `HeightFieldSolid` stores single-precision
  heights on an integer grid, interpolated over two triangles per cell, and is
  the part of its box below the surface. Typed ray queries cut the ray at cell
  edges and diagonals, where the height difference is linear, so their
  crossings are exact up to rounding; certified containment uses corner height
  ranges. `traceFirstHit` is the PovRay block traversal: it needs the box one
  unit inside the grid and can report hits on the cells just outside it.
  POV `HeightField` adapts it and applies the PovRay image rules (border and
  water-level points, POT and TGA fractions); the POV body places the grid.
  `ReaderAltimetry` reads text grids for VITRAL scenes.
- **Complete (H-4 bicubic patches):** `SubdividedBezierPatch` is the PovRay
  `bicubic_patch`: the open surface of the flat triangles that a
  `BezierPatchSubdivision` (uniform or precomputed grid, adaptive, adaptive
  tree, smooth grid) makes of a 4×4 Bezier net. It reports first hits, detail
  and `Unclassified` crossings; ray classification and containment are
  `Unsupported`. Typed normals face the u×v side of their triangle, and the
  legacy first hit ignores hits nearer than 0.001, as PovRay does, because the
  overlapping pieces of an adaptive subdivision would shadow themselves.
  `ParametricBiCubicPatch` remains the exact Bezier surface. POV
  `RayCastingParametricBiCubicPatch` adapts the patch and keeps the PovRay hit
  budget shared per patch and thread since the last primary ray.
- **Complete:** meshes are `TriangleMesh` (stage D).
- **Examples:** `RaytracingOfflineExample` renders `poly_surfaces.ray`,
  `blob_molecule.ray`, `heightfield_terrain.ray` (with `terrain.alt`) and
  `bezier_patches.ray` from `etc/geometry/mitscenes`. Raster representations
  of these families for the OpenGL examples are not provided; no OpenGL example
  draws the stage D quadrics either, so they belong to the consumer work in I.

#### Stage I status

- **Complete (I-1 offline MIT corpus):** `ReaderMitScene` reads every command
  of the MIT corpus (see §7.3) and rejects unknown commands and malformed
  lines instead of skipping them. `cube`, `cylinder` (`Cone`), `torus`,
  `polybox`, `appel` (`PolyhedralBoundedSolid` from the APPE1967 fixtures),
  `obj`, `quads` (`QuadMesh`), `rotation`, `texture`, `bumpmap` and
  `backgroundcubemap` build the same bodies as the Java reader. The offline
  example selects Phong shading when a scene has normal maps, because the
  default Gouraud selection shades without normal perturbation.
  `./runAll.sh --parallel` renders all 29 scenes; the 22 scenes without these
  commands are byte-identical to their previous images and sequential and
  parallel images are identical.
- **Complete (I-2 shared scene resources):** images and normal maps used by
  several bodies or by a background are owned by `SimpleScene`
  (`addImage`, `addNormalMap`) and referenced by bodies
  (`SimpleBody::setTextureReference`, `setNormalMapReference`), as geometries
  and materials already are. The MIT reader loads each texture, bump map and
  cubemap face once per scene. `ShadersExample` frames share the model's sphere,
  texture and normal map instead of cloning them for every frame.
- **Complete (I-3 point classification workspace):**
  `CsgPointClassification::classifyContainment` and
  `classifyOperandContainment` return the result of `classify` without
  collecting witnesses, proofs or attributions, using a caller-owned
  `CsgPointWorkspace`. `CsgCrossingMembershipFilter::select` takes the same
  workspace, keeps its selected events on the workspace stack and runs its leaf
  queries in the workspace's `CsgRayWorkspace`; after warm-up a query allocates
  nothing (about 3,600 allocations per query before, on a three-leaf tree).
  The heuristic remains about forty times slower than the Roth classifier on
  that tree: its time goes to the certified interval bounds of affine spheres
  (`QuadraticInteriorBound`, `FloatingInterval`), not to storage. `CsgBody` uses
  the overload without a caller workspace, which creates one per query.
- **Complete (I-4 POV catalog adoption):** POV `PolynomialShape`, `Blob` and
  `HeightField` expose their `PolynomialSurface`, `BlobSolid` and
  `HeightFieldSolid` through `getIntrinsicGeometry`, and
  `PovrayBody::queryWrappedSolid` reports them with their inversion, so POV
  bodies and CSG operands of these families answer typed point and ray
  queries. PovRay ignores `inverse` on height fields, so a height field is
  reported as given.
- **Complete (I-5 drawable representations and ShadersExample geometry
  selection):** `SolidBoundaryTessellator` (naive surface nets) meshes the
  boundary of any body that classifies rays or points, inside a world region
  and at a declared resolution: quadrics, implicit solids, height fields and
  `CsgBody`. Edge crossings and vertices come from the body's own ray hits and
  normals, and `SolidBoundaryTessellationReport` counts the crossings and
  vertices that lie on the boundary and those that do not.
  `BezierPatchTessellator` samples the Bezier surface of a patch's control net
  with its `S_u x S_v` normals. `OpenGL4GeometryRenderer` draws quadrics,
  implicit solids and height fields with finite bounds, and
  `SubdividedBezierPatch`, from these meshes. `ShadersExample` selects the
  sphere, a quartic, a blob, a height field, a CSG body or a patch (see §7.2).
- **Complete (I-6 POV scene construction):** the parser builds every POV
  object as a `PovrayBody` (I-8) and every POV CSG as a `CsgTree` with
  `PovrayBody` leaves (I-9). POV geometry adapters hold one VITRAL geometry and
  only POV render state (inversion flags and PovRay caches); none keeps a
  placement. The height field parser builds the `HeightFieldSolid` grid and puts
  the grid normalization in the body as its first scale step, with
  byte-identical `hfclip`, `mtmand` and `sunsethf`. Removing the adapters
  themselves, so that the parser references VITRAL geometries directly, is
  stage K.
- **Complete (I-7 POV bake and render paths):** the bake is PovRay's, compiles
  its records from the `PovrayBody` list and the CSG trees, and calls shared
  kernels. `QuadricRayExpansion` (inline, in VITRAL `geometry/volume`) expands a
  `QuadricSurface` along a ray from the per-ray products a renderer caches, with
  the constant term split into a part without mixed products that a renderer may
  cache per viewpoint; roots come from `QuadricSurface::solve`. POV
  `Quadric::intersectQuadric` and `BakedQuadricIntersector` both use it and clip
  with `Quadric::clipToRenderDepths`. Plane intersection uses
  `InfinitePlane::intersectionDepth` and sphere intersection `SphereTrace` in
  both paths. The fast CSG steps are VITRAL's (§5.3): the Roth evaluator
  (`ConstructiveSolidGeometryByRaySegment`) and the bake's
  `RaySegmentCsgCombiner` build and merge `CsgSegmentClassification` over POV hit
  records, after the shared classification sample (`classificationPoint`) and
  crossing drain (`segmentsFromCrossings`); the heuristic evaluator
  (`ConstructiveSolidGeometryByMorganRules`) and the bake's
  `CsgContainmentTest` select intersection crossings with
  `CsgCrossingSelection`; point folds use `CsgMembershipRule::combine` and
  `isDecided`. The bake's compiled plans (single core and plane intersections,
  union cull bins) remain its own specializations. The gates stay byte-identical
  at unchanged render time.
- **Complete (I-8 `SceneSolid` and `PovrayBody`):** `SimpleBody` and
  `PovrayBody` implement `SceneSolid` (§7.1); `CsgBodyOperand`,
  `CsgSolidEquivalence`, `RayCrossing`, `InstanceRayCrossingBuffer` and
  `SolidBoundaryTessellator` take a `SceneSolid`, so a CSG tree mixes VITRAL and
  POV leaves. Every POV object is a `PovrayBody`, and the bake reads the POV
  `Scene`'s `PovrayBody` list. `PovrayBody` keeps only its own placement storage
  and overrides typed queries only for its region policies; the boundary
  distances, box classification and solid equivalence come from the contract
  through `queryDenotedSolid`. The gates stay byte-identical at unchanged
  render time.
- **Complete (I-9 POV CSG over `CsgTree`):** `ConstructiveSolidGeometry` owns
  a VITRAL `CsgTree` and its `CsgOperand` leaves, seen as `CsgBodyOperand`;
  its operation, operand order and structure version are the tree's
  (`CsgTree::setOperation` applies the De Morgan complement of an inversion).
  `CsgOperand` is a `PovrayBody` with one placement layer and the operand
  material: its placement edits, copies, typed queries and modification
  version are the body's, and it keeps only its render role (operand bounds
  culling, detail-owner registration and normals unwound without
  normalization). The POV evaluators, the parser, the light linking and the
  bake read the same operands. Instance grouping (`RayCastingComposite`) stays
  distinct from CSG (§6). The gates stay byte-identical at unchanged render
  time.

Stage I is complete. The POV geometry adapters remain as POV render wrappers
until stage K removes them.

#### Stage J status

- **Complete (J-1 retirement of `./raycasting`):** the `vitral_raycasting`
  target and the `./raycasting` directory no longer exist. After stages H and I
  their files held no geometric data or kernel of their own: they are the POV
  render code of VITRAL geometries (the `RayCastingGeometry` render interface,
  crossing queues and per-ray state, render bounding volumes, the fast CSG
  evaluators over `CsgTree`, `PovrayBody`, `CsgOperand` and the instance
  composites). They are compiled in `povray_core` from PovRay
  `src/environment/geometry` and `src/environment/scene`, and their tests in
  `povray_geometry_tests`. The `BooleanSetOperations` alias was replaced by
  `CsgOperation`. The audit checks that the module does not return, that VITRAL
  includes no PovRay code, that no PovRay class repeats a VITRAL class name and
  that the base mirror is identical; the inventory lists only the components that
  stage K removes.
- **Complete (J-2 dead code):** POV code without callers found while auditing
  the relocated module was removed: `ConstructiveSolidGeometry::getStructure`
  and the unused per-viewpoint constant accessors of `Quadric`. The
  bounding-volume structure (`PreRayHitElement`, `BoundingVolumeHierarchy`,
  `AabbBoundingVolume`, `AxisAlignedBoundingBox`) is kept whole, including
  members not yet called: it is the bounding-volume package designed for
  VITRAL (`doc/projectClosingSteps.md` §3), not dead code.

Stage J is complete. Dead code found in any later stage is removed as soon as
it is identified. The removal of the POV geometry adapters, which J listed, is
a structural change of the POV render and is stage K.

#### Stage K plan

Objective: PovRay keeps no wrapper over a VITRAL geometry. The parser builds
VITRAL geometries (`Sphere`, `Box`, `PlanarHalfSpace`, `QuadricSurface`,
`PolynomialSurface`, `BlobSolid`, `HeightFieldSolid`, `SubdividedBezierPatch`,
`TriangleMesh`) and gives them to `PovrayBody` through `SceneSolid`; the
geometry model is VITRAL's and common to both projects. What the adapters hold
today moves to its owner: inversion to `PovrayBody`; per-viewpoint caches
(`Quadric`'s constant term without mixed products, the plane's viewpoint dot
product) and PovRay's historical tolerances (`GeometryConfig`, the blob shadow
root distance, the box depth clipping, the height field's truncated
containment) to the bake's records and per-ray caches; crossing emission and
normal evaluation to bake kernels that call the VITRAL geometry.

Starting point (after J): `RayCastingGeometry` is the POV render interface
(`doIntersectionForAllRayCrossings`, `doIntersectionForAllRayCrossingsAnnotated`,
`doIntersectionFirstHit`, `doContainmentTest`, `doExtraInformation`,
`createBoundingVolume`, `invertGeometry`, `computeSurfaceNormal`). Its
subclasses are the adapters `Quadric`, `RayCastingBox`,
`RayCastingInfinitePlane`, `PolynomialShape`, `Blob`, `HeightField`,
`RayCastingParametricBiCubicPatch` and `NativeGeometryBinding` (spheres and
meshes), the CSG geometry `ConstructiveSolidGeometry` with its Morgan and Roth
subclasses, and the parser's `LightGeometryAdapter`. The bake already compiles
every object into a record and chooses specialized kernels (sphere, box,
plane, quadric, their CSG operand forms) by `dynamic_cast` on the adapters
(`BakedSceneBuilder`); every other record (`BoundedGeneric` objects,
`DirectPrimitive` and `TransformedPrimitive` operands) calls the adapter's
virtual crossing, containment and detail methods.
`IntersectionCandidate` attributes a hit to a `RayCastingGeometry` and resolves
detail through the `PostRayHitElement` chain.

Risks, which decide the order:

- **Byte-identical gates.** The original POV render and the Roth render must
  stay identical. Any change to the order of floating-point operations
  (composing operand transforms of nested CSG into leaves, moving a constant
  term, renormalizing a normal) changes images. Each family keeps its
  arithmetic exactly; nested CSG keeps its placement chain instead of folding
  matrices.
- **Performance.** The adapters' virtual calls are the hot path. Kernels taken
  from VITRAL stay inline in headers (an out-of-line VITRAL kernel cost 50% in
  I-7); dispatch by family happens once, at bake time, not per ray.
- **Generic records.** Records without a specialized kernel reach the adapter
  through its virtual methods; each family needs a bake kernel, chosen when
  the record is built, before its adapter can go; otherwise the render would
  need a per-ray type switch.
- **Attribution.** Hit records, shaders, texture evaluation and the detail
  chain identify the hit by its `RayCastingGeometry`; they must identify it by
  VITRAL geometry and owning body first.

Subphases, each with the full gates (original render, Roth render, examples,
audit, tests) and dead code removed as it appears:

- **K-1 Inventory and measurement.** Map every caller of every
  `RayCastingGeometry` virtual (parser, scene, bake, `RenderEngine`, shaders,
  light linking, tests) and count, per gate scene, the records of each family
  and whether a specialized or a generic kernel serves them. Output: the
  per-family migration table with its byte-identity risks. Only dead code is
  removed. *Done:* the table and the caller map are in the execution record
  ("Stage K: inventory and measurement").
- **K-2 Hit attribution without adapters.** `IntersectionCandidate` and the
  detail chain carry the VITRAL geometry and the owning `PovrayBody`/operand;
  detail is resolved by a bake kernel per family. Adapters still exist but are
  no longer how a hit is identified. *Done:* hits carry the VITRAL surface and
  its `SurfaceDetailFamily`; `SurfaceDetailKernel` resolves the normal.
- **K-3 Bake kernels for every family.** Each record, including bounding and
  clipping regions and light geometry, is served by a bake kernel chosen at
  build time for its VITRAL geometry family, carrying the family's POV caches
  and tolerances; the generic records stop calling adapter virtuals.
  *Done:* `SurfaceTraceKernel` with one kernel per family and a
  `SurfaceTraceState`; bake records trace without adapter calls except a POV
  CSG traced as a generic object (K-6). `PovrayBody`, the POV CSG evaluators
  and composites still call the adapters until K-5 and K-6.
- **K-4 Inversion in `PovrayBody`.** The body's inversion flag replaces the
  adapters' (`RayCastingBox`, `PolynomialShape`, `Blob`, `NativeGeometryBinding`)
  and `invertGeometry`; quadric and plane inversion stays in their
  coefficients, as POV encodes it.
- **K-5 Families, one at a time.** In order of increasing risk: plane, box,
  sphere and meshes (`NativeGeometryBinding`, `ProjectedTriangleTraversal`),
  quadric, polynomial, blob, height field, bicubic patch. For each: the parser
  builds the VITRAL geometry, the bake record holds the family's caches and
  tolerances, the adapter is deleted.
- **K-6 POV CSG as a scene body.** A POV CSG becomes a `PovrayBody` that owns
  its `CsgTree` and `CsgOperand` leaves, as VITRAL `CsgBody` does;
  `ConstructiveSolidGeometry` and its subclasses stop being geometries and the
  Morgan and Roth evaluators become render strategies over the tree. Nested
  CSG keeps each level's placement (`CsgPlacedExpression` or a placed tree
  node) so that the composition order is unchanged.
- **K-7 Removal of `RayCastingGeometry`.** With no subclass left, the
  interface, `LightGeometryAdapter` (light geometry becomes a scene element)
  and the bridge helpers are deleted. The audit adds the check that PovRay
  defines no `Geometry` subclass.
- **K-8 Bounding-volume package in VITRAL.** `PreRayHitElement`,
  `BoundingVolumeHierarchy`, `AabbBoundingVolume` and `AxisAlignedBoundingBox`
  move to VITRAL `environment/geometry/boundingVolumeHierarchy`, with the
  dependency layout of `doc/projectClosingSteps.md` §3: the package depends
  only on geometry elements, and its ray filters take a VITRAL ray instead of
  PovRay's `RayWithTracingState`. VITRAL geometries create their bounding
  volume; the bake keeps its derived culling boxes. The structure is kept whole
  until then.
- **K-9 Completion.** The closing criteria of §10, and the documentation of
  the public APIs and their correspondence to the literature.

Stage E is complete. The optional numerical estimator remains explicitly
approximate and is never used as a guarantee.

The D work deferred to F is resolved as listed in the F status above. Applying
the placement, bounds and query contracts to future geometry families continues
in H as each family is adopted.

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

The stage D CSG structure does not evaluate expressions itself. The stage G
evaluators do, and `CsgBody` connects them to the offline reader and renderers,
with `.ray` scenes of union, intersection and difference whose surfaces,
orientation and sequential and parallel renders are verified (G-6).

The `RaytracingOfflineExample` special cases (textures, bump mapping,
`PolyhedralBoundedSolid` and the other classic MIT commands) are resolved by I-1.
`./runAll.sh --parallel` from `testsuite/OfflineExamples/RaytracingOfflineExample`
renders the whole MIT corpus, and the reader tests check the scene content of
those cases.

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
   requiring duplicated primitives or permanent graph conversions. PovRay defines
   no `Geometry` subclass: its scene references VITRAL geometries and its render
   state lives in the bake.
5. `./raycasting` and its target disappear. Its entire inventory has a final
   destination, and each implementation is maintained only once.
6. No references from VITRAL to PovRay remain, nor duplicated geometric kernels or
   CSG steps in the POV renderer, its bake or examples. VITRAL provides no baking
   service; the bake is PovRay's. Derived representations are caches or explicit
   approximations, never another source of truth.
7. ShadersExample, RaytracingOfflineExample, all examples, and the POV gates meet
   the criteria above; public APIs and their correspondence to algorithms in the
   literature are documented.

## 11. VITRAL refraction

`SimpleRaytracer` and `ParallelRaytracer` shade local illumination, shadows and
mirror reflection only. `ReaderMitScene` reads the transmission coefficient and
refraction index of each `surface`, and `sboard.ray` and `test.ray` declare
transmissive materials, but the renderers ignore them. Refraction is renderer
work, outside the geometric convergence, and is done after the plan stages:

- Transmitted rays use Snell's law with total internal reflection, and a ray
  that leaves a solid uses the inverse index ratio, so the renderer tracks
  whether the ray is inside. Nested transmissive solids need an index stack.
- Rays starting on a surface use the existing `VSDK::EPSILON` offset policy.
  A `CsgBody` ray that starts inside the solid gets its next boundary from the
  evaluator, as the Roth and hole tests already check.
- The existing recursion limit bounds reflection and transmission together;
  the reflected and transmitted contributions follow the material
  coefficients.
- Images of `sboard.ray`, `test.ray` and any other transmissive scene change,
  so their capture references are rebaselined as a deliberate change, apart
  from any migration change.
- With a transmissive material, `csg_union_pov.ray` and its Roth counterpart
  must show the internal union surfaces only with the heuristic. That is the
  visual check of the evaluator difference that opaque first-hit shading
  cannot show.
