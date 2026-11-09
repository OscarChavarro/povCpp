#ifndef __HIDDEN_LINE_RENDERER__
#define __HIDDEN_LINE_RENDERER__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/render/RenderingElement.h"
class Camera;
class Calligraphic2DBuffer;
class SimpleBody;
class PolyhedralBoundedSolid;
class _PolyhedralBoundedSolidFace;

class HiddenLineQuerySolid {
public:
    SimpleBody* body;
    PolyhedralBoundedSolid* solid;
    bool ownsSolid;

    HiddenLineQuerySolid()
        : body(0), solid(0), ownsSolid(false)
    {
    }
};

class HiddenLineRenderer : public RenderingElement {
public:
    static int isFaceVisibleFromCamera(
        _PolyhedralBoundedSolidFace* face,
        const Camera* camera);

    static void executeAppelAlgorithm(
        java::ArrayList<SimpleBody*>& inSimpleBodyArray,
        const Camera* inCamera,
        Calligraphic2DBuffer* outVisibleContourLineSet,
        Calligraphic2DBuffer* outVisibleNonContourLineSet,
        Calligraphic2DBuffer* outHiddenLineSet);
};

#endif
