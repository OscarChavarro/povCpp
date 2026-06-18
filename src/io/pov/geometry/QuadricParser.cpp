#include "java/util/PriorityQueue.txx"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

#include "environment/geometry/volume/Quadric.h"
#include "environment/scene/ModelBuilder.h"
#include "environment/geometry/SimpleBody.h"

#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/geometry/QuadricParser.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"

SimpleBody *
QuadricParser::parseQuadric()
{
    ParserContext ctx;
    return QuadricParser::parseQuadric(ctx);
}

SimpleBody *
QuadricParser::parseQuadric(ParserContext &ctx)
{
    (void)ctx;
    Quadric *localShape;
    SimpleBody *body = nullptr;
    Vector3Dd localVector;
    int constantId;
    PovrayMaterial *localTexture;

    localShape = nullptr;

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::LEFT_ANGLE_TOKEN:
                ctx.tokenStream().ungetToken();
                localShape = ModelBuilder::getQuadricShape();
                body = ModelBuilder::wrap(localShape);
                PrimitiveParser::parseVector(&(localShape->getObject2Terms()), ctx);
                PrimitiveParser::parseVector(
                    &(localShape->getObjectMixedTerms()), ctx);
                PrimitiveParser::parseVector(&(localShape->getObjectTerms()), ctx);
                localShape->setObjectConstant(PrimitiveParser::parseFloat(ctx));
                localShape->setNonZeroSquareTerm(
                    !((localShape->getObject2Terms().x() == 0.0) &&
                        (localShape->getObject2Terms().y() == 0.0) &&
                        (localShape->getObject2Terms().z() == 0.0) &&
                        (localShape->getObjectMixedTerms().x() == 0.0) &&
                        (localShape->getObjectMixedTerms().y() == 0.0) &&
                        (localShape->getObjectMixedTerms().z() == 0.0)));
                Exit_Flag = true;
                break;

            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].getConstantType() ==
                        ParseGlobals::QUADRIC_CONSTANT) {
                        body = (SimpleBody *)((TransformableElement *)ctx.constants()[(int)constantId]
                                .getConstantData())->copy();
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
            switch (ctx.token().tokenId) {
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
                if (localTexture->isConstant()) {
                    localTexture = TextureParser::copyTexture(localTexture);
                }
                TextureParser::prependTextureLayers(localTexture, body->getMaterialRef());
                break;

            case Tokenizer::COLOUR_TOKEN:
                body->setShapeColor(ModelBuilder::getColor());
                PrimitiveParser::parseColor(body->getShapeColor(), ctx);
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return body;
}
