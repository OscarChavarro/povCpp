

#include "java/lang/Math.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "environment/geometry/element/GeometryConfig.h"
#include "environment/geometry/volume/Blob.h"
#include "io/pov/geometry/SceneBuilder.h"

#include "io/pov/geometry/BlobParser.h"
#include "environment/material/povray/PovRayMaterialConstancy.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/PrimitiveParser.h"
#include "java/util/PriorityQueue.txx"
#include "java/util/ArrayList.txx"

SimpleBodyBuilder *
BlobParser::rebuildBodyWithGeometry(SimpleBodyBuilder *body, RayCastingGeometry *geometry)
{
    Material *clonedMaterial = (body->getMaterial() != nullptr) ?
        new PovRayMaterial(*static_cast<PovRayMaterial *>(body->getMaterial())) : nullptr;
    ColorRgba *clonedShapeColor = (body->getShapeColor() != nullptr) ?
        new ColorRgba(*body->getShapeColor()) : nullptr;
    SimpleBodyBuilder *newBody = new SimpleBodyBuilder(geometry, clonedMaterial, clonedShapeColor);
    delete body;
    return newBody;
}

SimpleBodyBuilder *
BlobParser::parseBlob()
{
    ParserContext ctx;
    return BlobParser::parseBlob(ctx);
}

SimpleBodyBuilder *
BlobParser::parseBlob(ParserContext &ctx)
{
    (void)ctx;
    Blob *localShape = nullptr;
    SimpleBodyBuilder *body = nullptr;
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
                java::ArrayList<BlobComponent> blobComponents;
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
                            const double strength = PrimitiveParser::parseFloat(ctx);
                            const double radius = PrimitiveParser::parseFloat(ctx);
                            Vector3Dd center;
                            PrimitiveParser::parseVector(&center, ctx);
                            blobComponents.add(BlobComponent(center, radius, strength));
                            break;
                        }

                        default:
                            ctx.tokenStream().ungetToken();
                            Exit_Flag = true;
                            break;
                        }
                    }
                }

                if (blobComponents.isEmpty()) {
                    Logger::reportMessage("Blob", Logger::FATAL_ERROR, "", "Need at least one component in a blob\n");
                }
                BlobSolid solid(threshold, false);
                for (long int i = 0; i < blobComponents.size(); i++) {
                    if (java::Math::abs(blobComponents[i].getStrength()) < GeometryConfig::INTERSECTION_EPSILON ||
                        blobComponents[i].getRadius() < GeometryConfig::INTERSECTION_EPSILON) {
                        Logger::reportMessage("Blob", Logger::ERROR, "", "Degenerate blob element\n");
                    }
                    solid.addComponent(blobComponents[i]);
                }
                localShape = new Blob(solid);
                body = SceneBuilder::wrap(localShape);
                Exit_Flag = true;
                break;
            }

            case Tokenizer::IDENTIFIER_TOKEN:
                int constantId;
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].getConstantType() ==
                        ParseGlobals::BLOB_CONSTANT) {
                        body = new SimpleBodyBuilder(
                                *(SimpleBodyBuilder *)ctx.constants()[(int)constantId]
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
