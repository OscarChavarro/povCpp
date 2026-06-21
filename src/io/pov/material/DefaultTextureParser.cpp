#include "java/util/PriorityQueue.txx"

#include "environment/material/DefaultTextureAliasTracker.h"
#include "environment/material/PovRayMaterialUtils.h"

#include "io/pov/context/ParserContext.h"
#include "io/pov/material/DefaultTextureParser.h"
#include "io/pov/material/PovRayMaterialConstancy.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"

void
DefaultTextureParser::parseDefault()
{
    ParserContext ctx;
    DefaultTextureParser::parseDefault(ctx);
}

void
DefaultTextureParser::parseDefault(ParserContext &ctx)
{
    PovRayMaterial * const oldDefaultTexture = ctx.getDefaultTexture();
    PovRayMaterial *parsedDefaultTexture = oldDefaultTexture;
    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);
    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::TEXTURE_TOKEN:
                parsedDefaultTexture = TextureParser::parseTexture(
                    ctx.getDefaultTexture(), ctx);
                break;
            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = true;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }
    PovRayMaterialConstancy::markConstant(parsedDefaultTexture);
    ctx.setDefaultTexture(parsedDefaultTexture);
    // Only an actual replacement (a texture{} block fired above) retires the
    // old default texture - objects placed earlier in the file may still be
    // aliasing it directly (see DefaultTextureAliasTracker), so it can't just
    // be freed here; the tracker defers until nothing aliases it anymore.
    if (parsedDefaultTexture != oldDefaultTexture) {
        DefaultTextureAliasTracker::retire(oldDefaultTexture);
    }
}
