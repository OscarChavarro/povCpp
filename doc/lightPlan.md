# Light model reconciliation: povCpp <-> VITRAL

Recommendations for converging `povCpp/src/environment/light` and VITRAL
`cpp/base/src/main/vsdk/toolkit/environment/light`, following the direction
declared in `doc/vitralPovrayParallelLearningExperience.md` and the
model-by-model comparison method of `doc/vitralNormalizationAnalysis.md`.

Verified against both trees on 2026-07-06.

---

## 1. Current state

### 1.1. povCpp: polymorphic hierarchy, raytracing-oriented

`src/environment/light/` holds three classes:

```
Light (abstract)
├── PointLight
└── SpotLight
```

`Light` state: `shapeColor` (owned `ColorRgba *const`, nullable), `center`,
`pointsAt`, `inverted`, `coefficient`, `radius`, `falloff`.

`Light` behavior:

- `virtual double evaluateLightResponseFactor(const Ray *lightSourceRay) const = 0`
  — the attenuation contract. `PointLight` returns `1.0`; `SpotLight`
  computes `pow(cosTheta, coefficient)` shaped by a `cubicSpline(falloff,
  radius, cosTheta)` edge. The incoming ray points **from the surface toward
  the light** and `SpotLight` negates the dot product accordingly.
- `virtual Light *copy() = 0`.
- `applyLinearTransformation` / `translate` / `rotate` / `scale` / `invert`.

Consumers:

- `Scene` stores `java::ArrayList<Light*> lightSources` (non-owning; see
  adapter below).
- `LightSamplerShader::sample` builds the surface→light shadow ray, reads
  `getShapeColor()` (white `(1,1,1,0)` when null) and multiplies by
  `evaluateLightResponseFactor`.
- `DirectLightShader::shade` iterates `lightSources` for shadow tests.
- `LightSourceParser` builds `PointLight`/`SpotLight` from POV syntax,
  applying `translate`/`rotate`/`scale` tokens inline on local
  `center`/`pointsAt` vectors (not through the `Light` transform methods).
- `LightGeometryAdapter : Geometry` wraps a `Light` so `light_source` can sit
  in the parsed object tree (including inside CSG bodies); its intersection
  and containment methods are no-ops. It **owns** the `Light`.
- `ScenePostProcessor::linkLights` walks the object tree, applies accumulated
  composite transforms to each adapter's light via `applyTransformsToLight`
  (again inline matrix math, not the `Light` methods), and collects the
  non-owning pointers into `Scene::lightSources`.

### 1.2. VITRAL: single class, type-switch, GL heritage

`vsdk/toolkit/environment/light/` holds two classes:

- `Light` — one concrete class for every light kind, discriminated by the
  **public** field `int tipo_de_luz` against `LightType` constants
  (`AMBIENT = 0`, `DIRECTIONAL = 1`, `POINT = 2`). Position/direction is the
  **public** field `Vector3Dd lvec` (normalized at construction for
  `DIRECTIONAL`, zeroed for `AMBIENT`). Private state: `ambient`, `diffuse`,
  `specular` (`ColorRgb` values — fixed-function OpenGL heritage; the
  constructor sets `specular` from the emission argument, `diffuse` to
  white, `ambient` to black), `id`, `name`.
- `LightType` — the constants holder.

Consumers:

- `SimpleScene` owns the `ArrayList<Light*>`, assigning `id` on `addLight`.
- `LightingShader` and `CookTorranceShader` branch on
  `light->tipo_de_luz`: `AMBIENT` contributes `material.ambient *
  light.emission` and continues; `DIRECTIONAL` uses `-lvec` as the incident
  direction; `POINT` uses `lvec - hitPoint`.
- `SimpleRaytracer::hasNonAmbientLights` branches on the same field.
- `ReaderMitScene` constructs lights by type constant.
- `OpenGL4LightRenderer` (gizmo drawing) uses only `getPosition()` and
  `getSpecular()`.
- The Java port mirrors `Light.java`/`LightType.java` 1:1; any change to the
  C++ classes must be replicated there (port-parity rule).

### 1.3. Divergence matrix

