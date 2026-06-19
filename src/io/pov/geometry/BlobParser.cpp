#include "java/util/PriorityQueue.txx"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

#include "environment/geometry/volume/Blob.h"
#include "environment/scene/ModelBuilder.h"
#include "environment/geometry/SimpleBody.h"

#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/geometry/BlobParser.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"

namespace {

SimpleBody *
rebuildBodyWithGeometry(SimpleBody *body, Geometry *geometry)
{
    SimpleBody *newBody = new SimpleBody(
        geometry, body->getMaterial(), body->getShapeColor());
    newBody->getTransform() = body->getTransform();
    newBody->getTransformInverse() = body->getTransformInverse();
    delete body->getGeometry();
    delete body;
    return newBody;
}

}

SimpleBody *
BlobParser::parseBlob()
{
    ParserContext ctx;
    return BlobParser::parseBlob(ctx);
}

SimpleBody *
BlobParser::parseBlob(ParserContext &ctx)
{
    (void)ctx;
    Blob *localShape;
    SimpleBody *body = nullptr;
    int constantId;
    Vector3Dd localVector;
    PovrayMaterial *localTexture;
    double threshold;
    int npoints;
    BlobList *blobComponents;
    BlobList *blobComponent;

    localShape = nullptr;
    {
        bool Exit_Flag;
        Exit_Flag = false;
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
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::THRESHOLD_TOKEN:
            case Tokenizer::COMPONENT_TOKEN:
                ctx.tokenStream().ungetToken();
                blobComponents = nullptr;
                npoints = 0;
                threshold = 1.0;

                // Here is where we get the blob coefficients
                {
                    bool Exit_Flag;
                    Exit_Flag = false;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().getTokenId()) {
                        case Tokenizer::THRESHOLD_TOKEN:
                            threshold = PrimitiveParser::parseFloat(ctx);
                            break;

                        case Tokenizer::COMPONENT_TOKEN:
                            blobComponent = new BlobList;
                            if (blobComponent == nullptr) {
                                ParseErrorReporter::reportError(
                                    "Out of Memory! Cannot allocate blob "
                                    "component", ctx);
                            }
                            blobComponent->getElem().getCoeffs()[2] =
                                PrimitiveParser::parseFloat(ctx);
                            blobComponent->getElem().setRadius2(
                                PrimitiveParser::parseFloat(ctx));
                            PrimitiveParser::parseVector(
                                &blobComponent->getElem().getPos(), ctx);
                            blobComponent->setNext(blobComponents);
                            blobComponents = blobComponent;
                            npoints++;
                            break;

                        default:
                            ctx.tokenStream().ungetToken();
                            Exit_Flag = true;
                            break;
                        }
                    }
                }

                localShape = new Blob(threshold, blobComponents, npoints, 0);
                body = ModelBuilder::wrap(localShape);
                Exit_Flag = true;
                break;

            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].getConstantType() ==
                        ParseGlobals::BLOB_CONSTANT) {
                        body = (SimpleBody *)((TransformableElement *)ctx.constants()[(int)constantId]
                                .getConstantData())->copy();
                        localShape = (Blob *)body->getGeometry();
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::reportUndeclared(ctx);
                }
                Exit_Flag = true;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::FLOAT_TOKEN, ctx);
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

            case Tokenizer::STURM_TOKEN:
                if (localShape->getSturmFlag() == 0) {
                    body = rebuildBodyWithGeometry(
                        body, localShape->copyWithSturmFlag(1));
                    localShape = (Blob *)body->getGeometry();
                }
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
                TextureParser::prependTextureLayers(localTexture, body);
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
