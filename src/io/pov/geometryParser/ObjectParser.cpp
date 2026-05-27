#include "io/pov/ParserContext.h"
#include "common/LegacyBoolean.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "io/pov/Parse.h"

#include "environment/camera/Camera.h"
#include "environment/geometry/elements/Triangle.h"
#include "environment/geometry/surface/InfinitePlane.h"
#include "environment/geometry/surface/parametric/ParametricPatch.h"
#include "environment/geometry/volume/Blob.h"
#include "environment/geometry/volume/Box.h"
#include "environment/geometry/volume/HeightField.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/geometry/volume/Sphere.h"
#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/geometry/volume/polynomial/PolynomialShape.h"
#include "environment/scene/SimpleBodyFactory.h"
#include "environment/light/Light.h"
#include "common/logger/Logger.h"
#include <cmath>
#include <cstdlib>

namespace {
bool shouldLogMonkeyDiagnostics()
{
    const char *flag = std::getenv("POVCPP_DIAG_MONKEY");
    return flag != nullptr && flag[0] != '\0';
}

int countGeometryChain(const Geometry *g)
{
    int count = 0;
    for (const Geometry *cur = g; cur != nullptr; cur = cur->nextObject) {
        ++count;
    }
    return count;
}

void logCsgOnce(const char *prefix, const CSG *csg)
{
    if (!shouldLogMonkeyDiagnostics() || csg == nullptr) {
        return;
    }
    Logger::info(
        "[DIAG-MONKEY] %s csg type=%d shapeCount=%d\n",
        prefix, csg->Type, countGeometryChain(csg->Shapes));
    if (csg->Shapes != nullptr) {
        Logger::info("[DIAG-MONKEY] %s csg firstShapeType=%d\n", prefix, csg->Shapes->Type);
    }
}

void logObjectOnce(const char *prefix, const SimpleBody *object)
{
    static int logged = 0;
    if (!shouldLogMonkeyDiagnostics() || logged++ > 0 || object == nullptr) {
        return;
    }

    const Geometry *shape = object->Shape;
    const Geometry *bounding = object->boundingShapes;
    const Geometry *clipping = object->clippingShapes;
    Logger::info("[DIAG-MONKEY] %s object type=%d shapeCount=%d boundingCount=%d clippingCount=%d\n",
        prefix, object->Type, countGeometryChain(shape), countGeometryChain(bounding),
        countGeometryChain(clipping));
    if (shape != nullptr) {
        Logger::info("[DIAG-MONKEY] %s object firstShapeType=%d\n", prefix, shape->Type);
    }
    if (bounding != nullptr) {
        Logger::info("[DIAG-MONKEY] %s boundingFirstType=%d\n", prefix, bounding->Type);
    }
    if (clipping != nullptr) {
        Logger::info("[DIAG-MONKEY] %s clippingFirstType=%d\n", prefix, clipping->Type);
    }
    auto logCsgChildren = [prefix](const char *tag, const Geometry *maybeCsg) {
        if (maybeCsg == nullptr || maybeCsg->Type != GeometryOperations::CSG_INTERSECTION_TYPE) {
            return;
        }
        const CSG *csg = (const CSG *)maybeCsg;
        int childIndex = 0;
        for (const Geometry *child = csg->Shapes; child != nullptr; child = child->nextObject, ++childIndex) {
            Logger::info("[DIAG-MONKEY] %s %s child[%d] type=%d\n", prefix, tag, childIndex, child->Type);
            if (child->Type == GeometryOperations::PLANE_TYPE) {
                const InfinitePlane *plane = (const InfinitePlane *)child;
                Logger::info("[DIAG-MONKEY] %s %s plane normal=<%.6f,%.6f,%.6f> dist=%.6f\n",
                    prefix, tag, plane->normalVector.x, plane->normalVector.y, plane->normalVector.z,
                    plane->Distance);
            } else if (child->Type == GeometryOperations::CSG_INTERSECTION_TYPE) {
                const CSG *nested = (const CSG *)child;
                int nestedIndex = 0;
                for (const Geometry *nestedChild = nested->Shapes; nestedChild != nullptr;
                    nestedChild = nestedChild->nextObject, ++nestedIndex) {
                    if (nestedChild->Type == GeometryOperations::PLANE_TYPE) {
                        const InfinitePlane *plane = (const InfinitePlane *)nestedChild;
                        Logger::info("[DIAG-MONKEY] %s %s nested child[%d] plane normal=<%.6f,%.6f,%.6f> dist=%.6f\n",
                            prefix, tag, nestedIndex, plane->normalVector.x, plane->normalVector.y,
                            plane->normalVector.z, plane->Distance);
                    }
                }
            } else if (child->Type == GeometryOperations::POLY_TYPE) {
                const PolynomialShape *poly = (const PolynomialShape *)child;
                const int coeffCount = PolynomialShape::termCounts()[poly->Order];
                Logger::info("[DIAG-MONKEY] %s %s poly order=%d coeffCount=%d\n",
                    prefix, tag, poly->Order, coeffCount);
                for (int i = 0; i < coeffCount; ++i) {
                    if (std::fabs(poly->Coeffs[i]) > 1.0e-12) {
                        Logger::info("[DIAG-MONKEY] %s %s poly coeff[%d]=%.6f\n",
                            prefix, tag, i, poly->Coeffs[i]);
                    }
                }
                if (poly->Transform != nullptr) {
                    Logger::info(
                        "[DIAG-MONKEY] %s %s poly transform row0=<%.6f,%.6f,%.6f,%.6f> row1=<%.6f,%.6f,%.6f,%.6f> row2=<%.6f,%.6f,%.6f,%.6f> row3=<%.6f,%.6f,%.6f,%.6f>\n",
                        prefix, tag,
                        poly->Transform->matrix[0][0], poly->Transform->matrix[0][1], poly->Transform->matrix[0][2], poly->Transform->matrix[0][3],
                        poly->Transform->matrix[1][0], poly->Transform->matrix[1][1], poly->Transform->matrix[1][2], poly->Transform->matrix[1][3],
                        poly->Transform->matrix[2][0], poly->Transform->matrix[2][1], poly->Transform->matrix[2][2], poly->Transform->matrix[2][3],
                        poly->Transform->matrix[3][0], poly->Transform->matrix[3][1], poly->Transform->matrix[3][2], poly->Transform->matrix[3][3]);
                }
            }
        }
    };
    logCsgChildren("object csg", shape);
    logCsgChildren("bounding csg", bounding);
}
}


