# Baked Scene Fallbacks

`Scene::finalizeCompiledTracingScene()` is the explicit pre-render boundary for
the scene-owned baked representation. `RenderEngine::initializeRenderer()` calls
it before any render ray is traced, then resets the temporary fallback counters.

Remaining render-time fallbacks to parsed scene objects are intentionally
limited and counted in `BakedTracingCommon`:

- `traceObjectFirstHit`: uses `entry.object->doIntersectionFirstHit()` when a
  compiled object has neither a baked simple-body index nor a baked composite
  index.
- `traceObjectAllCrossings`: uses
  `entry.object->doIntersectionForAllRayCrossings()` for the same unbaked entry
  class, commonly reached from bounding, clipping, shadow, or composite paths.
- `containmentTest`: uses `entry.object->doContainmentTest()` when an unbaked
  entry still owns containment behavior.

The counters are printed in the final render statistics as:

`Baked fallbacks: first-hit N  all-crossings N  containment N`

These diagnostics are temporary Phase 1 instrumentation. A zero count proves the
finalized baked arrays handled the render path for that scene; nonzero counts
identify parsed-scene behavior that later phases still need to migrate.
