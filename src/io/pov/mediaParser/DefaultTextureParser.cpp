#include "io/pov/ParserContext.h"
#include "io/pov/mediaParser/DefaultTextureParser.h"
#include "io/pov/ParseErrorReporter.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/mediaParser/TextureParser.h"
#include "environment/scene/SceneFrame.h"

void
DefaultTextureParser::parseDefault(RenderFrame *framePtr)
{
    ParserContext ctx;
    DefaultTextureParser::parseDefault(framePtr, ctx);
}

void
DefaultTextureParser::parseDefault(RenderFrame *framePtr, ParserContext &ctx)
{
    (void)framePtr;
    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);
    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::TEXTURE_TOKEN:
                TextureUtils::defaultTexture()->constantFlag = LegacyBoolean::FALSE_VALUE;
                TextureUtils::defaultTexture() = TextureParser::parseTexture(ctx);
                TextureUtils::defaultTexture()->constantFlag = LegacyBoolean::TRUE_VALUE;
                break;
            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }
}
