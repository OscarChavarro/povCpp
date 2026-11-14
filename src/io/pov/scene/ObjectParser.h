#ifndef __OBJECT_PARSER__
#define __OBJECT_PARSER__

#include "environment/scene/RayCastingComposite.h"
#include "io/pov/geometry/SimpleBodyBuilder.h"

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

    static RayCastingComposite *newComposite(Material *objectTexture);

    // A CSG keeps object transforms on its body layer; every other shape
    // keeps them on its geometry layer.
    static bool transformsBodyLayer(const PovrayBody *object);

    // Parses the shapes of a bounded_by/clipped_by block into regions.
    static void parseRegions(java::ArrayList<PovrayBody*> &regions, ParserContext &ctx);

    // Edits the body in place with the modifiers that follow its shape, up to
    // the closing brace. Composites accept regions and transforms only.
    static void parseObjectModifiers(PovrayBody *object, bool acceptsSurface, ParserContext &ctx);
};

#endif
