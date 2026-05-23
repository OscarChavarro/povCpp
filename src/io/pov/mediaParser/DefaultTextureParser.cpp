#include "io/pov/ParserContext.h"
#include "io/pov/mediaParser/DefaultTextureParser.h"
#include "io/pov/Parse.h"
#include "environment/scene/SceneFrame.h"

extern Texture *Default_Texture;

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
    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);
    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (ctx.token().tokenId) {
            case TEXTURE_TOKEN:
                Default_Texture->constantFlag = FALSE;
                Default_Texture = TextureParser::parseTexture(ctx);
                Default_Texture->constantFlag = TRUE;
                break;
            case RIGHT_CURLY_TOKEN:
                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }
}