| Aspect | povCpp | VITRAL |
|---|---|---|
| Kind discrimination | subclass + virtual dispatch | `int tipo_de_luz` + switch |
| Kinds covered | point, spot | ambient, directional, point |
| Attenuation contract | `evaluateLightResponseFactor(const Ray*)` | none (inline in shaders) |
| Color model | one nullable owned `ColorRgba*` | `ambient`/`diffuse`/`specular` `ColorRgb` values |
| Position field | `center` + `pointsAt` (spot aim) | `lvec` (public), meaning depends on type |
| Encapsulation | private fields, getters | two public fields |
| Identity | none | `id`, `name` |
| Cloning | `virtual copy()` | none |
| Transform API | `translate`/`rotate`/`scale` on `Light` (unused) | none |
| Scene ownership | `LightGeometryAdapter` owns; `Scene` aliases | `SimpleScene` owns |

Neither model is a subset of the other: povCpp has the hierarchy, the
attenuation contract and cloning; VITRAL has the ambient/directional kinds,
identity and clear scene ownership.

---

## 2. Target converged model

One `Light` hierarchy living in VITRAL (and mirrored into povCpp `base/`),
combining both:

```
Light (abstract)
├── AmbientLight        (VITRAL-only kind today)
├── DirectionalLight    (VITRAL-only kind today)
├── PointLight          (both)
└── SpotLight           (povCpp-only kind today)
```

- `Light` base state: `position` (`Vector3Dd`), `emission` (color value, not
  a heap pointer), `id`, `name`.
- `Light` base behavior: `virtual double evaluateLightResponseFactor(const
  Ray *lightSourceRay) const = 0` with the povCpp convention documented on
  the declaration (ray direction is surface→light, normalized);
  `virtual Light *copy() const = 0`; position/emission accessors.
- `AmbientLight::evaluateLightResponseFactor` is never sampled through a
  shadow ray — shaders handle ambient before sampling, as `LightingShader`
  already does — so it returns `0.0` and shaders keep the ambient branch,
  now driven by `dynamic_cast`/virtual query instead of `tipo_de_luz`.
- `DirectionalLight` stores the normalized direction and returns `1.0`.
- `PointLight` returns `1.0`.
- `SpotLight` adds `pointsAt`, `coefficient` (POV `tightness`), `radius`,
  `falloff` and the `cubicSpline` falloff — moved **down** from today's
  povCpp `Light` base, where they are spot-only state that `PointLight`
  carries dead.
- `LightType` disappears with the switch sites. If io code (`ReaderMitScene`)
  still needs a serializable discriminator, it maps file tokens to concrete
  constructors directly.

Color-model decision: the base keeps a **single emission color**. VITRAL's
`ambient`/`diffuse`/`specular` triplet is redundant in practice — the
constructor derives all three from one emission argument, `LightingShader`
uses `getSpecularReference()` for both diffuse and specular contributions,
and per-light ambient only matters for `AmbientLight`, which owns its
emission anyway. The triplet accessors can survive as deprecated shims
returning derived values while VITRAL examples migrate. Whether the shared
value type is `ColorRgb` or `ColorRgba` should follow the same decision made
for the material model (povCpp needs the filter channel; VITRAL shaders do
not read alpha) — proposal: `ColorRgba`, with VITRAL shaders ignoring alpha.

---

## 3. Direction A — VITRAL toward povCpp (implement the hierarchy)

Work on the VITRAL repository, C++ and Java in lockstep:

1. Add `AmbientLight`, `DirectionalLight`, `PointLight` subclasses of
   `Light`; make `Light` abstract with `evaluateLightResponseFactor` and
   `copy()`. Port `SpotLight` from povCpp (it has no VITRAL dependency
   beyond `Ray`, already shared).
2. Encapsulate the two public fields: `tipo_de_luz` → removed, `lvec` →
   private `position` behind the existing `getPosition()`/`setPosition()`.
3. Migrate the four switch sites (`LightingShader`, `CookTorranceShader`,
   `SimpleRaytracer::hasNonAmbientLights`, `ReaderMitScene`) from
   `tipo_de_luz` comparisons to the hierarchy. `OpenGL4LightRenderer` and the
   gizmo path already use only base accessors and need no change.
4. Delete `LightType` once no switch site remains.
5. Mirror every step into `java/base/src/main/vsdk/toolkit/environment/light/`
   and the Java shader/reader counterparts (port-parity rule from
   `vitralPovrayParallelLearningExperience.md`).

Risk: low — VITRAL's light consumers are few and mechanical. The main cost
is the Java mirror.

---

## 4. Direction B — povCpp toward VITRAL (simplify and decouple)

