#include "java/util/PriorityQueue.txx"

#include "environment/material/MaterialUtils.h"
#include "environment/scene/SceneFrame.h"

#include "io/pov/context/ParserContext.h"
#include "io/pov/material/DefaultTextureParser.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"

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
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::TEXTURE_TOKEN:
                MaterialUtils::instance().defaultTexture()->constantFlag = false;
                MaterialUtils::instance().setDefaultTexture(TextureParser::parseTexture(ctx));
                MaterialUtils::instance().defaultTexture()->constantFlag = true;
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
}
