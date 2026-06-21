#include <cstdio>

#include "environment/camera/Camera.h"
#include "environment/geometry/BoundedGeometry.h"
#include "environment/geometry/SimpleBody.h"
#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/light/Light.h"
#include "environment/material/PovRayMaterial.h"
#include "environment/material/PovRayMaterialUtils.h"
#include "environment/scene/Scene.h"
#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/material/PovRayMaterialConstancy.h"
#include "io/pov/scene/SceneBodyParser.h"
#include "io/pov/scene/SceneParser.h"
#include "io/pov/scene/ScenePostProcessor.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

void
SceneParser::postProcessPhase(Scene *framePtr)
{
    const java::ArrayList<BoundedGeometry*> &objects =
        framePtr->getObjects();
    Light *lightHead = nullptr;
    for (long int i = objects.size() - 1; i >= 0; i--) {
        ScenePostProcessor::linkLights(objects[i], lightHead);
    }
    framePtr->setLightSources(lightHead);
}



void
SceneParser::parse(Scene *framePtr)
{
    ParserContext ctx;
    SceneParser::parse(framePtr, ctx);
}

// Every #declare'd identifier's data (and any constant the scene's default texture
// or shape parsers built along the way) is stashed as a raw `void*` in
// ctx.symbols(), tagged with a ParseGlobals::*_CONSTANT type, and never freed -
// SymbolTable::clear() only memsets the table. By the time parsing finishes, every
// live use of a constant has already taken its own copy (see
// doc/memoryAudit/ownership.md's "Declared-constant aliasing audit"): texture
// identifiers are cloned on use if PovRayMaterialConstancy::isConstant(), and every
// geometry-shaped constant (object/sphere/box/csg/composite/...) is consumed through
// a copy constructor, never aliased directly into the final scene. So once parsing
// is done, the constant table itself is safe to free.
void
SceneParser::freeConstants(ParserContext &ctx)
{
    SymbolTable &symbols = ctx.symbols();
    for (int i = 1; i <= symbols.size(); i++) {
        Constant &constant = symbols.data()[i];
        void * const data = constant.getConstantData();
        if (data == nullptr) {
            continue;
        }
        switch (constant.getConstantType()) {
        case ParseGlobals::OBJECT_CONSTANT:
            delete static_cast<BoundedGeometry *>(data);
            break;
        case ParseGlobals::SPHERE_CONSTANT:
        case ParseGlobals::PLANE_CONSTANT:
        case ParseGlobals::TRIANGLE_CONSTANT:
        case ParseGlobals::SMOOTH_TRIANGLE_CONSTANT:
        case ParseGlobals::QUADRIC_CONSTANT:
        case ParseGlobals::POLY_CONSTANT:
        case ParseGlobals::HEIGHT_FIELD_CONSTANT:
        case ParseGlobals::BOX_CONSTANT:
        case ParseGlobals::BLOB_CONSTANT:
        case ParseGlobals::BICUBIC_PATCH_CONSTANT:
            delete static_cast<SimpleBody *>(data);
            break;
        case ParseGlobals::CSG_INTERSECTION_CONSTANT:
        case ParseGlobals::CSG_UNION_CONSTANT:
        case ParseGlobals::CSG_DIFFERENCE_CONSTANT:
            delete static_cast<CSG *>(data);
            break;
        case ParseGlobals::COMPOSITE_CONSTANT:
            delete static_cast<Composite *>(data);
            break;
        case ParseGlobals::TEXTURE_CONSTANT:
            PovRayMaterialConstancy::unmarkConstant(static_cast<PovRayMaterial *>(data));
            delete static_cast<PovRayMaterial *>(data);
            break;
        case ParseGlobals::VIEW_POINT_CONSTANT:
            delete static_cast<Camera *>(data);
            break;
        case ParseGlobals::COLOUR_CONSTANT:
            delete static_cast<ColorRgba *>(data);
            break;
        case ParseGlobals::VECTOR_CONSTANT:
            delete static_cast<Vector3Dd *>(data);
            break;
        case ParseGlobals::FLOAT_CONSTANT:
            delete static_cast<double *>(data);
            break;
        case ParseGlobals::LIGHT_SOURCE_CONSTANT:
            delete static_cast<Light *>(data);
            break;
        default:
            break;
        }
        constant.setConstantData(nullptr);
    }
}

void
SceneParser::parse(Scene *framePtr, ParserContext &ctx)
{
    // Parses directly into *framePtr rather than building a local Scene and
    // copying it in at the end: Scene owns its final Objects tree (see
    // ~Scene() and doc/memoryAudit/ownership.md), and the compiler-generated
    // copy constructor/assignment only shallow-copies that ArrayList. An
    // intermediate by-value Scene here would have its destructor delete the
    // same BoundedGeometry* the caller's framePtr still needs for the render.
    ctx.degenerateTriangles() = false;
    SceneParser::tokenInit(ctx);
    SceneParser::frameInit(framePtr, ctx);
    SceneParser::parseFrame(framePtr, ctx);
    postProcessPhase(framePtr);
    // Captured after parsing (not in frameInit) so this picks up whatever the
    // final default texture is, even if a `default { texture {...} }` block
    // replaced the one frameInit set. framePtr now owns it; see ~Scene().
    framePtr->captureDefaultTexture(ctx.getDefaultTexture());
    freeConstants(ctx);
    if (ctx.degenerateTriangles()) {
        fprintf(stderr, "Degenerate triangles were found and are being ignored.\n");
    }
}

void
SceneParser::tokenInit()
{
    ParserContext ctx;
    SceneParser::tokenInit(ctx);
}

void
SceneParser::tokenInit(ParserContext &ctx)
{
    ctx.resetTokenStreamHistory();
    ctx.symbols().clear();
}

// Set up the fields in the frame to default values
void
SceneParser::frameInit()
{
    ParserContext ctx;
    Scene frame;
    SceneParser::frameInit(&frame, ctx);
}

void
SceneParser::frameInit(Scene *framePtr, ParserContext &ctx)
{
    ctx.setDefaultTexture(PovRayMaterialUtils::getTexture());
    framePtr->resetForSceneParse(ctx.getAntialiasThreshold());
}

void
SceneParser::parseFrame()
{
    ParserContext ctx;
    Scene frame;
    SceneParser::parseFrame(&frame, ctx);
}

void
SceneParser::parseFrame(Scene *framePtr, ParserContext &ctx)
{
    SceneBodyParser::parseFrame(framePtr, ctx);
}
