#include "java/util/PriorityQueue.txx"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

#include "environment/geometry/volume/Box.h"
#include "environment/scene/SceneBuilder.h"
#include "environment/geometry/SimpleBody.h"

#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/geometry/BoxParser.h"
#include "environment/material/povray/PovRayMaterialConstancy.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/PrimitiveParser.h"

SimpleBody *
BoxParser::parseBox()
{
    ParserContext ctx;
    return BoxParser::parseBox(ctx);
}

SimpleBody *
BoxParser::parseBox(ParserContext &ctx)
{
    Box *localShape = nullptr;
    SimpleBody *body = nullptr;
    Vector3Dd localVector;
    Vector3Dd minBounds;
    Vector3Dd maxBounds;
    {
        bool Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::LEFT_CURLY_TOKEN:
                Exit_Flag = true;
                break;
            default:
                ParseErrorReporter::parseError(Tokenizer::LEFT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    {
        bool Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::LEFT_ANGLE_TOKEN:
            {
                ctx.tokenStream().ungetToken();
                PrimitiveParser::parseVector(&minBounds, ctx);
                PrimitiveParser::parseVector(&maxBounds, ctx);
                localShape = new Box(minBounds, maxBounds);
                body = SceneBuilder::wrap(localShape);
                Exit_Flag = true;
                break;
            }

            case Tokenizer::IDENTIFIER_TOKEN:
            {
                int constantId;
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].getConstantType() ==
                        ParseGlobals::BOX_CONSTANT) {
                        body = new SimpleBody(
                                *(SimpleBody *)ctx.constants()[(int)constantId]
                                    .getConstantData());
                        localShape = (Box *)body->getGeometry();
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::reportUndeclared(ctx);
                }
                Exit_Flag = true;
                break;
            }

            default:
                ParseErrorReporter::parseError(Tokenizer::LEFT_ANGLE_TOKEN, ctx);
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
                body->translate(&localVector);
                break;
            }

            case Tokenizer::ROTATE_TOKEN:
            {
                Vector3Dd localVector;
                PrimitiveParser::parseVector(&localVector, ctx);
                body->rotate(&localVector);
                break;
            }

            case Tokenizer::SCALE_TOKEN:
            {
                Vector3Dd localVector;
                PrimitiveParser::parseVector(&localVector, ctx);
                body->scale(&localVector);
                break;
            }

            case Tokenizer::INVERSE_TOKEN:
            {
                body->invert();
                break;
            }

            case Tokenizer::TEXTURE_TOKEN:
            {
                PovRayMaterial *localTexture = TextureParser::parseTexture(ctx);
                if (PovRayMaterialConstancy::isConstant(localTexture)) {
                    localTexture = TextureParser::copyTexture(localTexture);
                }
                body->prependMaterialLayers(localTexture);
                break;
            }

            case Tokenizer::COLOUR_TOKEN:
            {
                PrimitiveParser::parseColor(body->ensureShapeColor(), ctx);
                break;
            }

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return body;
}
