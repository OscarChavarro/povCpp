# Ray-casting environment extension

`vitral_raycasting` is a C++11 static library over `vitral_base`. It contains the
geometries, CSG strategies, bounds, ray workspaces and affine bodies extracted
from the renderer. It lives in povCpp during architecture alignment; VITRAL does
not depend on this library. It does not depend on PovRay materials, shaders, its parser or its
application. Static linkage lets consuming renderers retain cross-unit LTO.

The shared model is `Geometry` and the scene contract `SceneSolid`.
`PovrayBody` implements `SceneSolid` (it is not a `SimpleBody`): it owns its
geometry through the contract, keeps PovRay's two placement layers, appearance
and regions, and inherits the typed queries computed from its denoted solid. POV
bodies belong to the POV `Scene`, not to a `SimpleScene`; VITRAL renderers draw
`SimpleBody` bodies. Plain `SimpleBody` can wrap a `RayCastingGeometry` and use
its usual TRS placement.

The ordinary nearest-hit primitives remain available. Ray-casting variants
preserve historical crossing, tolerance and inversion policies; they are not
aliases that silently change the semantics of existing VITRAL scenes.

## Queries and placement

- Shared `doIntersectionFirstHit` / `doExtraInformation` queries bridge to the
  native crossing/detail path. This bridge creates a temporary workspace and
  supplies point/normal details; it does not implement UV or tangent generation.
- Native ray-casting queries reuse `RayWithTracingState` and the caller's queue
  pool. Renderers using that path retain its original allocation behavior.
- `emitRayCrossings` distinguishes unsupported capability from a supported miss.
  Events are ordered by ray parameter and preserve leaf, body and material
  attribution where available. The interval clips the primitive's emitted
  roots; it does not override the primitive's historical numerical tolerances.
- Events emitted by the compatibility bridge have `Kind::Unclassified`.
  Consumers must not treat them as entry events. Use the module's CSG operations
  or containment queries for solid membership. The native base `Sphere` emitter
  classifies its events explicitly. Chord/approach controls in `RayCrossingQuery`
  apply to that sphere kernel, not to arbitrary numerical surfaces.
- Resolve transformed/CSG detail through the queried geometry or body; leaf
  identity alone does not contain its placement chain.
- `PovrayBody` preserves two affine placement layers and their construction
  order; `getAffinePlacement()` exposes the combined placement. It has no TRS
  editing: use its `translate`, `rotate`, `scale` methods to compose placements,
  or wrap a geometry in plain `SimpleBody` when TRS editing is required.
- The detail-owner chain has an explicit limit of eight entries. Overflow throws
  instead of discarding attribution silently.

`getSurfaceMaterial()` exposes `Material*`. The legacy `getMaterial()` projection
returns `SimpleMaterial*` and rejects a different material family; applications
must use the shader pipeline that understands their material. Geometry queries
do not translate application materials into approximate simple materials.

Snapshots retain references to scene elements. Keep the scene alive and do not
mutate it until rendering completes. The snapshot owns its camera snapshot.
The specialized `RayCastingHit` and candidate queues remain query data, not a
second editable scene model.

## Build and verification

The povCpp root build includes this library; the VITRAL root build does not. It can also be added with
`add_subdirectory` after `base`; its public include root is `src/main`.
With `RAYCASTING_BUILD_TESTS=ON`, `raycasting_shared_environment` checks shared-model
CSG crossings, leaf/material/body attribution, detail-chain overflow, and
serial/parallel rendering through the existing VITRAL raytracers.
