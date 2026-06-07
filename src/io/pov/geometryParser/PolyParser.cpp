#include "io/pov/ParserContext.h"
#include "io/pov/geometryParser/PolyParser.h"
#include "common/logger/Logger.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/volume/polynomial/PolynomialShape.h"
#include "environment/scene/SimpleBodyFactory.h"
#include "environment/scene/ModelBuilder.h"
#include "io/pov/ParseErrorReporter.h"
#include "io/pov/ParseGlobals.h"
#include "processing/PolynomialConstants.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/PrimitiveParser.h"
#include "io/pov/SceneConfigParser.h"
#include "io/pov/mediaParser/TextureParser.h"

namespace {
bool shouldLogMonkeyDiagnostics()
{
    const char *flag = std::getenv("POVCPP_DIAG_MONKEY");
    return flag != nullptr && flag[0] != '\0';
}

void logPolyOnce(const char *prefix, const PolynomialShape *shape)
{
    if (!shouldLogMonkeyDiagnostics() || shape == nullptr) {
        return;
    }
    const int coeffCount = PolynomialShape::termCounts()[shape->Order];
    const Texture *texture = shape->Shape_Texture;
    const RGBAColor *colour = shape->Shape_Colour;
    Logger::info("[DIAG-MONKEY] %s quartic order=%d sturm=%d coeffCount=%d\n",
        prefix, shape->Order, shape->sturmFlag, coeffCount);
    Logger::info(
        "[DIAG-MONKEY] %s quartic colour=<%.6f,%.6f,%.6f,%.6f> texNum=%d amb=%.6f diff=%.6f spec=%.6f rough=%.6f phong=%.6f texColour1=<%.6f,%.6f,%.6f,%.6f>\n",
        prefix,
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
    Logger::info(
        "[DIAG-MONKEY] %s quartic texture full number=%d ambient=%.6f diffuse=%.6f brilliance=%.6f refraction=%.6f transmit=%.6f specular=%.6f roughness=%.6f phong=%.6f colour2=<%.6f,%.6f,%.6f,%.6f>\n",
        prefix,
        texture != nullptr ? texture->textureNumber : -1,
        texture != nullptr ? texture->objectAmbient : -1.0,
        texture != nullptr ? texture->objectDiffuse : -1.0,
        texture != nullptr ? texture->objectBrilliance : -1.0,
        texture != nullptr ? texture->objectRefraction : -1.0,
        texture != nullptr ? texture->objectTransmit : -1.0,
        texture != nullptr ? texture->objectSpecular : -1.0,
        texture != nullptr ? texture->objectRoughness : -1.0,
        texture != nullptr ? texture->objectPhong : -1.0,
        texture != nullptr && texture->Colour2 != nullptr ? texture->Colour2->Red : -1.0,
        texture != nullptr && texture->Colour2 != nullptr ? texture->Colour2->Green : -1.0,
        texture != nullptr && texture->Colour2 != nullptr ? texture->Colour2->Blue : -1.0,
        texture != nullptr && texture->Colour2 != nullptr ? texture->Colour2->Alpha : -1.0);
    for (int i = 0; i < coeffCount; ++i) {
        if (std::fabs(shape->Coeffs[i]) > 1.0e-12) {
            Logger::info("[DIAG-MONKEY] %s quartic coeff[%d]=%.6f\n", prefix, i, shape->Coeffs[i]);
        }
    }
}
}

Geometry *
PolyParser::parsePoly(int knownOrder)
{
    ParserContext ctx;
    return PolyParser::parsePoly(knownOrder, ctx);
}

Geometry *
PolyParser::parsePoly(int knownOrder, ParserContext &ctx)
{
    (void)ctx;
    PolynomialShape *localShape;
    Vector3Dd localVector;
    CONSTANT constantId;
    int order;
    Texture *localTexture;

    if (knownOrder > 0) {
        localShape = ModelBuilder::getPolyShape(knownOrder, ctx.termCounts());
    } else {
        localShape = nullptr;
    }

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::DASH_TOKEN:
            case Tokenizer::PLUS_TOKEN:
            case Tokenizer::FLOAT_TOKEN:
                ctx.tokenStream().ungetToken();
                if (localShape != nullptr) {
                    ParseErrorReporter::Error(
                        "The order of a polynomial may not be specified twice", ctx);
                }
                order = (int)PrimitiveParser::parseFloat(ctx);
                if (order < 2 || order > PolynomialConstants::MAX_ORDER) {
                    ParseErrorReporter::Error("Order of Poly is out of range", ctx);
                }
                localShape = ModelBuilder::getPolyShape(order, ctx.termCounts());
                break;

            case Tokenizer::LEFT_ANGLE_TOKEN:
                ctx.tokenStream().ungetToken();
                if (localShape == nullptr) {
                    Logger::info("Need the order of the Poly");
                }
                PrimitiveParser::parseCoeffs(
                    localShape->Order, &(localShape->Coeffs[0]));
                Exit_Flag = true;
                break;

            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant(ctx)) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::POLY_CONSTANT) {
                        localShape =
                            (PolynomialShape *)GeometryOperations::copy(
                                (SimpleBody *)ctx.constants()[(int)constantId]
                                    .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::Undeclared(ctx);
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
            switch (ctx.token().tokenId) {
            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = true;
                break;

            case Tokenizer::STURM_TOKEN:
                localShape->sturmFlag = 1;
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

                SimpleBodyFactory::link((SimpleBody *)localTexture,
                    (SimpleBody **)&localTexture->Next_Texture,
                    (SimpleBody **)&localShape->Shape_Texture);
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

    logPolyOnce("legacy", localShape);
    return ((Geometry *)localShape);
}