CSG *
ObjectParser::parseCsg(int type)
{
    ParserContext ctx;
    return ObjectParser::parseCsg(type, ctx);
}

Geometry *
ObjectParser::parseShape()
{
    ParserContext ctx;
    return ObjectParser::parseShape(ctx);
}

SimpleBody *
ObjectParser::parseObject()
{
    ParserContext ctx;
    return ObjectParser::parseObject(ctx);
}

SimpleBody *
ObjectParser::parseComposite()
{
    ParserContext ctx;
    return ObjectParser::parseComposite(ctx);
}

CSG *
ObjectParser::parseCsg(int type, ParserContext &ctx)
{
    CSG *container = nullptr;
    Geometry *localShape;
    Vector3Dd localVector;
    CONSTANT constantId;
    int firstShapeParsed = LegacyBoolean::FALSE_VALUE;

    if (type == GeometryOperations::CSG_UNION_TYPE) {
        container = ModelBuilder::getCsgUnion();

    } else if ((type == GeometryOperations::CSG_INTERSECTION_TYPE) ||
               (type == GeometryOperations::CSG_DIFFERENCE_TYPE)) {
        container = ModelBuilder::getCsgIntersection();
    }

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant(ctx)) != -1) {
                    if ((ctx.constants()[(int)constantId].constantType ==
                            ParseGlobals::CSG_INTERSECTION_CONSTANT) ||
                        (ctx.constants()[(int)constantId].constantType ==
                            ParseGlobals::CSG_UNION_CONSTANT) ||
                        (ctx.constants()[(int)constantId].constantType ==
                            ParseGlobals::CSG_DIFFERENCE_CONSTANT)) {
                        delete container;
                        container = (CSG *)GeometryOperations::copy(
                            (SimpleBody *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::Undeclared(ctx);
                }
                break;

            case Tokenizer::LIGHT_SOURCE_TOKEN:
                localShape = LightSourceParser::parseLightSource(ctx);
                if ((type == GeometryOperations::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = LegacyBoolean::TRUE_VALUE;
                SimpleBodyFactory::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case Tokenizer::SPHERE_TOKEN:
                localShape = SphereParser::parseSphere(ctx);
                if ((type == GeometryOperations::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = LegacyBoolean::TRUE_VALUE;
                SimpleBodyFactory::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case Tokenizer::PLANE_TOKEN:
                localShape = PlaneParser::parsePlane(ctx);
                if ((type == GeometryOperations::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = LegacyBoolean::TRUE_VALUE;
                SimpleBodyFactory::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case Tokenizer::TRIANGLE_TOKEN:
                localShape = TriangleParser::parseTriangle(ctx);
                if ((type == GeometryOperations::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = LegacyBoolean::TRUE_VALUE;
                SimpleBodyFactory::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case Tokenizer::SMOOTH_TRIANGLE_TOKEN:
                localShape = SmoothTriangleParser::parseSmoothTriangle(ctx);
                if ((type == GeometryOperations::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = LegacyBoolean::TRUE_VALUE;
                SimpleBodyFactory::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case Tokenizer::QUADRIC_TOKEN:
                localShape = QuadricParser::parseQuadric(ctx);
                if ((type == GeometryOperations::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = LegacyBoolean::TRUE_VALUE;
                SimpleBodyFactory::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case Tokenizer::HEIGHT_FIELD_TOKEN:
                localShape = HeightFieldParser::parseHeightField(ctx);
                if ((type == GeometryOperations::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = LegacyBoolean::TRUE_VALUE;
                SimpleBodyFactory::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case Tokenizer::CUBIC_TOKEN:
                localShape = PolyParser::parsePoly(3, ctx);
                if ((type == GeometryOperations::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = LegacyBoolean::TRUE_VALUE;
                SimpleBodyFactory::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case Tokenizer::QUARTIC_TOKEN:
                localShape = PolyParser::parsePoly(4, ctx);
                if ((type == GeometryOperations::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = LegacyBoolean::TRUE_VALUE;
                SimpleBodyFactory::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case Tokenizer::POLY_TOKEN:
                localShape = PolyParser::parsePoly(0, ctx);
                if ((type == GeometryOperations::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = LegacyBoolean::TRUE_VALUE;
                SimpleBodyFactory::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case Tokenizer::BOX_TOKEN:
                localShape = BoxParser::parseBox(ctx);
                if ((type == GeometryOperations::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = LegacyBoolean::TRUE_VALUE;
                SimpleBodyFactory::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case Tokenizer::BLOB_TOKEN:
                localShape = BlobParser::parseBlob(ctx);
                if ((type == GeometryOperations::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = LegacyBoolean::TRUE_VALUE;
                SimpleBodyFactory::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case Tokenizer::BICUBIC_PATCH_TOKEN:
                localShape = BicubicPatchParser::parseBicubicPatch(ctx);
                if ((type == GeometryOperations::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = LegacyBoolean::TRUE_VALUE;
                SimpleBodyFactory::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case Tokenizer::UNION_TOKEN:
                localShape =
                    (Geometry *)ObjectParser::parseCsg(GeometryOperations::CSG_UNION_TYPE, ctx);
                if ((type == GeometryOperations::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = LegacyBoolean::TRUE_VALUE;
                SimpleBodyFactory::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case Tokenizer::INTERSECTION_TOKEN:
                localShape =
                    (Geometry *)ObjectParser::parseCsg(GeometryOperations::CSG_INTERSECTION_TYPE, ctx);
                if ((type == GeometryOperations::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = LegacyBoolean::TRUE_VALUE;
                SimpleBodyFactory::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case Tokenizer::DIFFERENCE_TOKEN:
                localShape =
                    (Geometry *)ObjectParser::parseCsg(GeometryOperations::CSG_DIFFERENCE_TYPE, ctx);
                if ((type == GeometryOperations::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = LegacyBoolean::TRUE_VALUE;
                SimpleBodyFactory::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            default:
                ctx.tokenStream().ungetToken();
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;
            }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::translate(
                    (SimpleBody *)container, &localVector);
                break;

            case Tokenizer::ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::rotate(
                    (SimpleBody *)container, &localVector);
                break;

            case Tokenizer::SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::scale(
                    (SimpleBody *)container, &localVector);
                break;

            case Tokenizer::INVERSE_TOKEN:
                GeometryOperations::invert((SimpleBody *)container);
                break;

            default:
                if (type == GeometryOperations::CSG_UNION_TYPE) {
                    ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                } else if (type == GeometryOperations::CSG_INTERSECTION_TYPE) {
                    ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                } else {
                    ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                }
                break;
            }
        }
    }

    logCsgOnce("legacy", container);

    return ((CSG *)container);
}

Geometry *
ObjectParser::parseShape(ParserContext &ctx)
{
    Geometry *localShape = nullptr;

    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::LIGHT_SOURCE_TOKEN:
                localShape = LightSourceParser::parseLightSource(ctx);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::SPHERE_TOKEN:
                localShape = SphereParser::parseSphere(ctx);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::PLANE_TOKEN:
                localShape = PlaneParser::parsePlane(ctx);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::TRIANGLE_TOKEN:
                localShape = TriangleParser::parseTriangle(ctx);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::SMOOTH_TRIANGLE_TOKEN:
                localShape = SmoothTriangleParser::parseSmoothTriangle(ctx);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::QUADRIC_TOKEN:
                localShape = QuadricParser::parseQuadric(ctx);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::HEIGHT_FIELD_TOKEN:
                localShape = HeightFieldParser::parseHeightField(ctx);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::CUBIC_TOKEN:
                localShape = PolyParser::parsePoly(3, ctx);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::QUARTIC_TOKEN:
                localShape = PolyParser::parsePoly(4, ctx);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::POLY_TOKEN:
                localShape = PolyParser::parsePoly(0, ctx);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::BOX_TOKEN:
                localShape = BoxParser::parseBox(ctx);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::BLOB_TOKEN:
                localShape = BlobParser::parseBlob(ctx);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::BICUBIC_PATCH_TOKEN:
                localShape = BicubicPatchParser::parseBicubicPatch(ctx);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::UNION_TOKEN:
                localShape =
                    (Geometry *)ObjectParser::parseCsg(GeometryOperations::CSG_UNION_TYPE, ctx);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::INTERSECTION_TOKEN:
                localShape =
                    (Geometry *)ObjectParser::parseCsg(GeometryOperations::CSG_INTERSECTION_TYPE, ctx);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::DIFFERENCE_TOKEN:
                localShape =
                    (Geometry *)ObjectParser::parseCsg(GeometryOperations::CSG_DIFFERENCE_TYPE, ctx);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::QUADRIC_TOKEN, ctx);
                break;
            }
        }
    }
    return (localShape);
}

SimpleBody *
ObjectParser::parseObject(ParserContext &ctx)
{
    SimpleBody *object;
    Geometry *localShape;
    Vector3Dd localVector;
    CONSTANT constantId;
    Texture *localTexture;
    Texture *tempTexture;

    object = nullptr;

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant(ctx)) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::OBJECT_CONSTANT) {
                        object = (SimpleBody *)GeometryOperations::copy(
                            (SimpleBody *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::Undeclared(ctx);
                }
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::SPHERE_TOKEN:
            case Tokenizer::QUADRIC_TOKEN:
            case Tokenizer::QUARTIC_TOKEN:
            case Tokenizer::UNION_TOKEN:
            case Tokenizer::INTERSECTION_TOKEN:
            case Tokenizer::DIFFERENCE_TOKEN:
            case Tokenizer::TRIANGLE_TOKEN:
            case Tokenizer::SMOOTH_TRIANGLE_TOKEN:
            case Tokenizer::PLANE_TOKEN:
            case Tokenizer::CUBIC_TOKEN:
            case Tokenizer::POLY_TOKEN:
            case Tokenizer::BICUBIC_PATCH_TOKEN:
            case Tokenizer::HEIGHT_FIELD_TOKEN:
            case Tokenizer::LIGHT_SOURCE_TOKEN:
            case Tokenizer::BOX_TOKEN:
            case Tokenizer::BLOB_TOKEN:
                ctx.tokenStream().ungetToken();
                if (object == nullptr) {
                    object = SimpleBodyFactory::getObject();
                }

                localShape = ObjectParser::parseShape(ctx);
                SimpleBodyFactory::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(object->Shape));
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::SHAPE_TOKEN, ctx);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;
            }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::BOUNDED_TOKEN:

                ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

                {
                    int Exit_Flag;
                    Exit_Flag = LegacyBoolean::FALSE_VALUE;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {
                        case Tokenizer::RIGHT_CURLY_TOKEN:
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;

                        default:
                            ctx.tokenStream().ungetToken();
                            localShape = ObjectParser::parseShape(ctx);
                            SimpleBodyFactory::link((SimpleBody *)localShape,
                                (SimpleBody **)&(localShape->nextObject),
                                (SimpleBody **)&(object->boundingShapes));
                            break;
                        }
                    }
                }
                break;

            case Tokenizer::CLIPPED_TOKEN:

                ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

                {
                    int Exit_Flag;
                    Exit_Flag = LegacyBoolean::FALSE_VALUE;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {
                        case Tokenizer::RIGHT_CURLY_TOKEN:
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;

                        default:
                            ctx.tokenStream().ungetToken();
                            localShape = ObjectParser::parseShape(ctx);
                            SimpleBodyFactory::link((SimpleBody *)localShape,
                                (SimpleBody **)&(localShape->nextObject),
                                (SimpleBody **)&(object->clippingShapes));
                            break;
                        }
                    }
                }
                break;

            case Tokenizer::COLOUR_TOKEN:
                object->objectColour = ModelBuilder::getColour();
                PrimitiveParser::parseColour(object->objectColour, ctx);
                break;

            case Tokenizer::TEXTURE_TOKEN:
                localTexture = TextureParser::parseTexture(ctx);
                if (localTexture->constantFlag) {
                    localTexture = TextureParser::copyTexture(localTexture);
                }

                if (object->objectTexture == TextureUtils::defaultTexture()) {
                    object->objectTexture = localTexture;
                } else {
                    for (tempTexture = localTexture;
                        tempTexture->Next_Texture != nullptr;
                        tempTexture = tempTexture->Next_Texture) {
                    }

                    tempTexture->Next_Texture = object->objectTexture;
                    object->objectTexture = localTexture;
                }
                break;

            case Tokenizer::NO_SHADOW_TOKEN:
                object->noShadowFlag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::LIGHT_SOURCE_TOKEN:
                ParseErrorReporter::Error(
                    "Light source must be defined using new syntax", ctx);
                break;

            case Tokenizer::TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::translate(object, &localVector);
                break;

            case Tokenizer::ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::rotate(object, &localVector);
                break;

            case Tokenizer::SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::scale(object, &localVector);
                break;

            case Tokenizer::INVERSE_TOKEN:
                GeometryOperations::invert(object);
                break;

            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    logObjectOnce("legacy", object);

    return (object);
}

SimpleBody *
ObjectParser::parseComposite(ParserContext &ctx)
{
    Composite *localComposite;
    SimpleBody *localObject;
    Geometry *localShape;
    CONSTANT constantId;
    Vector3Dd localVector;

    localComposite = nullptr;

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant(ctx)) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::COMPOSITE_CONSTANT) {
                        localComposite = (Composite *)GeometryOperations::copy(
                            (SimpleBody *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::Undeclared(ctx);
                }
                break;

            case Tokenizer::COMPOSITE_TOKEN:
                if (localComposite == nullptr) {
                    localComposite = ModelBuilder::getCompositeObject();
                }

                localObject = ObjectParser::parseComposite(ctx);
                SimpleBodyFactory::link((SimpleBody *)localObject,
                    (SimpleBody **)&(localObject->nextObject),
                    (SimpleBody **)&(localComposite->Objects));
                break;

            case Tokenizer::OBJECT_TOKEN:
                if (localComposite == nullptr) {
                    localComposite = ModelBuilder::getCompositeObject();
                }
                localObject = ObjectParser::parseObject(ctx);
                SimpleBodyFactory::link(localObject, &(localObject->nextObject),
                    &(localComposite->Objects));
                break;

            case Tokenizer::RIGHT_CURLY_TOKEN:
                ctx.tokenStream().ungetToken();
                if (localComposite == nullptr) {
                    localComposite = ModelBuilder::getCompositeObject();
                }
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            default:
                ctx.tokenStream().ungetToken();
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;
            }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::BOUNDED_TOKEN:

                ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

                {
                    int Exit_Flag;
                    Exit_Flag = LegacyBoolean::FALSE_VALUE;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {
                        case Tokenizer::RIGHT_CURLY_TOKEN:
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;

                        default:
                            ctx.tokenStream().ungetToken();
                            localShape = ObjectParser::parseShape(ctx);
                            SimpleBodyFactory::link((SimpleBody *)localShape,
                                (SimpleBody **)&(localShape->nextObject),
                                (SimpleBody **)&(
                                    localComposite->boundingShapes));
                            break;
                        }
                    }
                }
                break;

            case Tokenizer::CLIPPED_TOKEN:

                ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

                {
                    int Exit_Flag;
                    Exit_Flag = LegacyBoolean::FALSE_VALUE;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {
                        case Tokenizer::RIGHT_CURLY_TOKEN:
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;

                        default:
                            ctx.tokenStream().ungetToken();
                            localShape = ObjectParser::parseShape(ctx);
                            SimpleBodyFactory::link((SimpleBody *)localShape,
                                (SimpleBody **)&(localShape->nextObject),
                                (SimpleBody **)&(
                                    localComposite->clippingShapes));
                            break;
                        }
                    }
                }
                break;

            case Tokenizer::TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::translate(
                    (SimpleBody *)localComposite, &localVector);
                break;

            case Tokenizer::ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::rotate(
                    (SimpleBody *)localComposite, &localVector);
                break;

            case Tokenizer::SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::scale(
                    (SimpleBody *)localComposite, &localVector);
                break;

            case Tokenizer::INVERSE_TOKEN:
                GeometryOperations::invert((SimpleBody *)localComposite);
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return ((SimpleBody *)localComposite);
}
