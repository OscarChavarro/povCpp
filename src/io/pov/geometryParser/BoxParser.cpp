#include "io/pov/ParserContext.h"
#include "io/pov/geometryParser/BoxParser.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/volume/Box.h"
#include "io/pov/Parse.h"
#include "io/pov/PrimitiveParser.h"
#include "io/pov/SceneConfigParser.h"
#include "io/pov/mediaParser/TextureParser.h"


Geometry *
BoxParser::parseBox()
{
    ParserContext ctx;
    return BoxParser::parseBox(ctx);
}

Geometry *
BoxParser::parseBox(ParserContext &ctx)
{
    (void)ctx;
    Box *localShape;
    CONSTANT constantId;
    Vector3Dd localVector;
    Texture *localTexture;
    Texture *tempTexture;

    localShape = nullptr;
    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (ctx.token().tokenId) {
            case LEFT_CURLY_TOKEN:
                Exit_Flag = TRUE;
                break;
            default:
                ParseErrorReporter::parseError(LEFT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (ctx.token().tokenId) {
            case LEFT_ANGLE_TOKEN:
                Tokenizer::ungetToken();
                localShape = ModelBuilder::getBoxShape();
                PrimitiveParser::parseVector(&(localShape->bounds[0]), ctx);
                PrimitiveParser::parseVector(&(localShape->bounds[1]), ctx);
                Exit_Flag = TRUE;
                break;

            case IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant(ctx)) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        BOX_CONSTANT) {
                        localShape = (Box *)GeometryOperations::copy(
                            (SimpleBody *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::Undeclared(ctx);
                }
                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(LEFT_ANGLE_TOKEN, ctx);
                break;
            }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (ctx.token().tokenId) {
            case RIGHT_CURLY_TOKEN:
                Exit_Flag = TRUE;
                break;

            case TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::translate(
                    (SimpleBody *)localShape, &localVector);
                break;

            case ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::rotate(
                    (SimpleBody *)localShape, &localVector);
                break;

            case SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::scale(
                    (SimpleBody *)localShape, &localVector);
                break;

            case INVERSE_TOKEN:
                GeometryOperations::invert((SimpleBody *)localShape);
                break;

            case TEXTURE_TOKEN:
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

            case COLOUR_TOKEN:
                localShape->Shape_Colour = ModelBuilder::getColour();
                PrimitiveParser::parseColour(localShape->Shape_Colour, ctx);
                break;

            default:
                ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return ((Geometry *)localShape);
}
