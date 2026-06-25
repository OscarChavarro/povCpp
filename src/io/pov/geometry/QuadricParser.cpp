#include "java/util/PriorityQueue.txx"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

#include "environment/geometry/volume/Quadric.h"
#include "environment/scene/SceneBuilder.h"
#include "environment/scene/SimpleBody.h"

#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/geometry/QuadricParser.h"
#include "environment/material/povray/PovRayMaterialConstancy.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"

SimpleBody *
QuadricParser::parseQuadric(ParserContext &ctx)
{
    Quadric *localShape;
    SimpleBody *body = nullptr;
    Vector3Dd localVector;
    Vector3Dd object2Terms;
    Vector3Dd objectMixedTerms;
    Vector3Dd objectTerms;
    double objectConstant;
    int constantId;
    PovRayMaterial *localTexture;

    localShape = nullptr;

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::LEFT_ANGLE_TOKEN:
                ctx.tokenStream().ungetToken();
                PrimitiveParser::parseVector(&object2Terms, ctx);
                PrimitiveParser::parseVector(&objectMixedTerms, ctx);
                PrimitiveParser::parseVector(&objectTerms, ctx);
                objectConstant = PrimitiveParser::parseFloat(ctx);
                localShape = new Quadric(
                    object2Terms, objectMixedTerms, objectTerms, objectConstant);
                body = SceneBuilder::wrap(localShape);
                Exit_Flag = true;
                break;

            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].getConstantType() ==
                        ParseGlobals::QUADRIC_CONSTANT) {
                        body = new SimpleBody(
                                *(SimpleBody *)ctx.constants()[(int)constantId]
                                    .getConstantData());
                        localShape = (Quadric *)body->getGeometry();
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::reportUndeclared(ctx);
                }
                Exit_Flag = true;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::LEFT_ANGLE_TOKEN, ctx);
                break;
            }
        }
    }

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = true;
                break;

            case Tokenizer::TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                body->translate(&localVector);
                break;

            case Tokenizer::ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                body->rotate(&localVector);
                break;

            case Tokenizer::SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                body->scale(&localVector);
                break;

            case Tokenizer::INVERSE_TOKEN:
                body->invert();
                break;

            case Tokenizer::TEXTURE_TOKEN:
                localTexture = TextureParser::parseTexture(ctx);
                if (PovRayMaterialConstancy::isConstant(localTexture)) {
                    localTexture = TextureParser::copyTexture(localTexture);
                }
                body->prependMaterialLayers(localTexture);
                break;

            case Tokenizer::COLOUR_TOKEN:
                PrimitiveParser::parseColor(body->ensureShapeColor(), ctx);
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return body;
}
