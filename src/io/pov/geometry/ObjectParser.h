#ifndef __OBJECT_PARSER__
#define __OBJECT_PARSER__

#include "environment/scene/SimpleBody.h"
#include "environment/scene/Composite.h"
#include "io/pov/geometry/SimpleBodyBuilder.h"
#include "io/pov/context/ParserContext.h"

class ObjectParser {
  public:
    static SimpleBodyBuilder *parseShape();
    static SimpleBodyBuilder *parseShape(ParserContext &ctx);
    static SimpleBody *parseObject();
    static SimpleBody *parseObject(ParserContext &ctx);
    static SimpleBody *parseComposite();
    static SimpleBody *parseComposite(ParserContext &ctx);

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

    static SimpleBody *buildObject(
        TransformedGeometry *geometry,
        Material *geometryMaterial,
        Material *objectTexture,
        ColorRgba *objectColor,
        bool noShadowFlag,
        const java::ArrayList<TransformedGeometry*> &boundingShapes,
        const java::ArrayList<TransformedGeometry*> &clippingShapes);

    static Composite *buildComposite(
        TransformedGeometry *geometry,
        Material *geometryMaterial,
        Material *objectTexture,
        ColorRgba *objectColor,
        bool noShadowFlag,
        const java::ArrayList<TransformedGeometry*> &boundingShapes,
        const java::ArrayList<TransformedGeometry*> &clippingShapes,
        const java::ArrayList<SimpleBody*> &simpleBodies);

    static void extractObjectState(
        SimpleBody *object,
        TransformedGeometry *&geometry,
        Material *&geometryMaterial,
        Material *&objectTexture,
        ColorRgba *&objectColor,
        bool &noShadowFlag,
        java::ArrayList<TransformedGeometry*> &boundingShapes,
        java::ArrayList<TransformedGeometry*> &clippingShapes);

    static void extractCompositeState(
        Composite *object,
        TransformedGeometry *&geometry,
        Material *&geometryMaterial,
        Material *&objectTexture,
        ColorRgba *&objectColor,
        bool &noShadowFlag,
        java::ArrayList<TransformedGeometry*> &boundingShapes,
        java::ArrayList<TransformedGeometry*> &clippingShapes,
        java::ArrayList<SimpleBody*> &simpleBodies);
};

#endif
