#ifndef __WIREFRAME_RENDERER__
#define __WIREFRAME_RENDERER__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/render/RenderingElement.h"
class Calligraphic2DBuffer;
class SimpleBody;
class Camera;
class Matrix4x4d;
class Vector3Dd;

class WireframeRenderer : public RenderingElement {
public:
    static void execute(Calligraphic2DBuffer* outLineSet,
                        java::ArrayList<SimpleBody*>& simpleBodies,
                        const Camera* camera);
};

#endif
