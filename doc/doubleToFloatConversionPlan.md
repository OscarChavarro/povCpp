# Double-to-Float Conversion Plan

This document records the steps required to migrate the codebase from `double`
to `float` precision.  The intent is to have a repeatable recipe that can be
applied again in the future, regardless of how the codebase evolves in the
interim.  No particular directory layout is assumed; every step is expressed in
terms of patterns rather than specific paths.

---

## Step 1 — Mechanical type substitution

Search every source file for the following identifiers and replace them
uniformly:

| From | To |
|---|---|
| `double` | `float` |
| `Double` (boxed/wrapper type) | `float` |
| `Vector3Dd` | `Vector3Df` |
| `Matrix4x4d` | `Matrix4x4f` |
| `Quaterniond` | `Quaternionf` |

This includes declarations, parameter types, return types, template arguments,
and cast expressions.  Do the substitution globally, then fix the build
incrementally as described in the following steps.

---

## Step 2 — Build repair

After the mechanical pass the build will fail in several predictable categories.
Work through them in any order:

### 2a. Missing methods on the float vector/matrix types

`Vector3Df` is a smaller API surface than `Vector3Dd`.  Add the missing
operations as inline definitions directly in the header so no new translation
unit is needed:

- `multiply(const Vector3Df& other)` — component-wise product
- `midpoint(const Vector3Df& other)` — component-wise average
- `normalized()` alias — if the `double` side used `normalizedFast()`, replace
  every call with `.normalized()` (which already exists on `Vector3Df`).

### 2b. Matrix4x4f — new implementation file and missing methods

`Matrix4x4f` exists as a thin wrapper around `Matrix4x4d` (composition via a
private `d_` member) but it must expose the same operations that callers use.
Three methods need native-float implementations — **do not delegate internally
to `Vector3Dd` or `double` arithmetic**; implement them directly in float:

- `transformPoint(const Vector3Df& v)` — applies the full 4×4 transform
  (with translation).
- `transformDirection(const Vector3Df& v)` — applies the 3×3 rotation block
  only (no translation column).
- `axisRotationRodrigues(Matrix4x4f* inverse, Vector3Df* eulerAngles)` — builds
  a rotation matrix from Euler angles using the Rodrigues product, then stores
  the transpose as the inverse.

Add the new `.cpp` file to the build system (e.g. `CMakeLists.txt`).

Quaternion-related methods (`exportToQuaternion`, `importFromQuaternion`) can
delegate to the `double` internals via explicit conversion, since they are not
on the hot path.

### 2c. ArrayList<float> template instantiation

If the project uses explicit template instantiation for `ArrayList<T>`, add
`template class ArrayList<float>;` alongside the existing `ArrayList<double>`
entry in whatever file or CMake rule generates the instantiation list.

### 2d. Polynomial solver APIs — keep local buffers as double

The polynomial and quartic solvers (`PolynomialSolver`, `QuarticSolver`,
`QuadraticSolver`) have `double*` APIs.  Do **not** change their signatures.
Instead, in each caller:

1. Declare a local `double` array for solver input/output.
2. Call the solver into the `double` buffer.
3. Copy results to the surrounding `float` context with an explicit cast:
   ```cpp
   double droots[N];
   int cnt = solver.solve(droots, ...);
   for (int k = 0; k < cnt; k++) depths[k] = (float)droots[k];
   ```

### 2e. `sscanf` format specifier — `%lf` must become `%f`

`%lf` writes a `double` (8 bytes) into the target variable.  When the target
is now a `float` (4 bytes) this is undefined behaviour and produces a
stack-smashing or stack-corruption crash at runtime.

Find every `sscanf` call that reads into a numeric float variable and change
the format specifier from `"%lf"` to `"%f"`.  Common locations:

- Lexer/tokeniser numeric literal parsing.
- Command-line option parsing for threshold/quality values.

### 2f. Floating-point literal suffixes

