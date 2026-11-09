#ifndef __OBJECT_PARSER__
#define __OBJECT_PARSER__

#include "vsdk/toolkit/environment/raycasting/scene/RayCastingComposite.h"
#include "io/pov/geometry/SimpleBodyBuilder.h"
#include "io/pov/scene/ParsedTransformOp.h"

class ObjectParser {
  public:
    static SimpleBodyBuilder *parseShape();
    static SimpleBodyBuilder *parseShape(ParserContext &ctx);
    static RayCastingBody *parseObject();
    static RayCastingBody *parseObject(ParserContext &ctx);
    static RayCastingBody *parseComposite();
    static RayCastingBody *parseComposite(ParserContext &ctx);

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

    static RayCastingBody *buildObject(
        RayCastingGeometry *geometry,
        Material *geometryMaterial,
        Material *objectTexture,
        ColorRgba *objectColor,
        bool noShadowFlag,
        const java::ArrayList<RayCastingBody*> &boundingShapes,
        const java::ArrayList<RayCastingBody*> &clippingShapes,
        Matrix4x4d *transformation = nullptr,
        Matrix4x4d *transformationInverse = nullptr,
        Matrix4x4d *geometryTransformation = nullptr,
        Matrix4x4d *geometryTransformationInverse = nullptr,
        const java::ArrayList<TransformStep> &bodySteps = java::ArrayList<TransformStep>(),
        const java::ArrayList<TransformStep> &geometrySteps = java::ArrayList<TransformStep>());

    static RayCastingComposite *buildComposite(
        RayCastingGeometry *geometry,
        Material *geometryMaterial,
        Material *objectTexture,
        ColorRgba *objectColor,
        bool noShadowFlag,
        const java::ArrayList<RayCastingBody*> &boundingShapes,
        const java::ArrayList<RayCastingBody*> &clippingShapes,
        const java::ArrayList<RayCastingBody*> &simpleBodies,
        Matrix4x4d *transformation = nullptr,
        Matrix4x4d *transformationInverse = nullptr,
        Matrix4x4d *geometryTransformation = nullptr,
        Matrix4x4d *geometryTransformationInverse = nullptr);

    static void extractObjectState(
        RayCastingBody *object,
        RayCastingGeometry *&geometry,
        Material *&geometryMaterial,
        Material *&objectTexture,
        ColorRgba *&objectColor,
        bool &noShadowFlag,
        java::ArrayList<RayCastingBody*> &boundingShapes,
        java::ArrayList<RayCastingBody*> &clippingShapes,
        Matrix4x4d *&transformation,
        Matrix4x4d *&transformationInverse,
        Matrix4x4d *&geometryTransformation,
        Matrix4x4d *&geometryTransformationInverse,
        java::ArrayList<TransformStep> &bodySteps,
        java::ArrayList<TransformStep> &geometrySteps);

    static void extractCompositeState(
        RayCastingComposite *object,
        RayCastingGeometry *&geometry,
        Material *&geometryMaterial,
        Material *&objectTexture,
        ColorRgba *&objectColor,
        bool &noShadowFlag,
        java::ArrayList<RayCastingBody*> &boundingShapes,
        java::ArrayList<RayCastingBody*> &clippingShapes,
        java::ArrayList<RayCastingBody*> &simpleBodies,
        Matrix4x4d *&transformation,
        Matrix4x4d *&transformationInverse,
        Matrix4x4d *&geometryTransformation,
        Matrix4x4d *&geometryTransformationInverse);
};

#endif
