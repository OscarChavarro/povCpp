#include "java/util/PriorityQueue.txx"
#include "java/util/ArrayList.txx"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

#include "environment/geometry/volume/Blob.h"
#include "environment/scene/SceneBuilder.h"
#include "environment/geometry/SimpleBody.h"

#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/geometry/BlobParser.h"
#include "io/pov/material/PovRayMaterialConstancy.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"

namespace {

SimpleBody *
rebuildBodyWithGeometry(SimpleBody *body, Geometry *geometry)
{
    Material *clonedMaterial = (body->getMaterial() != nullptr) ?
        body->getMaterial()->copy() : nullptr;
    ColorRgba *clonedShapeColor = (body->getShapeColor() != nullptr) ?
        new ColorRgba(*body->getShapeColor()) : nullptr;
    SimpleBody *newBody = new SimpleBody(geometry, clonedMaterial, clonedShapeColor);
    newBody->getTransform() = body->getTransform();
    newBody->getTransformInverse() = body->getTransformInverse();
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
    Blob *localShape = nullptr;
    SimpleBody *body = nullptr;
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
            case Tokenizer::THRESHOLD_TOKEN:
            case Tokenizer::COMPONENT_TOKEN:
            {
                double threshold = 1.0;
                int npoints = 0;
                java::ArrayList<BlobElement *> blobComponents;
                ctx.tokenStream().ungetToken();

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
                        {
                            BlobElement *blobComponent = new BlobElement;
                            if (blobComponent == nullptr) {
                                ParseErrorReporter::reportError(
                                    "Out of Memory! Cannot allocate blob "
                                    "component", ctx);
                            }
                            blobComponent->getCoeffs()[2] =
                                PrimitiveParser::parseFloat(ctx);
                            blobComponent->setRadius2(
                                PrimitiveParser::parseFloat(ctx));
                            PrimitiveParser::parseVector(
                                &blobComponent->getPos(), ctx);
                            blobComponents.add(blobComponent);
                            npoints++;
                            break;
                        }

                        default:
                            ctx.tokenStream().ungetToken();
                            Exit_Flag = true;
                            break;
                        }
                    }
                }

                localShape = new Blob(threshold, &blobComponents, npoints, 0);
                body = SceneBuilder::wrap(localShape);
                Exit_Flag = true;
                break;
            }

            case Tokenizer::IDENTIFIER_TOKEN:
                int constantId;
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].getConstantType() ==
                        ParseGlobals::BLOB_CONSTANT) {
                        body = new SimpleBody(
                                *(SimpleBody *)ctx.constants()[(int)constantId]
                                    .getConstantData());
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
        bool Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = true;
                break;

            case Tokenizer::STURM_TOKEN:
            {
                if (localShape->getSturmFlag() == 0) {
                    body = rebuildBodyWithGeometry(
                        body, localShape->copyWithSturmFlag(1));
                    localShape = (Blob *)body->getGeometry();
                }
                break;
            }

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
