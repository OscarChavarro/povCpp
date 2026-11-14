#ifndef __OBJECT_PARSER__
#define __OBJECT_PARSER__

#include "environment/scene/RayCastingComposite.h"
#include "io/pov/geometry/SimpleBodyBuilder.h"
#include "io/pov/scene/ParsedTransformOp.h"

class ObjectParser {
  public:
    static SimpleBodyBuilder *parseShape();
    static SimpleBodyBuilder *parseShape(ParserContext &ctx);
    static PovrayBody *parseObject();
    static PovrayBody *parseObject(ParserContext &ctx);
    static PovrayBody *parseComposite();
    static PovrayBody *parseComposite(ParserContext &ctx);

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

    static PovrayBody *buildObject(
        RayCastingGeometry *geometry,
        Material *geometryMaterial,
        Material *objectTexture,
        ColorRgba *objectColor,
        bool noShadowFlag,
        const java::ArrayList<PovrayBody*> &boundingShapes,
        const java::ArrayList<PovrayBody*> &clippingShapes,
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
        const java::ArrayList<PovrayBody*> &boundingShapes,
        const java::ArrayList<PovrayBody*> &clippingShapes,
        const java::ArrayList<PovrayBody*> &simpleBodies,
        Matrix4x4d *transformation = nullptr,
        Matrix4x4d *transformationInverse = nullptr,
        Matrix4x4d *geometryTransformation = nullptr,
        Matrix4x4d *geometryTransformationInverse = nullptr);

    static void extractObjectState(
        PovrayBody *object,
        RayCastingGeometry *&geometry,
        Material *&geometryMaterial,
        Material *&objectTexture,
        ColorRgba *&objectColor,
        bool &noShadowFlag,
        java::ArrayList<PovrayBody*> &boundingShapes,
        java::ArrayList<PovrayBody*> &clippingShapes,
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
        java::ArrayList<PovrayBody*> &boundingShapes,
        java::ArrayList<PovrayBody*> &clippingShapes,
        java::ArrayList<PovrayBody*> &simpleBodies,
        Matrix4x4d *&transformation,
        Matrix4x4d *&transformationInverse,
        Matrix4x4d *&geometryTransformation,
        Matrix4x4d *&geometryTransformationInverse);
};

#endif
