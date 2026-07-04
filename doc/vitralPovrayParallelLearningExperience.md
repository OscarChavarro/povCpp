Tenemos dos árboles de código fuente que se vienen trabajabando en paralelo:
1. VITRAL
2. POVCPP

Ambos árboles se estan trabajando de tal manera que se estudian, y a medida que aparecen y se identifican elementos conceptuales, se intenta mejorarlos.
En el documento doc/designObjectives.md se documentan los objetivos de diseño en estas mejoras graduales. A medida que se aprende algo en uno de los
árboles, se intenta integrarlo en el otro:
1. Cuando se aprende algo desde el estudio de vitral se mejora povcpp
2. Cuando se aprende algo desde el estudio de povcpp se mejora vitral

Y posteriormente, cuando un concepto se entiende por completo y de buena calidad, este concepto se integra en el arbol de vitral, que es la arquitectura
de destino que busca centralizar todos los componentes, y esa estructura se replica en povcpp, hasta que se logre hacer que la copia de povcpp sea
eliminada, y que VITRAL se termine usando como una librería sobre la cual se apoya povcpp.

Nótese que este avance se plasma en la carpeta base, que contiene aquel conjunto de vitral que se usa en povcpp. Algunos ejemplos de estructuras-clases
que se han unificado incluyen:
1. La capa de clases en el namespace ::java que ofrecen en un estilo similar a JDK una serie de clases base comunes como ArrayList y Math
2. La capa de álgebra lineal vitral con clases como Vector3Dd y Matrix4x4d
3. La capa de texturas sólidas basadas sobre ruido de Perlin
4. La capa de solver polinómico

El objetivo de la siguiente fase en este trabajo es lograr lo mismo a nivel de Geometry. Lo que queremos es:
1. Expresar el paquete environment/geometry de povcpp de la manera más similar a vitral. Nótese que en doc/vitralNormalizationAnalysis.md se
describen los objetivos de esta normalización.
2. Identificar las operaciones que existen en povcpp y que no existen en vitral, para en un futuro cercano llevarlas hacia vitral
3. Mover esas operaciones a Vitral, la idea es que vitral absorba la capa envirnonment/geometry por completo, haciendo posible eliminar ese paquete de
povcpp, hacer que povcpp sea una aplicación que usa la librería vitral y finalizar esa duplicación.
4. Seleccionar la implementación más eficiente entre las dos. Cuando alguna implementación de Vitral sea superior a la de povcpp se queda. Cuando la
implementación de povcpp sea superior a la de vitral, la de vitral se reemplazará
5. Las operaciones existentes en vitral que no estan en povcpp se mantienen. Por ejemplo, la habilidad de las geometrías de exportar un modelo
PolyhedralBoundedSolid.
6. Nótese que Vitral tiene operaciones que facilitan pintar las geometrías con JOGL / OpenGL y ports en varios lenguajes de programación (inicialmente
C++ y Java). Cuando realicemos cambios en la librería vitral, debemos mantener paridad 1:1 entre ports. Vitral también ofrece operaciones de persistencia
que permiten crear estas geometrías a partir de diversos formatos de archivo.
7. Agregar soporte a CSG a los rayos de Vitral, tanto para el método de leyes de Morgan como para el método de Roth/segmentos.

Nótese que antes de mover las Geometry de povcpp a vitral, tendremos que conciliar varios modelos, como Material, RayHit. Al final, el modelo
PovRayMaterial, la capa io, y la capa de render/bakedScene se consideran partes de la aplicación PovRay y eso no se buscará mover a Vitral de
momento.

Uno de los aspectos que se ha descubierto en el estudio de PovCpp es el manejo de "bake" de escena. Esto no existe de ninguna manera comparable en Vitral.
La razón de haber hecho el decoupling y luego las múltiples fases de mejora de rendimient, han dado como resultado una capa de baking en PovCpp.

Uno de los aspectos que se tienen identificados y que no están presentes ni en Vitral ni en el PovCpp original es el uso de jerarquías de volúmenes
envolventes, y eso será trabajo por hacer próximamente.

Respecto a la capa de Shaders se tendrá una situación similar. Se busca poder implementar estos shaders tres veces: una para el modelo de cómputo en CPU
y otro para el modelo de cómputo con shaders tanto en GLSL como sn SPIR-V. Esta parte de momento está fuera de alcance y no se trabajará en eso todavía.
