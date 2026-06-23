# Plan: fixing CSGByRaySegment (Roth) classification bugs

Reference algorithm: `doc/references/[ROTH1982]_RayCastingForModelingSolids.pdf`
(Scott D. Roth, *Ray Casting for Modeling Solids*, CGIP 18, 109–144, 1982).
Companion: `doc/CSGByRaySegments.md` (original design of the `-csgRoth` path).

This document drives the **bug-fixing phase** of the ray-segment CSG path. The
scaffolding (the `CSGByRaySegment` class, the `-csgRoth` flag, the render and
view scripts) already exists and produces images. Now we triage the visual
results and fix the cases that are genuinely wrong.

> **Rule for the implementing agent (applies to every stage below): DO NOT
> COMMIT.** Do not run `git commit`, `git push`, `git add` with intent to
> commit, amend, tag, or otherwise write to git history at any point. Leave all
> changes in the working tree for the user to review and commit themselves.
> This restriction holds in every section and every step of this plan, with no
> exceptions.

---

## 0. How to reproduce the triage

> Stage rule: **do not commit** — this stage only renders, views and measures;
> leave the working tree untouched in git terms.

```
./scripts/renderCsgByRaySegments.sh        # renders all scenes with -csgRoth into outputCsgByRaySegments/
./scripts/viewImagesCsgByRaySegments.sh    # builds reference|output|diff strips into /tmp/i and shows them
```

Each strip in `/tmp/i/<level>/<scene>.png` is `reference | -csgRoth output |
red-diff-mask`. A fully white diff panel means "no visible difference"; red
pixels mark where the two images disagree.

Objective per-scene signal used throughout this plan (normalized RMSE, 0 = bit
identical):

```
compare -metric RMSE outputCsgByRaySegments/<level>/<scene>.tga \
                     ../referenceTestImages/<level>/<scene>.tga null:
```

---

## 1. Triage taxonomy (observed 2026-06-23, full 108-scene run)

> Stage rule: **do not commit** — classification only; no git history changes.

The user classified the visual output into six cases. Mapping each to a
representative scene with its measured RMSE:

