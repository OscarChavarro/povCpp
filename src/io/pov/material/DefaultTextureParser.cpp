#include "environment/material/povray/DefaultTextureAliasTracker.h"
#include "io/pov/material/DefaultTextureParser.h"
#include "environment/material/povray/PovRayMaterialConstancy.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"

void
DefaultTextureParser::parseDefault(ParserContext &ctx)
{
    PovRayMaterial * const oldDefaultTexture =
        static_cast<PovRayMaterial *>(ctx.getDefaultTexture());
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
                    static_cast<PovRayMaterial *>(ctx.getDefaultTexture()), ctx);
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
    if (parsedDefaultTexture != oldDefaultTexture) {
        DefaultTextureAliasTracker::retire(oldDefaultTexture);
    }
}
