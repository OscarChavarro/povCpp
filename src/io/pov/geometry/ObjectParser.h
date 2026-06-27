#ifndef __OBJECT_PARSER__
#define __OBJECT_PARSER__

#include "environment/geometry/BoundedGeometry.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/scene/SimpleBody.h"
#include "io/pov/context/ParserContext.h"

class ObjectParser {
  public:
    static SimpleBody *parseShape();
    static SimpleBody *parseShape(ParserContext &ctx);
    static BoundedGeometry *parseObject();
    static BoundedGeometry *parseObject(ParserContext &ctx);
    static BoundedGeometry *parseComposite();
    static BoundedGeometry *parseComposite(ParserContext &ctx);

  private:
    // An untextured object/composite starts out wearing the scene's shared
    // default texture directly (see objectTexture initialization in
    // parseObject/parseComposite). Transforming it in place would mutate
    // that shared instance for every other object relying on it, so callers
    // must copy it out first whenever it is still the canonical instance AND
    // the transform would actually touch it (matching PovRayMaterial's own
    // needsTransform gate) -- copying unconditionally would make
    // objectTexture no longer pointer-equal to ctx.getDefaultTexture(),
    // which the TEXTURE_TOKEN handling relies on to decide between
    // replacing and layering.
    static Material *ensurePrivateTexture(Material *objectTexture);

    static BoundedGeometry *buildObject(
        SimpleBody *geometry,
        Material *objectTexture,
        ColorRgba *objectColor,
        bool noShadowFlag,
        const java::ArrayList<SimpleBody*> &boundingShapes,
        const java::ArrayList<SimpleBody*> &clippingShapes);

    static Composite *buildComposite(
        SimpleBody *geometry,
        Material *objectTexture,
        ColorRgba *objectColor,
        bool noShadowFlag,
        const java::ArrayList<SimpleBody*> &boundingShapes,
        const java::ArrayList<SimpleBody*> &clippingShapes,
        const java::ArrayList<BoundedGeometry*> &simpleBodies);

    static void extractObjectState(
        BoundedGeometry *object,
        SimpleBody *&geometry,
        Material *&objectTexture,
        ColorRgba *&objectColor,
        bool &noShadowFlag,
        java::ArrayList<SimpleBody*> &boundingShapes,
        java::ArrayList<SimpleBody*> &clippingShapes);

    static void extractCompositeState(
        Composite *object,
        SimpleBody *&geometry,
        Material *&objectTexture,
        ColorRgba *&objectColor,
        bool &noShadowFlag,
        java::ArrayList<SimpleBody*> &boundingShapes,
        java::ArrayList<SimpleBody*> &clippingShapes,
        java::ArrayList<BoundedGeometry*> &simpleBodies);
};

#endif
