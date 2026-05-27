#include "io/pov/ParserContext.h"
#include "io/pov/geometryParser/SphereParser.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/volume/Sphere.h"
#include "io/pov/Parse.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/PrimitiveParser.h"
#include "io/pov/SceneConfigParser.h"
#include "io/pov/mediaParser/TextureParser.h"
#include "common/logger/Logger.h"
#include <cstdlib>

namespace {
bool shouldLogMonkeyDiagnostics()
{
    const char *flag = std::getenv("POVCPP_DIAG_MONKEY");
    return flag != nullptr && flag[0] != '\0';
}

void logSphereOnce(const char *prefix, const Sphere *sphere)
{
    if (!shouldLogMonkeyDiagnostics() || sphere == nullptr) {
        return;
    }
    const RGBAColor *colour = sphere->Shape_Colour;
    const Texture *texture = sphere->Shape_Texture;
    Logger::info(
        "[DIAG-MONKEY] %s sphere center=<%.6f,%.6f,%.6f> radius=%.6f colour=<%.6f,%.6f,%.6f,%.6f> texNum=%d amb=%.6f diff=%.6f spec=%.6f rough=%.6f phong=%.6f texColour1=<%.6f,%.6f,%.6f,%.6f>\n",
        prefix, sphere->Center.x, sphere->Center.y, sphere->Center.z, sphere->Radius,
        colour != nullptr ? colour->Red : -1.0,
        colour != nullptr ? colour->Green : -1.0,
        colour != nullptr ? colour->Blue : -1.0,
        colour != nullptr ? colour->Alpha : -1.0,
        texture != nullptr ? texture->textureNumber : -1,
        texture != nullptr ? texture->objectAmbient : -1.0,
        texture != nullptr ? texture->objectDiffuse : -1.0,
        texture != nullptr ? texture->objectSpecular : -1.0,
        texture != nullptr ? texture->objectRoughness : -1.0,
        texture != nullptr ? texture->objectPhong : -1.0,
        texture != nullptr && texture->Colour1 != nullptr ? texture->Colour1->Red : -1.0,
        texture != nullptr && texture->Colour1 != nullptr ? texture->Colour1->Green : -1.0,
        texture != nullptr && texture->Colour1 != nullptr ? texture->Colour1->Blue : -1.0,
        texture != nullptr && texture->Colour1 != nullptr ? texture->Colour1->Alpha : -1.0);
}
}


Geometry *
SphereParser::parseSphere()
{
    ParserContext ctx;
    return SphereParser::parseSphere(ctx);
}

Geometry *
SphereParser::parseSphere(ParserContext &ctx)
{
    (void)ctx;
    Sphere *localShape;
    CONSTANT constantId;
    Vector3Dd localVector;
    Texture *localTexture;
    Texture *tempTexture;

    localShape = nullptr;

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::LEFT_ANGLE_TOKEN:
                ctx.tokenStream().ungetToken();
                localShape = ModelBuilder::getSphereShape();
                PrimitiveParser::parseVector(&(localShape->Center), ctx);
                localShape->Radius = PrimitiveParser::parseFloat(ctx);
                localShape->radiusSquared =
                    localShape->Radius * localShape->Radius;
                localShape->inverseRadius = 1.0 / localShape->Radius;
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant(ctx)) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::SPHERE_CONSTANT) {
                        localShape = (Sphere *)GeometryOperations::copy(
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

            default:
                ParseErrorReporter::parseError(Tokenizer::LEFT_ANGLE_TOKEN, ctx);
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
                    (SimpleBody *)localShape, &localVector);
                break;

            case Tokenizer::ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::rotate(
                    (SimpleBody *)localShape, &localVector);
                break;

            case Tokenizer::SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::scale(
                    (SimpleBody *)localShape, &localVector);
                break;

            case Tokenizer::INVERSE_TOKEN:
                GeometryOperations::invert((SimpleBody *)localShape);
                break;

            case Tokenizer::TEXTURE_TOKEN:
                localTexture = TextureParser::parseTexture(ctx);
                if (localTexture->constantFlag) {
                    localTexture = TextureParser::copyTexture(localTexture);
                }

                {
                    for (tempTexture = localTexture;
                        tempTexture->Next_Texture != nullptr;
                        tempTexture = tempTexture->Next_Texture) {
                    }

                    tempTexture->Next_Texture = localShape->Shape_Texture;
                    localShape->Shape_Texture = localTexture;
                }
                break;

            case Tokenizer::COLOUR_TOKEN:
                localShape->Shape_Colour = ModelBuilder::getColour();
                PrimitiveParser::parseColour(localShape->Shape_Colour, ctx);
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    logSphereOnce("legacy", localShape);
    return ((Geometry *)localShape);
}
