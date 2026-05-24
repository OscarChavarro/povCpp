#include "io/pov/ParserContext.h"
#include "io/pov/geometryParser/BlobParser.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/volume/Blob.h"
#include "io/pov/Parse.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/PrimitiveParser.h"
#include "io/pov/SceneConfigParser.h"
#include "io/pov/mediaParser/TextureParser.h"

Geometry *
BlobParser::parseBlob()
{
    ParserContext ctx;
    return BlobParser::parseBlob(ctx);
}

Geometry *
BlobParser::parseBlob(ParserContext &ctx)
{
    (void)ctx;
    Blob *localShape;
    CONSTANT constantId;
    Vector3Dd localVector;
    Texture *localTexture;
    Texture *tempTexture;
    double threshold;
    int npoints;
    BlobList *blobComponents;
    BlobList *blobComponent;

    localShape = nullptr;
    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::LEFT_CURLY_TOKEN:
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;
            default:
                ParseErrorReporter::parseError(Tokenizer::LEFT_CURLY_TOKEN, ctx);
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
            case Tokenizer::THRESHOLD_TOKEN:
            case Tokenizer::COMPONENT_TOKEN:
                ctx.tokenStream().ungetToken();
                localShape = ModelBuilder::getBlobShape();
                blobComponents = nullptr;
                npoints = 0;
                threshold = 1.0;

                /* Here is where we get the blob coefficients */
                {
                    int Exit_Flag;
                    Exit_Flag = LegacyBoolean::FALSE_VALUE;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {
                        case Tokenizer::THRESHOLD_TOKEN:
                            threshold = PrimitiveParser::parseFloat(ctx);
                            break;

                        case Tokenizer::COMPONENT_TOKEN:
                            blobComponent = new BlobList;
                            if (blobComponent == nullptr) {
                                ParseErrorReporter::Error(
                                    "Out of Memory! Cannot allocate blob "
                                    "component", ctx);
                            }
                            blobComponent->elem.coeffs[2] =
                                PrimitiveParser::parseFloat(ctx);
                            blobComponent->elem.radius2 =
                                PrimitiveParser::parseFloat(ctx);
                            PrimitiveParser::parseVector(
                                &blobComponent->elem.pos);
                            blobComponent->next = blobComponents;
                            blobComponents = blobComponent;
                            npoints++;
                            break;

                        default:
                            ctx.tokenStream().ungetToken();
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;
                        }
                    }
                }

                /* Finally, process the information */
                Blob::makeBlob((SimpleBody *)localShape, threshold, blobComponents,
                    npoints, 0);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant(ctx)) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::BLOB_CONSTANT) {
                        localShape = (Blob *)GeometryOperations::copy(
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
                ParseErrorReporter::parseError(Tokenizer::FLOAT_TOKEN, ctx);
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

    return ((Geometry *)localShape);
}
