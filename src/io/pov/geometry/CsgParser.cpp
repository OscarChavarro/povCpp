/**
This file parses POV-style constructive solid geometry blocks.

Note on algorithmic scope: by default the parsed CSG tree is evaluated by
collecting surface intersections and filtering them with point-membership
inside tests (ConstructiveSolidGeometryByMorganRules), not by merging full
in/out ray classifications as in [ROTH1982] Scott D. Roth, "Ray Casting for
Modeling Solids", Computer Graphics and Image Processing 18, 109-144 (1982).
When ParserContext::usesCsgRoth() is set (-csgRoth), this builds
ConstructiveSolidGeometryByRaySegment nodes instead, which do implement
[ROTH1982]'s ray-segment classification (see
environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometryByRaySegment.h).
*/
#include "java/util/ArrayList.txx"
#include "io/pov/geometry/CsgParser.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometryByRaySegment.h"
#include "environment/scene/SimpleBody.h"
#include "environment/scene/SceneBuilder.h"
#include "io/pov/geometry/GeometryBuilder.h"
#include "io/pov/context/ParseGlobals.h"
#include "io/pov/geometry/BicubicPatchParser.h"
#include "io/pov/geometry/BlobParser.h"
#include "io/pov/geometry/BoxParser.h"
#include "io/pov/geometry/HeightFieldParser.h"
#include "io/pov/geometry/PlaneParser.h"
#include "io/pov/geometry/PolyParser.h"
#include "io/pov/geometry/QuadricParser.h"
#include "io/pov/geometry/SmoothTriangleParser.h"
#include "io/pov/geometry/SphereParser.h"
#include "io/pov/geometry/TriangleParser.h"
#include "io/pov/light/LightGeometryAdapter.h"
#include "io/pov/light/LightSourceParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"