| # | Behaviour | Representative scene | RMSE | Has CSG? | Verdict |
|---|-----------|----------------------|------|----------|---------|
| 1 | Bit-identical, no CSG | `level1/box`, `level1/colors`, `level1/checker2` | 0.000 | no | **ACCEPT** (expected) |
| 2 | Should be clean, but wrong | `math/trough` (Image #3) | 0.048 | yes (intersection clip) | **BUG B** |
| 3 | CSG, expected edge diffs, none visible | `level3/dfwood` (Image #4) | 0.0018 | difference + intersection + inverse | **ACCEPT** |
| 4 | CSG, visible boundary-line diffs | `level3/car` (Image #5) | 0.0090 | yes | **ACCEPT** (expected at boolean edges) |
| 5 | Surface speckle / numeric noise | `level2/lpops2` (Image #6) | 0.0066 | intersection + inverse | **ACCEPT** (minor precision) |
| 6 | Simple CSG fails fundamentally; wrong solid selected | `level1/cantelop` (Image #7) | 0.055 | intersection of spheres + `inverse` | **BUG A** |

Note on Image #2 (`math/witch`, RMSE 0.0072): the user read it as "no CSG,
perfect". It actually carries a small clipping `intersection` and is visually
perfect; it belongs with the ACCEPT cases, not with the truly-zero non-CSG
scenes (`box`, `colors`, …). No action.

Only **two** verdicts require code changes: **Bug A** (`cantelop`) and **Bug B**
(`trough`). Everything else is accepted as expected behaviour of a ray-segment
classifier (boolean edges and sub-pixel precision are allowed to differ; the
gate is RMSE-tolerant, not exact — see §2).

### 1.1 Regression watchlist (likely same family as Bug A)

The worst-RMSE scenes correlate strongly with the user-authored `inverse`
keyword and explicit `intersection`, i.e. with Bug A's mechanism. After fixing
Bug A, re-run the per-scene gate on each of these and expect a large RMSE drop:

| Scene | RMSE | inverse | intersection |
|-------|------|---------|--------------|
| `level2/room` | 0.212 | yes | yes |
| `level2/skyvase` | 0.168 | 2 | 3 |
| `level2/esp01` | 0.112 | yes | yes |
| `level2/pacman` | 0.088 | 3 | 2 |
| `level1/ballbox1` | 0.085 | 2 | 1 |
| `level3/snack` | 0.074 | many | many |
| `level1/cantelop` | 0.055 | yes | yes |

Scenes whose high RMSE is **not** `inverse`-related and is accepted as case 5
(numeric noise, no CSG): `level1/blob` (0.16), `math/bezier0` (0.12),
`level3/snail` (0.09). Do **not** chase these.

---

## 2. The gate: ONE scene, low-but-not-exact comparison

> Stage rule: **do not commit.** You may create `scripts/gateCsgScene.sh` in the
> working tree and run it, but do not commit it (or anything else) to git.

**Do not run the full 108-scene render/gate while iterating.** It is slow and
drowns the signal. For every fix, render and compare **only the scene under
test**, and require a **low normalized RMSE, not bit-exactness** (the Roth path
is legitimately non-identical at boolean edges).

Suggested helper — create `scripts/gateCsgScene.sh`:

```bash
#!/usr/bin/env bash
# Single-scene gate for the -csgRoth path. Renders ONE scene with -csgRoth and
# compares it (RMSE-tolerant) against its golden reference image.
#   usage: scripts/gateCsgScene.sh <level>/<scene> [RMSE_THRESHOLD]
#   e.g.   scripts/gateCsgScene.sh level1/cantelop 0.02
set -euo pipefail

rel="$1"                                   # e.g. level1/cantelop
threshold="${2:-0.02}"
scene="$(basename "$rel")"                 # cantelop
level="$(dirname "$rel")"                  # level1
povray="$(pwd)/build/povray"
reference="../referenceTestImages/${rel}.tga"
out_dir="$(mktemp -d)"
out="${out_dir}/${scene}.tga"

# Resolve the scene's source dir + include path the same way renderCsgByRaySegments.sh does.
# Most scenes live in etc/<level>/<scene>.pov with include at ../include; a few
# (bumpmap, car, drums, fish13, ionic5, ntreal, pencil, piece2, piece3, snail,
# teapot) live one directory deeper — adjust scene_dir/include for those.
scene_dir="etc/${level}"
include="../include"
( cd "${scene_dir}" && "${povray}" \
    "+l${include}" "+i${scene}.pov" "+o${out}" \
    +w1280 +h800 -d -v +x +ft -csgRoth -parallel >/dev/null 2>&1 )

rmse="$(compare -metric RMSE "${out}" "${reference}" null: 2>&1 | sed -n 's/.*(\(.*\))/\1/p')"
echo "RMSE(${rel}) = ${rmse}  (threshold ${threshold})"
awk -v r="${rmse}" -v t="${threshold}" 'BEGIN{ exit !(r+0 <= t+0) }' \
  && echo "GATE PASS" \
  || { echo "GATE FAIL"; exit 1; }
```

Always print the RMSE even on failure: while iterating, a drop from 0.055 →
0.012 is progress even if it has not yet crossed the threshold. Pick the
threshold from the scene's accepted neighbours (≈0.02 is the project-wide
tolerance already used by `scripts/testCsgRoth.sh`).

Run the full `scripts/renderCsgByRaySegments.sh` + `scripts/testCsgRoth.sh`
**once at the end**, to confirm no regression across all scenes.

---

## 3. Root-cause analysis

> Stage rule: **do not commit** — analysis only; no code or git changes here.

### 3.1 Bug A — `inverse`-flagged primitives get reversed parity

`CSGByRaySegment::buildRaySegments`
(`src/environment/geometry/volume/compound/CSGByRaySegment.cpp`) derives each
crossing's In/Out parity from the **raw geometric surface normal**:

```cpp
entering = ray->getDirection().dotProduct(surfaceNormal) < 0.0;
```

and derives `initialInside` (the state before the first crossing) from
`child->doContainmentTest(...)`.

These two sources use **inconsistent inside/outside conventions** for any leaf
carrying the user-authored `inverse` keyword:

- `Sphere::normal()` (`src/environment/geometry/volume/Sphere.cpp:158-165`)
  returns `(point − center)/radius` **unconditionally** — it never flips for an
  inverted sphere.
- `Sphere::doContainmentTest()` (`Sphere.cpp:145-156`) **does** flip on
  `isInverted()`.

So for `sphere { … inverse }` the solid's interior is the *exterior* of the
geometric sphere, `doContainmentTest` reports that correctly, but the
normal-based `entering` is computed against the *outward* normal and comes out
**reversed**. The interval merge then mixes a correct `initialInside` with
back-to-front crossings and corrupts the result.

This is exactly `cantelop.pov`: each melon half is
`intersection { sphere(1) ∩ sphere(0.65 inverse) ∩ plane }`. The
`sphere{0.65 inverse}` is meant to hollow out the shell; with reversed parity
the hollow is filled, so the bowls render solid (Image #7). The default
point-membership CSG path is immune because it classifies **every** operand via
`doContainmentTest` (inversion-aware) and never via the bare normal.

`dfwood` (case 3, RMSE 0.0018) is *not* affected even though it uses `inverse`,
because its `inverse` participates through a `difference` operator (handled by
`combineDifference`, which never reads a leaf normal for the subtrahend's
membership), not as an `intersection` operand. This confirms the mechanism is
specifically "**`inverse` leaf used as an `intersection`/`union` operand**".

### 3.2 Bug B — open quartic surface as an intersection-clip operand

`trough.pov` is `intersection { quartic(saddle) ∩ Unit_Cube }`, no `inverse`.
Under `-csgRoth` an extra red lens leaks out of the bottom of the clip box
(Image #3, RMSE 0.048).

Same smell as Bug A, different trigger:
`PolynomialShape::doContainmentTest()`
(`src/environment/geometry/volume/polynomial/PolynomialShape.cpp:792-810`)
defines inside as `evaluatePolynomial(...) < tol` (flipped by `inverted`),
whereas `PolynomialShape::normal()` (`PolynomialShape.cpp:812-839`) returns the
gradient direction and, like `Sphere`, **never consults `inverted`** and does
not guarantee its sign points away from the `doContainmentTest` interior.

Leading hypothesis: for this saddle the normal used for parity points **into**
the half-space that `doContainmentTest` calls "inside", so the open surface's
crossings are classified backwards on one sheet, and the box-intersection keeps
a segment it should drop → the bottom leak. Open (non-closed) surfaces also
produce odd crossing counts within a clip volume, which stresses the
`initialInside = !crossings[0].entering` assumption.

Bug B must be **confirmed empirically** with the per-scene gate before/after,
not assumed — its mechanism is plausibly the same normal-vs-containment
disagreement as Bug A, which is why the recommended fix below addresses both.

---

## 4. Fix strategy (ordered)

The unifying defect is: **per-crossing parity comes from the raw normal, but the
authoritative inside/outside convention lives in `doContainmentTest`.** Make
parity agree with containment.

> Stage rule for ALL steps in this section: **do not commit.** Implement the
> fix in the working tree, validate it with the single-scene gate, and stop.
> Leave every change uncommitted for the user to review. Do not `git commit`,
> `git push`, or `git add`-to-commit at the end of any step.

### Step 1 — Bug A, `cantelop` (clean reproducer, do this first)

> Step rule: **do not commit** — leave the Step 1 fix uncommitted in the working
> tree once its gate passes.

Baseline: `scripts/gateCsgScene.sh level1/cantelop` → expect RMSE ≈ 0.055.

Candidate fixes (prefer the most local that passes the gate):

1. **Containment-derived parity (recommended, fixes A and likely B).** In
   `buildRaySegments`, instead of `entering = dir·normal < 0`, decide the side
   the ray moves *into* by sampling the child's own `doContainmentTest` just
   past the crossing along the ray (`point + dir·ε`). This reuses the single
   authoritative inside-test for every primitive (spheres, quartics, inverted
   or not) and cannot disagree with `initialInside`. Keep the ε consistent with
   the existing no-crossing sampling already in `buildRaySegments` (it already
   uses `2·Config::SMALL_TOLERANCE`). Cost: one extra containment test per
   crossing — acceptable; revisit only if profiling shows it matters.

2. **Inversion-aware normal parity (narrower).** Flip `entering` when the
   originating leaf is inverted. Requires a uniform way to ask a hit "was your
   geometry inverted?" (today `inverted` is private per shape:
   `Sphere::isInverted()`, `PolynomialShape::inverted`, …). This needs plumbing
   through `IntersectionAttributes`/`getHitGeometry()` and does **not**
   generalize to open quartics, so it is the fallback, not the first choice.

Do **not** "fix" this by flipping the sign inside `Sphere::normal()` /
`PolynomialShape::normal()`: those normals feed shading and the default
(non-Roth) renderer, which is gated bit-identical — changing them would regress
the golden images.

Gate: `cantelop` RMSE must drop to the accepted band (≈ ≤0.02, in line with
neighbouring CSG scenes). Boundary-edge residue is fine.

### Step 2 — regression watchlist (§1.1)

> Step rule: **do not commit** — measurement and note-taking only; record
> before/after RMSE in the working tree / PR notes without committing.

With Step 1 in place, run the per-scene gate on `room`, `skyvase`, `esp01`,
`pacman`, `ballbox1`, `snack`. Expect large RMSE drops. Record before/after in
the PR. Any scene that does **not** improve is a distinct bug — note it, do not
fold it silently into Step 1.

### Step 3 — Bug B, `trough`

> Step rule: **do not commit** — leave any Step 3 fix uncommitted in the working
> tree once its gate passes.

Re-measure `scripts/gateCsgScene.sh math/trough` after Step 1. If candidate fix
#1 (containment-derived parity) already removed the bottom leak, Bug B is
closed — confirm and move on. If it persists, investigate the **open-surface**
angle specifically:

- whether the quartic yields an odd number of crossings inside the clip box and
  how `initialInside`/the interval merge handle it;
- whether `quarticNormal`'s sign matches `evaluatePolynomial`'s inside sign.

Add a focused note to `doc/CSGByRaySegments.md` once the mechanism is confirmed.

---

## 5. Constraints (do not regress)

> Stage rule: **do not commit** — these constraints must hold in the working
> tree; verifying them never involves writing to git history.

- **Do not commit.** The implementing agent must never create commits, push, or
  otherwise alter git history; all work stays in the working tree for the user.
- **Default path stays byte-identical.** Without `-csgRoth`, output must remain
  bit-for-bit the golden. Verify with the existing default gate
  (`scripts/renderAll.sh` + `scripts/testAgainstGoldenImages.sh` → "Test
  passed."). This is why §4 forbids touching shared `normal()` methods.
- **Scope the change to the Roth path**, ideally entirely inside
  `CSGByRaySegment` / its helper classes (`RaySegments`,
  `RaySegmentCrossing`). Leaf geometry classes should not change behaviour for
  the default renderer.
- **VITRAL is not touched.**
- Iterate with the single-scene gate (§2); run the full
  `renderCsgByRaySegments.sh` + `testCsgRoth.sh` only as the final check.

---

## 6. Definition of done

> Stage rule: **do not commit.** "Done" means all criteria below hold in the
> working tree and are reported to the user — it does **not** mean committing.
> The user performs the commit; the agent never does.

1. `gateCsgScene.sh level1/cantelop` passes (RMSE in the accepted band); the
   bowls render hollow.
2. `gateCsgScene.sh math/trough` passes; no bottom leak.
3. Every regression-watchlist scene (§1.1) improves; none regresses.
4. Full `scripts/testCsgRoth.sh` passes at its RMSE threshold.
5. Default (non-Roth) golden gate still reports "Test passed."
6. The confirmed root cause and fix are summarized in
   `doc/CSGByRaySegments.md`.
```