The compiler will warn (or error) on implicit `double→float` narrowing in
`constexpr` and template contexts.  Append the `f` suffix to every affected
literal: `0.0` → `0.0f`, `1.0` → `1.0f`, `0.77` → `0.77f`, etc.

### 2g. Output adapter — delegate defaultFileName()

If an `ImageOutputAdapter` (or similar adapter) wraps an `ImageOutput` but
inherits its `defaultFileName()` from a base class that returns `nullptr`,
override the method and delegate to the inner object:

```cpp
const char* ImageOutputAdapter::defaultFileName() const {
    return delegate->defaultFileName();
}
```

Omitting this causes a null-pointer `strncpy` crash when the renderer is
invoked without an explicit output filename (`+o` flag).

---

## Step 3 — Adjust precision constants

`float` has roughly 7 decimal digits of precision vs 15–16 for `double`.
Epsilon values that were calibrated for double must be relaxed; values that
were already much looser than double precision can stay as-is.

Concrete mappings found to work in practice:

| Constant | double value | float value |
|---|---|---|
| `Config::INTERSECTION_EPSILON` | `1.0e-5` | `1.0e-4f` |
| `Config::POLYNOMIAL_SOLVER_EPSILON` | `1.0e-10` | `1.0e-5f` |
| `Config::PARAMETRIC_CURVE_EPSILON` | `1.0e-10` | `1.0e-5f` |
| `Config::SMALL_TOLERANCE` | `0.001` | `0.001f` (unchanged) |
| `Config::MAX_DISTANCE` | `1.0e7` | `1.0e7f` (unchanged) |
| `VSDK::EPSILON` | `1e-6` | `1e-4f` |

The general rule: if the original value was tighter than `1e-6`, relax it by
roughly one order of magnitude toward `1e-4`; if it was already `1e-3` or
coarser, leave the numeric value unchanged and just add the `f` suffix.

---

## Step 4 — Height-field traversal: clamp pixel indices

This is a float-specific correctness fix, not a compile error.

**Root cause.**  The ray-traversal loop in the height-field intersector computes
integer pixel indices using `floor()` and `ceil()` on floating-point coordinates.
With `double` arithmetic the rounding error is sub-epsilon so a coordinate
that should be `0.0` stays non-negative.  With `float`, the same coordinate
can come out as `-ε`, and `floor(-ε)` = `-1`, producing a negative array index.
Accessing `Map[-1]` (a pointer table whose entries are 8 bytes each) reads memory
before the allocated buffer — caught by ASan as a heap-buffer-overflow.

**Fix.**  After each index computation in the traversal function(s), clamp to
the valid range before the first use:

```cpp
const int maxIx = hField->mapInnerSize - 2;
const int maxIz = hField->mapOuterSize - 2;

// ... compute ix from floor/ceil ...
if (ix < 0) ix = 0;
if (ix > maxIx) ix = maxIx;

// ... compute iz from floor/ceil ...
if (iz < 0) iz = 0;
if (iz > maxIz) iz = maxIz;
```

Apply this at every point where `ix`/`iz` are first computed — there are
typically four cases: `(xDom, isdx≥0)`, `(xDom, isdx<0)`, `(!xDom, isdz≥0)`,
`(!xDom, isdz<0)`.

Add a symmetric bounds guard inside `getHeightAt` itself as a second line of
defence:

```cpp
float getHeightAt(int x, int z, const HeightField* hf) {
    if (x < 0) x = 0;
    if (z < 0) z = 0;
    if (x >= hf->mapInnerSize) x = hf->mapInnerSize - 1;
    if (z >= hf->mapOuterSize) z = hf->mapOuterSize - 1;
    return hf->Map[z][x];
}
```

The same class of bug (negative index from `floor(-ε_float)`) can appear in
any other traversal that converts floating-point cell coordinates to integer
indices.  Apply the same clamp pattern wherever that conversion occurs.