ConstructiveSolidGeometry *
CsgParser::parse(BooleanSetOperations booleanSetOperation, ParserContext &ctx, bool isNested)
{
    ConstructiveSolidGeometry *container = nullptr;
    SimpleBody *localShape;
    bool firstShapeParsed = false;

    if (ctx.usesCsgRoth()) {
        // The Roth (ray-segment) algorithm needs DIFFERENCE kept as a
        // distinct geometryType (see ConstructiveSolidGeometryByRaySegment),
        // unlike the point-membership path below which folds it into an
        // INTERSECTION container plus per-child invert().
        ConstructiveSolidGeometryByRaySegment *rothContainer =
            new ConstructiveSolidGeometryByRaySegment(booleanSetOperation);
        rothContainer->setTopLevel(!isNested);
        container = rothContainer;

    } else if (booleanSetOperation == BooleanSetOperations::UNION) {
        container = GeometryBuilder::getCsgUnion();

    } else if ((booleanSetOperation == BooleanSetOperations::INTERSECTION) ||
               (booleanSetOperation == BooleanSetOperations::DIFFERENCE)) {
        container = GeometryBuilder::getCsgIntersection();
    }

    if (container == nullptr) {
        return nullptr;
    }

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        bool Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::IDENTIFIER_TOKEN:
            {
                int constantId;
                if ((constantId = ctx.findConstant()) != -1) {
                    if ((ctx.constants()[(int)constantId].getConstantType() ==
                            ParseGlobals::CSG_INTERSECTION_CONSTANT) ||
                        (ctx.constants()[(int)constantId].getConstantType() ==
                            ParseGlobals::CSG_UNION_CONSTANT) ||
                        (ctx.constants()[(int)constantId].getConstantType() ==
                            ParseGlobals::CSG_DIFFERENCE_CONSTANT)) {
                        delete container;
                        // copy() is virtual, so a declared
                        // ConstructiveSolidGeometryByRaySegment constant
                        // (under -csgRoth) keeps its dynamic type instead of
                        // being sliced down to ConstructiveSolidGeometry.
                        container = (ConstructiveSolidGeometry *)((ConstructiveSolidGeometry *)ctx.constants()[(int)constantId]
                            .getConstantData())->copy();
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::reportUndeclared(ctx);
                }
                break;
            }

            case Tokenizer::LIGHT_SOURCE_TOKEN:
                localShape = SceneBuilder::wrap(
                    new LightGeometryAdapter(LightSourceParser::parseLightSource(ctx)));
                if ((booleanSetOperation == BooleanSetOperations::DIFFERENCE) && firstShapeParsed &&
                    !ctx.usesCsgRoth()) {
                    localShape->invert();
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::SPHERE_TOKEN:
                localShape = SphereParser::parseSphere(ctx);
                if ((booleanSetOperation == BooleanSetOperations::DIFFERENCE) && firstShapeParsed &&
                    !ctx.usesCsgRoth()) {
                    localShape->invert();
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::PLANE_TOKEN:
                localShape = PlaneParser::parsePlane(ctx);
                if ((booleanSetOperation == BooleanSetOperations::DIFFERENCE) && firstShapeParsed &&
                    !ctx.usesCsgRoth()) {
                    localShape->invert();
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::TRIANGLE_TOKEN:
                localShape = TriangleParser::parseTriangle(ctx);
                if ((booleanSetOperation == BooleanSetOperations::DIFFERENCE) && firstShapeParsed &&
                    !ctx.usesCsgRoth()) {
                    localShape->invert();
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::SMOOTH_TRIANGLE_TOKEN:
                localShape = SmoothTriangleParser::parseSmoothTriangle(ctx);
                if ((booleanSetOperation == BooleanSetOperations::DIFFERENCE) && firstShapeParsed &&
                    !ctx.usesCsgRoth()) {
                    localShape->invert();
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::QUADRIC_TOKEN:
                localShape = QuadricParser::parseQuadric(ctx);
                if ((booleanSetOperation == BooleanSetOperations::DIFFERENCE) && firstShapeParsed &&
                    !ctx.usesCsgRoth()) {
                    localShape->invert();
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::HEIGHT_FIELD_TOKEN:
                localShape = HeightFieldParser::parseHeightField(ctx);
                if ((booleanSetOperation == BooleanSetOperations::DIFFERENCE) && firstShapeParsed &&
                    !ctx.usesCsgRoth()) {
                    localShape->invert();
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::CUBIC_TOKEN:
                localShape = PolyParser::parsePoly(3, ctx);
                if ((booleanSetOperation == BooleanSetOperations::DIFFERENCE) && firstShapeParsed &&
                    !ctx.usesCsgRoth()) {
                    localShape->invert();
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::QUARTIC_TOKEN:
                localShape = PolyParser::parsePoly(4, ctx);
                if ((booleanSetOperation == BooleanSetOperations::DIFFERENCE) && firstShapeParsed &&
                    !ctx.usesCsgRoth()) {
                    localShape->invert();
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::POLY_TOKEN:
                localShape = PolyParser::parsePoly(0, ctx);
                if ((booleanSetOperation == BooleanSetOperations::DIFFERENCE) && firstShapeParsed &&
                    !ctx.usesCsgRoth()) {
                    localShape->invert();
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::BOX_TOKEN:
                localShape = BoxParser::parseBox(ctx);
                if ((booleanSetOperation == BooleanSetOperations::DIFFERENCE) && firstShapeParsed &&
                    !ctx.usesCsgRoth()) {
                    localShape->invert();
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::BLOB_TOKEN:
                localShape = BlobParser::parseBlob(ctx);
                if ((booleanSetOperation == BooleanSetOperations::DIFFERENCE) && firstShapeParsed &&
                    !ctx.usesCsgRoth()) {
                    localShape->invert();
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::BICUBIC_PATCH_TOKEN:
                localShape = BicubicPatchParser::parseBicubicPatch(ctx);
                if ((booleanSetOperation == BooleanSetOperations::DIFFERENCE) && firstShapeParsed &&
                    !ctx.usesCsgRoth()) {
                    localShape->invert();
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::UNION_TOKEN:
                localShape =
                    SceneBuilder::wrap(CsgParser::parse(BooleanSetOperations::UNION, ctx, true));
                if ((booleanSetOperation == BooleanSetOperations::DIFFERENCE) && firstShapeParsed &&
                    !ctx.usesCsgRoth()) {
                    localShape->invert();
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::INTERSECTION_TOKEN:
                localShape =
                    SceneBuilder::wrap(CsgParser::parse(BooleanSetOperations::INTERSECTION, ctx, true));
                if ((booleanSetOperation == BooleanSetOperations::DIFFERENCE) && firstShapeParsed &&
                    !ctx.usesCsgRoth()) {
                    localShape->invert();
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::DIFFERENCE_TOKEN:
                localShape =
                    SceneBuilder::wrap(CsgParser::parse(BooleanSetOperations::DIFFERENCE, ctx, true));
                if ((booleanSetOperation == BooleanSetOperations::DIFFERENCE) && firstShapeParsed &&
                    !ctx.usesCsgRoth()) {
                    localShape->invert();
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            default:
                ctx.tokenStream().ungetToken();
                Exit_Flag = true;
                break;
            }
        }
    }

    {
        bool Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = true;
                break;

            case Tokenizer::TRANSLATE_TOKEN:
            {
                Vector3Dd localVector;
                PrimitiveParser::parseVector(&localVector, ctx);
                container->translate(&localVector);
                break;
            }

            case Tokenizer::ROTATE_TOKEN:
            {
                Vector3Dd localVector;
                PrimitiveParser::parseVector(&localVector, ctx);
                container->rotate(&localVector);
                break;
            }

            case Tokenizer::SCALE_TOKEN:
            {
                Vector3Dd localVector;
                PrimitiveParser::parseVector(&localVector, ctx);
                container->scale(&localVector);
                break;
            }

            case Tokenizer::INVERSE_TOKEN:
                container->invert();
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return container;
}