Work on this repository, gate must stay byte-identical (no attenuation math
changes):

1. **Delete dead API on `Light`.** `applyLinearTransformation`, `translate`,
   `rotate`, `scale` and `invert` have no callers — `LightSourceParser` and
   `ScenePostProcessor::applyTransformsToLight` both do their matrix math
   inline. The `inverted` flag is never set to `true` (the parser defaults it
   and only copies it between constants) and never read by shading — remove
   the field, `isInverted()`, `invert()` and the constructor parameter. The
   default `Light()` and the no-color constructor are also uncalled (only
   `PointLight`/`SpotLight` construct through the full-argument constructor).
2. **Push spot-only state down to `SpotLight`.** `pointsAt`, `coefficient`,
   `radius` and `falloff` leave the base; `PointLight` becomes
   position + color. Parser impact: `LightSourceParser` already accumulates
   these in local variables and decides `spotlight` at the end, so
   construction is unaffected; only the `LIGHT_SOURCE_CONSTANT` clone path
   (which reads them through base getters) must switch to `copy()` plus a
   `dynamic_cast<SpotLight*>` to extract spot parameters, and
   `ScenePostProcessor::applyTransformsToLight` must transform `pointsAt`
   only for spot lights.
3. **Replace the owned `ColorRgba*` with a value.** `shapeColor` is a
   heap-cloned nullable pointer whose null case `LightSamplerShader` maps to
   white `(1,1,1,0)`; store a `ColorRgba` value defaulted to that white and
   drop the null branch, the destructor and the clone-in-constructor logic.
   This also removes the ownership comments burden in `LightSourceParser`.
4. **Rename toward the shared vocabulary.** `center` → `position`
   (`getPosition()`/`setPosition()`), matching VITRAL and
   `OpenGL4LightRenderer` expectations, so the class can later move into
   `base/` without renaming its consumers twice.
5. **Fix scene ownership.** Today `LightGeometryAdapter` owns each `Light`
   and stays in the parsed object tree as a no-op `Geometry`, while
   `Scene::lightSources` aliases into it. After
   `ScenePostProcessor::linkLights` runs, transfer ownership to `Scene`
   (matching `SimpleScene::addLight`) and remove the adapters from the
   object list — they contribute nothing at trace time. The adapter itself
   remains a parse-time construct in `src/io/pov/light/`; it is
   application-specific and never migrates to VITRAL (same status as the
   rest of the `io` layer).

Steps 1–4 reshape `src/environment/light/` into exactly the target model of
§2 minus the ambient/directional kinds, which povCpp does not need but also
does not conflict with.

---

## 5. Direction C — unification through `base/`

Once A and B have landed and both trees expose the same class shapes:

1. Create `base/src/main/vsdk/toolkit/environment/light/` with the converged
   `Light`, `AmbientLight`, `DirectionalLight`, `PointLight`, `SpotLight`,
   kept in manual sync with the VITRAL repository like the rest of `base/`.
2. Point povCpp includes at the `base/` package and delete
   `src/environment/light/`.
3. povCpp keeps only its application-specific pieces: `LightSourceParser`,
   `LightGeometryAdapter`, `ScenePostProcessor` light linking,
   `LightSamplerShader` and `DirectLightShader` (these depend on
   `RayWithTracingState`, `BakedScene` and the POV shadow model, which stay
   out of VITRAL per the parallel-learning document).

Ordering: B can start immediately (local cleanup, gate-verifiable). A is
independent and can proceed in parallel on the VITRAL side. C strictly
follows both.

---

## 6. Open decisions

1. **Color value type in the shared base** — `ColorRgb` vs `ColorRgba`
   (§2). Proposal: `ColorRgba`; must be decided together with the pending
   `Material` reconciliation, since lights and materials should not disagree
   on the color vocabulary.
2. **Ambient handling in the shared contract** — keep ambient as a light
   kind (VITRAL today) or as a scene/background property. Keeping
   `AmbientLight` is the zero-migration option and is what §2 assumes.
3. **`id`/`name` on the base** — VITRAL needs them (`SimpleScene::addLight`,
   gizmo/GL tooling); povCpp ignores them. They are cheap; proposal: keep
   them on the shared base rather than fork the class shape.
4. **`evaluateLightResponseFactor` for GL paths** — the OpenGL renderers
   never call it; no conflict, but the Java port must still carry the method
   for parity even where unused.
