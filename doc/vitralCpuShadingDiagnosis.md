# CPU shading diagnosis

`ShadersExample` currently traces an analytic `Sphere` in its software path.
`FlatTexturedShader`, `GouraudTextureShader` and the equivalent Phong variant all
delegate to `LightingShader` with the same flags when bump mapping is disabled.
They evaluate illumination at the ray hit; there is no triangle identity,
vertex illumination, or barycentric interpolation in that path.

The OpenGL path consumes `SpherePolyhedralCache` at the selected meridian and
parallel counts. Its flat shader derives a per-triangle normal from the rasterized
triangle, while its Gouraud vertex shader evaluates lighting at vertices and
interpolates that result. Both then apply the interpolated texture coordinates.
The project decision is to keep the analytic sphere in CPU and not emulate flat
faces on it. The differing evaluation models are documented here for context;
this diagnosis is not a request to triangulate or replace the CPU sphere.

## Reproduction

Offline renders at 160×120 with texture and bump disabled, comparing 12×12 and
100×100 tessellation, are in `/tmp/vitral-shading-diagnosis`. The four software
outputs (Flat/Gouraud, both resolutions) have the same SHA-256:
`6be8dd727812281ad85d101196d21700e4abc5a679ceeeebaab3d9af2ce8f885`.
The four OpenGL outputs have distinct hashes. Thus the current software path
ignores both the shading distinction and the tessellation controls. This is a
confirmed implementation gap, not merely a material-loading issue.

No renderer implementation was changed during this diagnosis. In particular,
Phong and Cook–Torrance continue to use the existing analytic-sphere path.
