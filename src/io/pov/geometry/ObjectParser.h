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
        Geometry *geometry,
        Material *geometryMaterial,
        Material *objectTexture,
        ColorRgba *objectColor,
        bool noShadowFlag,
        const java::ArrayList<SimpleBody*> &boundingShapes,
        const java::ArrayList<SimpleBody*> &clippingShapes,
        Matrix4x4d *transformation = nullptr,
        Matrix4x4d *transformationInverse = nullptr);

    static Composite *buildComposite(
        Geometry *geometry,
        Material *geometryMaterial,
        Material *objectTexture,
        ColorRgba *objectColor,
        bool noShadowFlag,
        const java::ArrayList<SimpleBody*> &boundingShapes,
        const java::ArrayList<SimpleBody*> &clippingShapes,
        const java::ArrayList<SimpleBody*> &simpleBodies,
        Matrix4x4d *transformation = nullptr,
        Matrix4x4d *transformationInverse = nullptr);

    static void extractObjectState(
        SimpleBody *object,
        Geometry *&geometry,
        Material *&geometryMaterial,
        Material *&objectTexture,
        ColorRgba *&objectColor,
        bool &noShadowFlag,
        java::ArrayList<SimpleBody*> &boundingShapes,
        java::ArrayList<SimpleBody*> &clippingShapes,
        Matrix4x4d *&transformation,
        Matrix4x4d *&transformationInverse);

    static void extractCompositeState(
        Composite *object,
        Geometry *&geometry,
        Material *&geometryMaterial,
        Material *&objectTexture,
        ColorRgba *&objectColor,
        bool &noShadowFlag,
        java::ArrayList<SimpleBody*> &boundingShapes,
        java::ArrayList<SimpleBody*> &clippingShapes,
        java::ArrayList<SimpleBody*> &simpleBodies,
        Matrix4x4d *&transformation,
        Matrix4x4d *&transformationInverse);
};

#endif
