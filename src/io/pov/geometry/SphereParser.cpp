

#include "environment/geometry/volume/Sphere.h"
#include "io/pov/geometry/SceneBuilder.h"

#include "io/pov/geometry/SphereParser.h"
#include "environment/material/povray/PovRayMaterialConstancy.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"
#include "java/util/PriorityQueue.txx"

SimpleBodyBuilder *
SphereParser::parseSphere()
{
    ParserContext ctx;
    return SphereParser::parseSphere(ctx);
}

SimpleBodyBuilder *
SphereParser::parseSphere(ParserContext &ctx)
{
    SimpleBodyBuilder *body = nullptr;
    Sphere *localShape = nullptr;

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::LEFT_ANGLE_TOKEN:
            {
                Vector3Dd localCenter;
                double localRadius;
                ctx.tokenStream().ungetToken();
                PrimitiveParser::parseVector(&localCenter, ctx);
                localRadius = PrimitiveParser::parseFloat(ctx);
                localShape = new Sphere(localRadius);
                body = SceneBuilder::wrap(localShape);
                // The radius is intrinsic to the Sphere geometry now; only
                // the center is accumulated on the owner transform, kept
                // separate from the material/texture transform path.
                body->translateOwnerOnly(&localCenter);
                Exit_Flag = true;
                break;
            }

            case Tokenizer::IDENTIFIER_TOKEN:
            {
                int constantId;
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].getConstantType() ==
                        ParseGlobals::SPHERE_CONSTANT) {
                        body = new SimpleBodyBuilder(
                                *(SimpleBodyBuilder *)ctx.constants()[(int)constantId]
                                    .getConstantData());
                        localShape = (Sphere *)body->getGeometry();
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
        bool Exit_Flag;
        Exit_Flag = false;
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
                body->invert();
                break;

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
