# `src/solidTexture` — revisión y recomendaciones para promover a librería (vitral)

Objetivo: extraer las texturas sólidas (ruido de Perlin, patrones de color/relieve,
mapeo de imágenes) de povCpp hacia `/home/jedilink/VITRAL/vitral` como librería
académica reutilizable: tipos inmutables, código reentrante (apto para
implementaciones concurrentes), simple y autocontenido, con citas claras a
[PERL1985] (Ken Perlin, "An Image Synthesizer", SIGGRAPH '85, Vol. 19 No. 3,
pp. 287-296) y referencias relacionadas.

## Estado actual de `src/solidTexture`

- **`procedural/ProceduralNoise`** — núcleo de ruido de Perlin (`noise`, `dNoise`,
  `turbulence`, `dTurbulence`, `cycloidal`, `triangleWave`), ya extraído como clase
  independiente (ver memoria *ProceduralNoise extraction*). Solo depende de
  `Vector3Dd`.
- **`procedural/ColorTextureFixture` / `BumpTextureFixture`** — funciones de patrón
  (agate, bozo, marble, wood, leopard, onion, ripples, wrinkles, etc.) construidas
  sobre `ProceduralNoise`.
- **`TextureUtils`** — singleton fachada que junta noise + tablas de "wave sources"
  + sampling de `colorMap`.
- **`from2d/ImageTexture`** — proyecciones 2D→3D (planar, esférica, cilíndrica,
  toroidal) + bump/material maps.
- **`SolidTextureStatistics`** — bolsa mutable de contadores inyectada por puntero.

Las citas a `[PERL1985]` con número de página (.289–.291, Apéndice) ya son
correctas y coinciden con lo que dice el paper en pp. 287-291 (Noise(), Dnoise(),
turbulencia, "statistical invariance under rotation/translation"). Buen punto de
partida.

## Problemas frente al objetivo (librería inmutable, reentrante, autocontenida)

1. **`ProceduralNoise` tiene estado mutable construido en dos fases**
   (`new short[4096]`, `new double[267]`, `new double[1000]` en `initialize()`,
   liberados en el destructor). Para una clase reentrante e inmutable, estas tablas
   son de **tamaño fijo conocido en compilación** (4096, `MAXSIZE=267`,
   `SIN_TABLE_SIZE=1000`) → deberían ser arreglos miembro de tamaño fijo
   (`short[4096]`, etc., o `std::array`), llenados en el constructor, sin
   `new`/`delete`/punteros nulos que comprobar. Eso vuelve la clase trivialmente
   segura para usar desde varios hilos (solo lectura tras construir).

2. **`srand(0)` / `rand()` global** en `initTextureTable()`/`initRTable()`. Esto es
   lo más delicado:
   - No es reentrante (estado global de libc).
   - El resultado depende de la implementación de `rand()` de cada
     plataforma/compilador, así que el "byte-identical" actual probablemente solo
     es válido en este toolchain.
   - Para promover a librería académica habría que sustituirlo por un PRNG
     explícito y documentado (p.ej. un LCG simple con la fórmula citada), pero esto
     **cambiaría las tablas y por tanto los goldens** — es un trabajo aparte, no
     trivial, que conviene separar en su propia tarea con sus propios goldens
     nuevos.

3. **APIs por puntero de salida** (`dNoise(Vector3Dd *result, ...)`,
   `r(Vector3Dd *v)`, y todo el patrón
   `color->setR(color->getR() + newColor.getR())` en `ColorTextureFixture`). Esto
   contradice la migración reciente a `Vector3Dd` inmutable (ver memoria *Vector3Dd
   immutable migration*). Para la versión de librería:
   - `dNoise`/`dTurbulence`/`r()` deberían **devolver `Vector3Dd` por valor**.
   - Las funciones de `ColorTextureFixture` deberían **devolver un `ColorRgba`
     "puro"** (el aporte de esa textura) en vez de mutar un acumulador por
     referencia. La política de "sumar capas" es una decisión de composición de
     povCpp (ligada al fix de *wtorus layered texture transforms*), no algo que
     deba vivir en la librería de patrones.

4. **Estado global compartido en `TextureUtils`**: el singleton
   (`textureInstance`) y los arreglos `static` a nivel de archivo
   `frequencyInstance`/`waveSourcesInstance` impiden tener dos instancias
   independientes (p.ej. dos escenas, o tests en paralelo). Para la librería, esto
   debería convertirse en un **objeto inmutable "WaveSourceTable"** construido a
   partir de un `ProceduralNoise` (sin singleton, sin `rand()` directo), que
   `BumpTextureFixture::ripples/waves` reciban como dependencia.

