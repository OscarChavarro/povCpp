# Plan: reinterpret povCpp's `+qN` quality as VITRAL-style lazy selection

**Plan** document (actionable, unlike `doc/vitralNormalizationAnalysis.md`,
which is descriptive). It defines how to evolve povCpp's single quality
integer (`+qN`) into an orthogonal set of feature flags plus a per-ray lazy
detail mask, mirroring VITRAL's `RendererConfiguration` + `RayHit::
requiredDetailMask` model — while keeping every existing render byte-identical
and tracking the performance cost cell by cell.

Read §7 of `doc/vitralNormalizationAnalysis.md` first; this plan executes the
convergence opportunity described there.

---

## 1. Objective and current state

povCpp gates rendering features behind comparisons on a single integer
`RenderingConfiguration::quality` (default 9, set by `+qN`). The thresholds,
verbatim from the current pipeline:

| Feature gated | Condition | Where |
|---|---|---|
| Surface lit at all (normal + ambient + diffuse/specular) | `quality > 1` | `LocalSurfaceShader.cpp:36` |
| Shadow-ray object tests (cast shadows) | `quality > 3` | `DirectLightShader.cpp:63` |
| Full pigment/texture eval (else `quickColor`/`objectColor`/grey) | `quality > 5` | `RayShaderPipeline.cpp:72` |
| Transparent/coloured shadows (shadow ray not short-circuited) | `quality > 5` | `RayShaderPipeline.cpp:59` |
| Refraction / transmission | `quality > 5` | `RayShaderPipeline.cpp:124` |
| Bump-mapped normal (surface and refraction paths) | `quality >= 8` / `> 7` | `LocalSurfaceShader.cpp:46`, `RayShaderPipeline.cpp:131` |
| Mirror reflection | `quality >= 8` | `LocalSurfaceShader.cpp:67` |

VITRAL expresses the same "don't compute what the image doesn't need" idea as
two orthogonal mechanisms: `RendererConfiguration` feature booleans
(`isTextureSet`, `isBumpMapSet`, `shadingType`, …) and the per-ray
`RayHit::requiredDetailMask` (`DETAIL_POINT|NORMAL|UV|TANGENT`), consumed by
`doExtraInformation()` and chosen by `SimpleRaytracer::buildSurfaceDetailMask`.

**Goal:** make `+qN` a *named preset* over a flag set that the shaders query
directly, and add a per-ray lazy mask, so that:

1. the quality knob stops being a pile of magic-number comparisons;
2. povCpp's configuration model moves toward VITRAL's feature-flag shape;
3. arbitrary feature subsets become expressible (today impossible: e.g.
   "textures but no shadows", "reflection without bump");
4. per-ray laziness (shadow vs primary) becomes first-class rather than the
   single ad-hoc `shadowRay && quality <= 5` short-circuit.

---

## 2. Does this make sense? (assessment)

**Yes, with a clear boundary.** The reinterpretation is worth doing and does
bring the two codebases closer, but only the *configuration/laziness* layer
should converge — not the renderers themselves.

What genuinely converges:

- **The feature-flag model.** povCpp's `quality` staircase is, provably, a
  single hand-picked path through the very flag space VITRAL already exposes
  (`withShadows`, `withTexture`, `withBump`, `shadingType`, …). Decomposing it
  into those flags makes povCpp's `RenderingConfiguration` structurally
  resemble VITRAL's `RendererConfiguration` for the first time (today they
  share only a name; see §5 of the analysis doc). This is real convergence.
- **The lazy detail mask.** Adding a per-ray `requiredDetailMask` to
  `RayWithSegments`/`PovRayHit` is a direct port of a VITRAL concept povCpp
  currently lacks (it always computes the winner's normal). It pays for
  itself: shadow rays and `q<=1` previews stop computing normals/UV they
  discard.

What must **not** converge (and why this stays "additive, layered"):

- The **engines are different by necessity**: povCpp keeps `allIntersections`
  + the CSG depth queue + the containing-media refraction stack, none of which
  VITRAL's nearest-hit `SimpleRaytracer` has. We are aligning the *selection
  vocabulary*, not the traversal.
- The **preset semantics of `+qN` must be preserved bit-for-bit** for existing
  users and goldens. So the flag set is introduced *underneath* `+qN`: each
  quality level maps to a fixed flag combination that reproduces today's
  output exactly (guarded by `scripts/testQualities.sh`). The flags are the
  new primitive; `+qN` becomes sugar.

Net: this is one of the higher-value convergence steps available, because it
simultaneously (a) removes magic numbers, (b) unlocks the efficiency lever
povCpp already half-uses (fast preview) and makes it measurable, and (c) ports
a concrete VITRAL mechanism (the detail mask) rather than merely renaming.

The risk it does **not** pay off: if every realistic preset ends up being one
of the existing five bands anyway, the orthogonality buys expressiveness few
scenes use. The performance baseline (§4) is what tells us whether finer
control actually saves time on real scenes, so we collect it *before*
committing to the refactor.

---

## 3. Target model

### 3.1. Feature flags on `RenderingConfiguration`

Introduce predicates the shaders query instead of `getQuality() <> N`:

- `withSurfaceLighting()`  — normal + ambient + diffuse/specular (today `q>1`)
- `withShadows()`          — cast-shadow ray tests (today `q>3`)
- `withTextures()`         — full pigment vs `quickColor` (today `q>5`)
- `withFilteredShadows()`  — coloured/transparent shadows (today `q>5`)
- `withRefraction()`       — transmission/refraction (today `q>5`)
- `withBumpMapping()`      — bump normal perturbation (today `q>=8`/`>7`)
- `withReflection()`       — mirror reflection (today `q>=8`)
- `shadingType`            — `getShadingType()`, a *derived* read-only view
  aligned with VITRAL's `NOLIGHT/FLAT/GOURAUD/PHONG/COOK_TERRANCE` enum
  values; povCpp only ever reports `NOLIGHT` (q0-1) or `PHONG` (q2+) since it
  has no FLAT/GOURAUD/COOK_TERRANCE shader (see §8, Step 7).

`setQuality(N)` becomes a *preset setter* that fills these flags to reproduce
band `N` exactly (see the mapping below). Direct flag setters allow subsets
`+qN` cannot express.

| Band | withSurfaceLighting | withShadows | withTextures | withFilteredShadows | withRefraction | withBumpMapping | withReflection |
|------|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| q0–q1 | – | – | – | – | – | – | – |
| q2–q3 | ✓ | – | – | – | – | – | – |
| q4–q5 | ✓ | ✓ | – | – | – | – | – |
| q6–q7 | ✓ | ✓ | ✓ | ✓ | ✓ | – | – |
| q8–q9 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |

### 3.2. Per-ray lazy detail mask

Add a `requiredDetailMask` (mirroring VITRAL's `DETAIL_POINT|NORMAL|UV|
TANGENT`) to the hit path, set per ray from the flags above:

- shadow rays → `DETAIL_NONE` (only `hitDistance`);
- `q<=1` preview primary rays → no normal (the surface is flat-shaded anyway);
- textured primary rays → normal (+ UV once povCpp computes UV on the hit).

`TransformableElement::doExtraInformation` already exists as the choke point
(§5 of the analysis doc); it gains a mask check before calling `normal()`,
exactly as VITRAL's `doExtraInformation` honours `needsNormal()`.

---

## 4. Gates

### 4.1. Correctness gate (new): `scripts/testQualities.sh`

`scripts/renderQualities.sh` renders two tiers into `output/qualities/`:
`etc/level2/iortest.pov` at every quality 0..9 (320×200), and — since §8's
execution — all 108 gate scenes at one representative quality per band
(`q0,q2,q4,q6,q8`), mirroring `renderAll.sh`'s per-level output layout
(`output/qualities/<level>/<scene>_q<N>.tga`). `scripts/testQualities.sh`
diffs everything under `output/qualities/` recursively against the committed
baseline in `../referenceTestImagesQualities/`, by relative path, so it needs
no change when the render script's coverage grows. `iortest.pov` remains the
single tight, full-ten-level witness that the preset mapping in §3.1 stays
bit-identical band-boundary by band-boundary; the 108-scene tier is the
broader, shallower check that the same mapping holds across the whole scene
corpus, not just on that one spanning fixture (this addresses §7's "iortest
proves too narrow a witness" risk).

Run sequence for the new gate:

```
./scripts/clean.sh; ./scripts/compile.sh
./scripts/renderQualities.sh; ./scripts/testQualities.sh        # => "Test passed."
```

The existing full gate must also stay green:

```
./scripts/renderAll.sh; ./scripts/testAgainstGoldenImages.sh    # => "Test passed."
```

> Note: the full golden gate renders at `+q9` (default), so it only guards the
> top band. `testQualities.sh` is what guards the *other* bands; both are
> required for this work.

### 4.2. Performance baseline (Step 0): the 108×5 matrix

`scripts/renderQualityMatrix.sh` renders all 108 gate scenes at q0..q9 (10
raw renders per scene) and records wall-clock seconds per cell into
`output/qualityMatrix.tsv` (320×200 by default; `WIDTH=1280 HEIGHT=800` for
full-res). Per §1/§3.1, `+qN` only takes five distinct *images*: each pair
{q0,q1}, {q2,q3}, {q4,q5}, {q6,q7}, {q8,q9} maps to the same flag combination
and renders byte-identical output (confirmed for `iortest` in the analysis
doc's §7.2 AE-diff). So the meaningful baseline shape is **108 scenes × 5
bands**, not 108×10: the table below averages each same-band pair's timing
into one column per band (the raw 108×10 timings remain in
`output/qualityMatrix.tsv` for anyone who wants the unaveraged numbers, but
`output/` is gitignored). `output/` is gitignored, so the **table pasted
below is the canonical, committed baseline**. Every later step re-runs the
matrix and is judged by its delta against these numbers, band by band.

What the baseline must show to justify the refactor: a clear *monotone
decrease* in time as the band drops (the fast-preview lever), and where the
big steps are (expected at the band boundaries q1→q2, q3→q4, q5→q6, q7→q8).
The reinterpreted model should reproduce that curve within noise at the
preset levels, and additionally let a *custom* flag subset land *between* two
bands in both image and time — that interpolation is the new capability we
are buying.

<!-- QUALITY_MATRIX_BASELINE -->
Baseline collected at 320×200 (108 scenes × 10 qualities = 1080 renders,
single-threaded, ~253 s total), then collapsed to 108×5 bands by averaging
each same-image pair ({q0,q1}, {q2,q3}, {q4,q5}, {q6,q7}, {q8,q9} — see §1/
§3.1 for why those pairs are image-identical). Seconds per render; lower
band is the fast preview. The two summary rows give the per-band geometric
mean over the 108 scenes and the column totals — note the monotone steps at
the band boundaries q1→q2 (lighting), q3→q4 (shadows, ~×1.7), q5→q6
(textures + refraction), and q7→q8 (bump + reflection); the full model
(q8-9) costs ~3.4× the q0-1 preview.

| # | scene | lvl | q0-1 | q2-3 | q4-5 | q6-7 | q8-9 |
|--:|-------|-----|---:|---:|---:|---:|---:|
| 1 | bumpmap | level1 | 0.035 | 0.037 | 0.054 | 0.056 | 0.103 |
| 2 | alphafun | level1 | 0.035 | 0.049 | 0.058 | 0.159 | 0.177 |
| 3 | ballbox1 | level1 | 0.046 | 0.075 | 0.096 | 0.308 | 0.508 |
| 4 | basicvue | level1 | 0.035 | 0.048 | 0.055 | 0.060 | 0.070 |
| 5 | blob | level1 | 0.050 | 0.054 | 0.067 | 0.065 | 0.072 |
| 6 | box | level1 | 0.033 | 0.038 | 0.039 | 0.037 | 0.048 |
| 7 | cantelop | level1 | 0.035 | 0.043 | 0.038 | 0.040 | 0.041 |
| 8 | checker2 | level1 | 0.036 | 0.046 | 0.052 | 0.123 | 0.190 |
| 9 | cliptst2 | level1 | 0.038 | 0.042 | 0.043 | 0.065 | 0.088 |
| 10 | colors | level1 | 0.255 | 0.260 | 0.351 | 0.358 | 0.361 |
| 11 | dish | level1 | 0.039 | 0.060 | 0.075 | 0.074 | 0.075 |
| 12 | dodec2 | level1 | 0.038 | 0.053 | 0.054 | 0.062 | 0.060 |
| 13 | fogtst | level1 | 0.044 | 0.039 | 0.046 | 0.062 | 0.075 |
| 14 | glasdish | level1 | 0.045 | 0.064 | 0.074 | 0.196 | 0.772 |
| 15 | glass | level1 | 0.033 | 0.048 | 0.053 | 0.389 | 0.861 |
| 16 | imagetst | level1 | 0.052 | 0.060 | 0.066 | 0.113 | 0.131 |
| 17 | intee1 | level1 | 0.053 | 0.060 | 0.076 | 0.364 | 0.477 |
| 18 | laser | level1 | 0.052 | 0.086 | 0.156 | 0.370 | 0.717 |
| 19 | mapper | level1 | 0.042 | 0.030 | 0.043 | 0.046 | 0.049 |
| 20 | mappr2 | level1 | 0.051 | 0.048 | 0.056 | 0.054 | 0.064 |
| 21 | matmap | level1 | 0.045 | 0.058 | 0.059 | 0.077 | 0.102 |
| 22 | pvinterp | level1 | 0.042 | 0.045 | 0.049 | 0.066 | 0.061 |
| 23 | shapes2 | level1 | 0.066 | 0.084 | 0.142 | 0.135 | 0.142 |
| 24 | shapes | level1 | 0.095 | 0.105 | 0.270 | 0.250 | 0.245 |
| 25 | spotlite | level1 | 0.043 | 0.062 | 0.084 | 0.122 | 0.130 |
| 26 | stone1 | level1 | 0.072 | 0.085 | 0.125 | 0.191 | 0.193 |
| 27 | stone2 | level1 | 0.078 | 0.078 | 0.119 | 0.402 | 0.410 |
| 28 | stone3 | level1 | 0.079 | 0.089 | 0.122 | 0.357 | 0.367 |
| 29 | stone4 | level1 | 0.044 | 0.044 | 0.048 | 0.056 | 0.063 |
| 30 | sunset1 | level1 | 0.036 | 0.053 | 0.065 | 0.052 | 0.085 |
| 31 | sunset | level1 | 0.033 | 0.045 | 0.054 | 0.213 | 0.296 |
| 32 | texture1 | level1 | 0.080 | 0.096 | 0.164 | 0.253 | 0.258 |
| 33 | texture2 | level1 | 0.069 | 0.090 | 0.158 | 0.221 | 0.398 |
| 34 | texture3 | level1 | 0.043 | 0.060 | 0.089 | 0.105 | 0.147 |
| 35 | window | level1 | 0.051 | 0.055 | 0.077 | 0.083 | 0.115 |
| 36 | arches | level2 | 0.050 | 0.074 | 0.097 | 0.415 | 0.831 |
| 37 | cluster | level2 | 0.046 | 0.063 | 0.119 | 0.112 | 0.153 |
| 38 | crystal | level2 | 0.049 | 0.074 | 0.116 | 0.171 | 0.414 |
| 39 | eight | level2 | 0.046 | 0.053 | 0.071 | 0.079 | 0.089 |
| 40 | esp01 | level2 | 0.054 | 0.068 | 0.078 | 0.167 | 0.191 |
| 41 | hfclip | level2 | 0.079 | 0.095 | 0.144 | 0.168 | 0.184 |
| 42 | illum1 | level2 | 0.061 | 0.082 | 0.112 | 0.251 | 1.091 |
| 43 | illum2 | level2 | 0.072 | 0.091 | 0.207 | 0.170 | 0.338 |
| 44 | iortest | level2 | 0.081 | 0.113 | 0.378 | 0.695 | 2.061 |
| 45 | lpops1 | level2 | 0.075 | 0.080 | 0.100 | 0.156 | 0.415 |
| 46 | lpops2 | level2 | 0.084 | 0.084 | 0.115 | 0.181 | 0.505 |
| 47 | magglass | level2 | 0.050 | 0.053 | 0.098 | 0.173 | 0.185 |
| 48 | mtmand | level2 | 0.057 | 0.073 | 0.101 | 0.104 | 0.118 |
| 49 | pacman | level2 | 0.040 | 0.054 | 0.072 | 0.091 | 0.126 |
| 50 | pawns | level2 | 0.251 | 0.268 | 0.726 | 1.548 | 1.958 |
| 51 | planet | level2 | 0.038 | 0.040 | 0.054 | 0.233 | 0.239 |
| 52 | poolball | level2 | 0.060 | 0.073 | 0.100 | 0.102 | 0.127 |
| 53 | romo | level2 | 0.050 | 0.057 | 0.071 | 0.091 | 0.191 |
| 54 | room | level2 | 0.051 | 0.075 | 0.132 | 0.238 | 0.243 |
| 55 | skyvase | level2 | 0.059 | 0.080 | 0.102 | 0.147 | 0.534 |
| 56 | smoke | level2 | 0.041 | 0.048 | 0.054 | 0.186 | 0.199 |
| 57 | spline | level2 | 0.214 | 0.217 | 0.307 | 0.318 | 0.421 |
| 58 | stonewal | level2 | 0.074 | 0.085 | 0.135 | 0.164 | 0.255 |
| 59 | sunsethf | level2 | 0.071 | 0.069 | 0.126 | 0.285 | 0.417 |
| 60 | tetra | level2 | 0.066 | 0.079 | 0.134 | 0.164 | 0.384 |
| 61 | waterbow | level2 | 0.058 | 0.068 | 0.080 | 0.147 | 0.362 |
| 62 | wtorus | level2 | 0.056 | 0.072 | 0.093 | 0.188 | 0.197 |
| 63 | car | level3 | 0.117 | 0.124 | 0.251 | 0.258 | 0.830 |
| 64 | chess | level3 | 0.123 | 0.138 | 0.195 | 0.279 | 0.404 |
| 65 | desk | level3 | 0.129 | 0.147 | 0.359 | 0.343 | 0.518 |
| 66 | dfwood | level3 | 0.052 | 0.068 | 0.083 | 0.236 | 0.243 |
| 67 | drums | level3 | 0.354 | 0.405 | 2.320 | 4.046 | 5.745 |
| 68 | fish13 | level3 | 0.239 | 0.257 | 0.482 | 0.573 | 0.911 |
| 69 | fishbowl | level3 | 0.073 | 0.088 | 0.120 | 0.206 | 0.223 |
| 70 | ionic5 | level3 | 0.446 | 0.457 | 1.064 | 1.699 | 1.707 |
| 71 | kscope | level3 | 0.061 | 0.076 | 0.114 | 0.153 | 0.502 |
| 72 | lamp | level3 | 0.085 | 0.103 | 0.213 | 0.169 | 0.268 |
| 73 | ntreal | level3 | 0.244 | 0.262 | 0.396 | 0.525 | 0.532 |
| 74 | oak2 | level3 | 0.345 | 0.347 | 0.863 | 1.275 | 1.554 |
| 75 | palace | level3 | 0.075 | 0.079 | 0.126 | 0.207 | 0.214 |
| 76 | pencil | level3 | 0.073 | 0.085 | 0.172 | 0.182 | 0.298 |
| 77 | piece1 | level3 | 0.155 | 0.167 | 0.514 | 0.480 | 0.881 |
| 78 | piece2 | level3 | 0.306 | 0.326 | 0.752 | 0.810 | 1.514 |
| 79 | piece3 | level3 | 0.436 | 0.470 | 1.127 | 1.093 | 1.546 |
| 80 | pool | level3 | 0.132 | 0.145 | 0.199 | 0.244 | 0.321 |
| 81 | roman | level3 | 0.136 | 0.153 | 0.228 | 0.331 | 0.515 |
| 82 | snack | level3 | 0.209 | 0.212 | 0.528 | 0.675 | 1.663 |
| 83 | snail | level3 | 0.171 | 0.172 | 0.323 | 0.813 | 0.849 |
| 84 | takeoff | level3 | 0.076 | 0.086 | 0.268 | 0.253 | 0.534 |
| 85 | teapot | level3 | 0.412 | 0.425 | 0.722 | 0.733 | 0.728 |
| 86 | tomb | level3 | 0.354 | 0.372 | 0.542 | 0.825 | 0.851 |
| 87 | wealth | level3 | 0.068 | 0.090 | 0.145 | 0.302 | 0.657 |
| 88 | wg5 | level3 | 0.096 | 0.113 | 0.214 | 0.386 | 1.199 |
| 89 | bezier0 | math | 0.046 | 0.046 | 0.050 | 0.053 | 0.058 |
| 90 | bezier | math | 0.065 | 0.077 | 0.106 | 0.102 | 0.103 |
| 91 | bicube | math | 0.054 | 0.058 | 0.067 | 0.068 | 0.060 |
| 92 | folium | math | 0.049 | 0.056 | 0.067 | 0.095 | 0.097 |
| 93 | grafbic | math | 0.131 | 0.133 | 0.171 | 0.168 | 0.169 |
| 94 | helix | math | 0.097 | 0.108 | 0.189 | 0.231 | 0.234 |
| 95 | hyptorus | math | 0.050 | 0.067 | 0.096 | 0.126 | 0.126 |
| 96 | lemnisc2 | math | 0.093 | 0.107 | 0.162 | 0.186 | 0.194 |
| 97 | lemnisca | math | 0.053 | 0.068 | 0.077 | 0.081 | 0.093 |
| 98 | monkey | math | 0.060 | 0.067 | 0.074 | 0.088 | 0.076 |
| 99 | partorus | math | 0.058 | 0.071 | 0.106 | 0.130 | 0.143 |
| 100 | piriform | math | 0.058 | 0.070 | 0.071 | 0.087 | 0.100 |
| 101 | quarcyl | math | 0.050 | 0.046 | 0.065 | 0.065 | 0.075 |
| 102 | quarpara | math | 0.067 | 0.074 | 0.096 | 0.167 | 0.169 |
| 103 | steiner | math | 0.073 | 0.073 | 0.117 | 0.118 | 0.119 |
| 104 | tcubic | math | 0.064 | 0.065 | 0.070 | 0.083 | 0.090 |
| 105 | teardrop | math | 0.071 | 0.072 | 0.076 | 0.083 | 0.082 |
| 106 | torus | math | 0.053 | 0.055 | 0.087 | 0.091 | 0.085 |
| 107 | trough | math | 0.082 | 0.083 | 0.100 | 0.136 | 0.136 |
| 108 | witch | math | 0.053 | 0.069 | 0.083 | 0.128 | 0.131 |
|  | **geomean (108)** |  | 0.072 | 0.084 | 0.125 | 0.181 | 0.248 |
|  | **total (s)** |  | 10.150 | 11.400 | 21.350 | 31.700 | 47.700 |
<!-- /QUALITY_MATRIX_BASELINE -->

---

## 5. Implementation steps

Each step keeps both gates green and re-runs the matrix to record its cost.

1. **Step 0 — baseline.** Run `scripts/renderQualityMatrix.sh` and paste its
   `output/qualityMatrix.tsv` into §4.2 (the md table is the committed copy;
   `output/` is gitignored). Done as part of landing this plan.
2. **Introduce flags, derive from quality.** Add the §3.1 predicates to
   `RenderingConfiguration`, implemented purely in terms of the current
   `quality` int (no shader change yet). No behaviour change; gates green.
3. **Switch shaders to flags.** Replace each `getQuality() <> N` in
   `LocalSurfaceShader`, `DirectLightShader`, `RayShaderPipeline` with the
   matching predicate. Behaviour and goldens unchanged; this is the
   magic-number removal. Re-run matrix → expect no perf change.
4. **Make `setQuality` a preset.** `setQuality(N)` now sets flags per the
   §3.1 table instead of storing a bare int; remove the stored `quality`
   field (or keep it only as a label). Gates green.
5. **Expose direct flag setters / CLI.** Allow `+q` presets *and* explicit
   feature toggles (e.g. a `+f<flags>` style option). Add a couple of
   non-band presets (e.g. "textures, no shadows") and measure them against
   the matrix to demonstrate the interpolation capability.
6. **Add the per-ray lazy mask (§3.2).** Introduce `requiredDetailMask` on the
   ray/hit path; gate `doExtraInformation`/`normal()` on `needsNormal()`. Set
   `DETAIL_NONE` for shadow rays and `q<=1` previews. Re-run matrix → this is
   the step expected to *beat* the baseline at low quality and for
   shadow-heavy scenes; quantify by cell.
7. **(Optional, later) `shadingType` enum.** Align povCpp's lighting selector
   with VITRAL's `ShadingType` so the flag set and VITRAL's
   `RendererConfiguration` share the shading vocabulary too.

---

## 6. Performance comparison method

For every step that can change timing (3, 5, 6):

1. `WIDTH=320 HEIGHT=200 ./scripts/renderQualityMatrix.sh` (same resolution as
   the baseline — relative comparison only needs matching pixels).
2. Average the raw ten-column `output/qualityMatrix.tsv` into the same
   five-band shape as §4.2 ({q0,q1}, {q2,q3}, {q4,q5}, {q6,q7}, {q8,q9}), then
   diff against the committed 108×5 baseline: per-cell ratio `new/baseline`,
   plus per-band-column geometric mean across the 108 scenes (the headline
   "how much did band qN cost change").
3. Record regressions (ratio > 1.0 at any preset level — the presets must not
   get slower) and the wins (ratio < 1.0, expected at low q after Step 6).
4. A step is acceptable when: both gates green, no preset-level cell is
   meaningfully slower than baseline, and Step 6 shows the intended low-q
   speedups.

The 108×5 shape matters because the quality lever's payoff is scene-dependent
(a shadowless scene like `iortest` at the q4-5 band already shows shadows cost
it nothing — see analysis doc §7.2); only the full per-scene matrix reveals
which scene classes the lazy model actually accelerates. The bands, not the
raw ten levels, are the comparison unit: any reinterpretation that keeps
`+qN` byte-identical can only move the band boundaries or split a band into
finer (non-default) presets, so column-by-column band deltas are the
right-sized signal.

---

## 7. Risks and open questions

- **Preset fidelity.** The §3.1 mapping must reproduce all five bands
  (q0-1, q2-3, q4-5, q6-7, q8-9) bit-for-bit across all ten `+qN` values.
  **Resolved** — `testQualities.sh` no longer covers only `iortest`; it now
  also checks all 108 gate scenes at one representative quality per band
  (§4.1, §8). `iortest` alone is no longer a single point of failure for this
  risk; it remains the only *ten-level* witness, but the band-fidelity claim
  itself is now checked corpus-wide.
- **UV on the hit.** VITRAL's mask includes `DETAIL_UV`, but povCpp computes
  UV inside the texture pipeline, not on the hit record. The lazy mask can
  only gate `DETAIL_NORMAL` until UV is lifted onto the hit — a separate piece
  of work, not blocking this plan.
- **Matrix noise.** Wall-clock timing is noisy; use the geometric-mean column
  summary and, for borderline steps, average 2–3 matrix runs.
- **Does orthogonality earn its keep?** If Step 5's non-band presets show no
  useful time/quality point the existing bands don't already hit, the flag
  decomposition still has value (magic-number removal + VITRAL convergence),
  but the per-ray mask (Step 6) becomes the main efficiency justification.

---

## 8. Execution log

**Steps 0–3: done, gates green, no regression.**

- **Step 0** — baseline collected (§4.2, above), before any code change.
- **Step 2** — `RenderingConfiguration` (`src/environment/material/
  RendererConfiguration.h/.cpp`) gained the seven §3.1 predicates
  (`withSurfaceLighting/withShadows/withTextures/withFilteredShadows/
  withRefraction/withBumpMapping/withReflection`), each backed by a new bit
  in the existing `options` bitmask (same pattern as `CSG_ROTH` etc., values
  `4096u`..`262144u`) plus a direct setter (`setShadowsEnabled`, …).
  `setQuality(N)` now fills these bits per the band table in §3.1; `quality`
  itself is kept only as the `getQuality()` label. Found and fixed a real
  bug while wiring this up: `reset()` set `quality = 9` *directly*, bypassing
  `setQuality()` — under the new model that would have left every flag off
  (a silent regression to the q0 preview look on every fresh
  `RenderingConfiguration`). Fixed by calling `setQuality(9)` in `reset()`.
- **Step 3** — all eight `getQuality() <> N` call sites in
  `LocalSurfaceShader.cpp`, `DirectLightShader.cpp`, `RayShaderPipeline.cpp`
  replaced by the matching predicate (e.g. `quality <= 1` →
  `!withSurfaceLighting()`, `quality > 7` → `withBumpMapping()`). No other
  logic changed.
- **Gates**: `scripts/renderQualities.sh` + `scripts/testQualities.sh` →
  "Test passed." (all ten `iortest` images still byte-identical).
  `scripts/renderAll.sh` + `scripts/testAgainstGoldenImages.sh` (full 108-scene
  golden gate) → "Test passed."
- **Performance re-check**: re-ran `scripts/renderQualityMatrix.sh` at the
  same 320×200 resolution, collapsed to the same 108×5 band shape as §4.2,
  and diffed cell-by-cell against the committed baseline. Per-band geometric
  mean of `new/baseline` ratio across all 108 scenes: **q0-1: 1.005, q2-3:
  1.005, q4-5: 1.005, q6-7: 1.003, q8-9: 0.992** — i.e. within wall-clock
  noise of 1.0 in every band; zero cells exceeded a 1.5× ratio. This is the
  expected outcome for Steps 2–3: they only rename comparisons, so the
  baseline table in §4.2 remains valid and was **not** rewritten.

**Steps 4 is effectively folded into Step 2** above (`setQuality` already
*is* the preset setter over the flags, not a bare int store — there was no
reason to land it as a separate, later commit once the flags existed).

**Step 5: implemented, full CLI surface.** The direct setters
(`setShadowsEnabled`, etc.) already existed from Step 2; added a new
multi-character CLI option, `+qflags<letters>`/`-qflags<letters>`
(`src/app/options/CommandLineOptions.cpp`), checked before the single-letter
switch (same pattern as the existing `parallel`/`csgRoth` multi-char options)
so it cannot collide with `+f<format>` (output format) or `+q<N>` (quality
preset). Letters select which bits to toggle — `L`=surface lighting,
`S`=shadows, `T`=textures, `F`=filtered/coloured shadows, `R`=refraction,
`B`=bump mapping, `M`=mirror reflection — and the option's own leading `+`/`-`
(already parsed for every option) decides whether the listed flags turn on or
off. So `+q9 -qflagsS` is "full preset minus shadows" and `+q1 +qflagsT` is
"preview plus textures" — both genuinely new combinations no single `+qN`
expresses, applied on the command line after (and overriding) the `+qN`
preset's bulk assignment. Added to `usage()`'s help text.

Demonstrated the interpolation claim concretely: rendered `etc/level2/
iortest.pov` (320×200) at `+q9 -qflagsS`, `+q1 +qflagsT`, and the three
neighbouring presets (`+q1`, `+q6`, `+q9`), then diffed pairwise with
`compare -metric AE`. Both custom combinations differ from *every* adjacent
preset (AE 8479–54946, all non-zero) — i.e. they are not secretly collapsing
onto an existing band, they render a real point between them. This is exactly
the "couple of non-band presets" demonstration §5's Step 5 asked for; the
demo renders were not added to any golden baseline (they are deliberately
off the band grid, so there is nothing canonical to pin them against) and
were discarded after the check.

Both gates re-run and green (`testQualities.sh`, full
`testAgainstGoldenImages.sh` → "Test passed.") — expected, since no existing
scene or script passes `+qflags`, so the new option is dormant by default.

**Step 6: implemented, in four sub-steps (gates checked after each).**

The actual surface turned out much smaller than the "touches every geometry's
`normal()` override" framing above suggested. `doExtraInformation()` already
sits behind a *single* call site for primary rays
(`RenderEngine::trace`, which already computed the winning hit's normal
exactly once — never per-candidate, never for shadow rays); shadow rays
(`DirectLightShader`'s `lightSourceRay`) never call it at all, since
`ShadowShader` only reads `hitDistance`. So no per-geometry change was needed
— only the one decision point that calls `doExtraInformation`.

- **6.1 — mask storage.** Added VITRAL-named `DETAIL_NONE/POINT/NORMAL/UV/
  TANGENT/ALL` constants and a `requiredDetailMask` field to `RayWithSegments`
  (not `PovRayHit`: the decision must be made *before* the hit exists, to
  decide whether to call `doExtraInformation` at all), with
  `getRequiredDetailMask/setRequiredDetailMask/needsNormal/needsUv/
  needsTangent`. Default `DETAIL_ALL` in the constructor and
  `initializeContainers()`; copied (not reset) in `copyContainersFrom` so
  reflection/refraction child rays inherit it before `RenderEngine::trace`
  recomputes it fresh. Pure addition; nothing reads the field yet.
- **6.2 — set the mask per ray.** `DirectLightShader::shade` sets
  `lightSourceRay`'s mask to `DETAIL_NONE` (documents the existing "shadow
  rays never need a normal" invariant explicitly). `RenderEngine::trace` sets
  the winning ray's mask to `DETAIL_ALL` if `withSurfaceLighting()` else
  `DETAIL_NONE`, right before the `doExtraInformation` call. `withRefraction()`
  is not checked separately because it can only be `true` when
  `withSurfaceLighting()` already is (refraction starts at band q6-7,
  surface lighting at q2-3), so the one flag is exactly the "does any
  downstream reader need this hit's normal" answer — confirmed by grepping
  every read of `hit.n`/`.getIntersection().normal` in `src/render`: only
  `LocalSurfaceShader` (gated on `withSurfaceLighting()`) and
  `RayShaderPipeline`'s refraction branch (gated on `withRefraction()`).
  Still no consumer change at this sub-step.
- **6.3 — wire the gate.** `RenderEngine::trace` now only calls
  `doExtraInformation` (and copies its result into
  `localIntersection.getIntersection().normal`) when
  `localRay->needsNormal()` is true. At q0-1 the field is left holding
  whatever the pooled `Intersection` happened to contain, which is safe by
  construction since nothing reads it there (see 6.2).
- **6.4 — verify.** Both gates green and byte-identical
  (`testQualities.sh` → "Test passed.", full 108-scene
  `testAgainstGoldenImages.sh` → "Test passed.") — expected, since the skip is
  only reachable where the value was already unread. Re-ran the 108×10
  matrix, collapsed to 108×5, diffed against the §4.2 baseline: per-band
  geomean ratio **q0-1: 1.011, q2-3: 1.016, q4-5: 1.014, q6-7: 1.011,
  q8-9: 1.006** — i.e. *no measurable speedup*, including for the
  expensive-normal scenes one would expect to benefit most
  (`bezier`/`bezier0`/`bicube`/`witch`/`monkey`, all quartic/parametric
  surfaces with non-trivial analytic normals: ratios 1.03–1.13, indistinguishable
  from noise). At 320×200 these renders take 0.05–0.08 s; per-process fixed
  cost (scene parse, file I/O, process startup) dominates the per-pixel
  normal computation this step removes, so the saving is real but currently
  unmeasurable against that floor. This directly answers §7's "does
  orthogonality earn its keep?" question for *this* resolution: not yet
  demonstrated. A higher-resolution or normal-computation-heavy scene
  (e.g. a dense height field, or `+w1280 +h800`) would be needed to surface
  the effect, if it exists at a magnitude worth the added state — left as a
  follow-up measurement, not redone here since it would change the
  resolution baseline this plan's §4.2 is pinned to.

**Step 7: implemented, scoped down to what povCpp actually supports.**

VITRAL's `ShadingType` (`SHADING_NOLIGHT/FLAT/GOURAUD/PHONG/COOK_TERRANCE`,
values 0–4) is a real, settable mode that dispatches to five different
shading algorithms in `ShaderSelector`. povCpp has no FLAT (per-vertex flat
shading), GOURAUD (interpolated-normal shading — meaningless for ray-traced
implicit/parametric primitives), or COOK_TERRANCE (Cook-Torrance BRDF)
implementation anywhere in `src/render/shaders/`; it only ever does one of
two things: nothing (the q0-1 "no lighting" preview) or a fixed
ambient + Lambert + Blinn-Phong + Phong-highlight composite (q2+). Adding a
*settable* `shadingType` with those five values would silently misrepresent
support that doesn't exist — exactly the kind of fake convergence §2 warns
against ("must not converge ... the engines are different by necessity").

So the implementation is a single, derived, **read-only** view: added the
same five `SHADING_TYPE_*` constants (same integer values as VITRAL's, for
direct comparability) to `RenderingConfiguration`
(`src/environment/material/RendererConfiguration.h`), plus
`getShadingType() const`, which returns `SHADING_TYPE_NOLIGHT` when
`!withSurfaceLighting()` and `SHADING_TYPE_PHONG` otherwise — i.e. it
recognises povCpp's existing q0-1/q2+ split as the same concept as VITRAL's
NOLIGHT/PHONG, without inventing storage, a setter, or shading paths for the
three modes that don't exist. Purely additive (nothing calls it yet); both
gates re-run and green (`testQualities.sh` and the full
`testAgainstGoldenImages.sh` → "Test passed.") confirms zero behaviour
change, as expected for a derived getter with no callers.

If FLAT/GOURAUD/COOK_TERRANCE shading is ever implemented in povCpp, this is
the seam where they'd plug in: give `shadingType` real storage, a setter, and
have `LocalSurfaceShader`/`DirectLightShader` switch on it instead of always
running the Phong-style path. Not done here — no such shader exists, so
there is nothing to select between yet.

**Gate widened: `testQualities.sh` now covers all 108 scenes, not just
`iortest`** (resolves the §7 "preset fidelity" risk's open suggestion).
`scripts/renderQualities.sh` gained a second tier: reusing the same 108-scene
list as `renderQualityMatrix.sh`/`renderAll.sh`, it renders each scene at one
representative quality per band — `q0, q2, q4, q6, q8` — into
`output/qualities/<level>/<scene>_q<N>.tga`, mirroring `renderAll.sh`'s
per-level (not per-scene-subdirectory) output layout. `iortest`'s original
all-ten-levels render is unchanged and kept as the deeper, single-scene
witness; the new tier is the broader, shallower one. `testQualities.sh`
itself needed **no code change** — its comparison loop already walks
`output/qualities/` recursively and matches by relative path, so it is
structure-agnostic by construction; only its header comment was updated.
540 new renders complete in ~15s (single-threaded would be longer; this runs
all of them as background jobs like `renderAll.sh` does, and the box has 72
cores). The first run's images were copied into
`../referenceTestImagesQualities/<level>/` as the new baseline (550 `.tga`
files total now: the original 10 `iortest_q*` plus 540 new). Re-ran both
gates after this change — `testQualities.sh` → "Test passed.";
`testAgainstGoldenImages.sh` (full 108-scene render at `+q9`) → "Test
passed." — confirming the wider quality gate doesn't interfere with the
unrelated full-resolution golden gate (both scan `output/` but compare
disjoint, non-overlapping file sets by relative path).

**Net effect of this pass**: the quality knob is no longer a pile of
magic-number comparisons (§1's goal #1), povCpp's `RenderingConfiguration`
now structurally exposes the same feature vocabulary as VITRAL's
`RendererConfiguration` (§1's goal #2), arbitrary feature subsets are
expressible via the direct setters and the `+qflags<letters>` CLI option
(§1's goal #3, demonstrated end-to-end), and per-ray laziness exists as a
real, gated mechanism (§1's goal #4) — but,
measured honestly, it has not yet been shown to pay for itself at this
resolution. The refactor is complete and safe (both gates green throughout,
zero image or measured-performance regressions); the speedup VITRAL's
detail-mask model promises remains a hypothesis to re-test at a resolution or
scene where per-pixel cost is not swamped by fixed overhead.