5. **Duplicación**: `fabsInline` está definida idéntica en `ProceduralNoise` y en
   `TextureUtils`. Y ojo con `TextureUtils::floorInline`: **no es equivalente a
   `std::floor`** para enteros exactos negativos (`floorInline(-2.0) == -3`,
   `std::floor(-2.0) == -2`). Es una particularidad heredada de DKB/AAC que afecta
   el patrón `checker` en los bordes de celda — si se "limpia" sin documentarlo y
   testearlo, puede romper goldens. Recomendación: un solo helper, con un
   comentario explicando *por qué* difiere de `floor` estándar, y un test unitario
   que fije ese comportamiento en la frontera.

6. **`SolidTextureStatistics` no debería viajar con la librería**. Es
   instrumentación específica de povCpp (contadores inyectados por puntero,
   mutables). En vitral el patrón equivalente es `RaytraceStatistics` (capa
   separada, con métodos no-op por defecto). Recomendación: la librería de
   noise/texturas **no debe recibir `SolidTextureStatistics*` en su interfaz**; si
   se necesita instrumentación, exponerla como un *hook* opcional (callback) o
   dejar que povCpp la mida por fuera (p.ej. contando llamadas en el wrapper que sí
   se queda en povCpp).

7. **Los enums `SolidTextureColorTextures`, `SolidTextureBumpyTextures`,
   `ImageToSolidTextureProjectionMethods/InterpolationTypes`** son códigos de la
   gramática de POV-Ray (acoplados al parser/IR de povCpp). No deberían promoverse
   a la librería: la librería expone funciones (`agate`, `wood`, `bumps`,
   `planarMap`, ...) y povCpp mantiene su propio mapeo "palabra clave del .pov →
   función de la librería".

8. **`ImageTexture`** mezcla bien lo puro (proyecciones `planarImageMap`,
   `sphericalImageMap`, `biLinear`, `normDist` — sin estado, buenos candidatos) con
   lo dependiente de `ControlledRGBAImageHDRUncompressed` (que es un wrapper
   mutable de imagen, razonable porque una imagen es un buffer). Conviene separar:
   **proyecciones puras** (testeables con coordenadas, sin imagen) vs.
   **muestreo/interpolación sobre buffer**.

## Plan de extracción sugerido (por capas, de menor a mayor riesgo)

1. **Capa 0 — `noise/PerlinNoiseField`** (de `ProceduralNoise`): solo depende de
   `Vector3Dd`. Convertir tablas a arreglos fijos de miembro, quitar
   `SolidTextureStatistics*` del constructor, devolver `Vector3Dd` por valor en
   `dNoise`/`dTurbulence`. **No tocar aún el `srand(0)/rand()`** — documentarlo
   como "reproducibility caveat" con un TODO/issue para un PRNG propio. Esta capa
   es la más fácil de promover ya, y la de mayor reuso (sirve para cualquier
   textura procedural, no solo POV).

2. **Capa 1 — proyecciones de imagen puras** (`planarImageMap`, `sphericalImageMap`,
   `cylindricalImageMap`, `torusImageMap`, `biLinear`, `normDist`): funciones
   libres/estáticas sin estado, fácilmente testeables con casos conocidos del paper
   /POV-Ray.

3. **Capa 2 — patrones de color/relieve** (`ColorTextureFixture`/`BumpTextureFixture`
   reescritos como funciones puras sobre `PerlinNoiseField const&` +
   `WaveSourceTable const&`, devolviendo `ColorRgba`/`Vector3Dd`). Aquí conviene
   primero hacer el cambio de firma (puro, retorno por valor) **dentro de povCpp**,
   validar goldens, y luego mover el archivo.

4. **Capa 3 — `WaveSourceTable`**: objeto inmutable construido desde un
   `PerlinNoiseField` (reemplaza el singleton `TextureUtils` para
   `ripples`/`waves`).

5. Mantener en povCpp: enums de la gramática, `SolidTextureStatistics`/`Statistics`,
   la política de acumulación de `ColorRgba` (suma de capas), y el
   `ImageMap`/`MaterialMap` de alto nivel que usa los tipos de la IR.

## Sobre referencias

La cobertura de `[PERL1985]` ya es buena. Para el README del paquete promovido se
sugiere consolidar:

- `[PERL1985]` — Ken Perlin, "An Image Synthesizer", SIGGRAPH '85, Vol. 19 No. 3,
  pp. 287-296.
- `[PERL1989]` — "Hypertexture", SIGGRAPH '89, p. 253.
- *The RenderMan Companion* (Steve Upstill, Addison Wesley, 1990).
- Atribuciones históricas de DKB/AAC/Scott Taylor (POV-Ray 1.0), preservadas como
  procedencia del algoritmo.

Eso está en línea con el espíritu "académico con citas claras" que pide vitral.

## Siguiente paso propuesto

Empezar por la **Capa 0**: limpiar `ProceduralNoise` a tablas fijas + retorno por
valor (sin tocar `rand()`), validando con los goldens existentes, antes de mover el
archivo a vitral.
