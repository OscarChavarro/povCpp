#include "java/util/PriorityQueue.txx"

#include "environment/material/MaterialUtils.h"
#include "environment/scene/Scene.h"

#include "io/pov/context/ParserContext.h"
#include "io/pov/material/DefaultTextureParser.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"

void
DefaultTextureParser::parseDefault(Scene *framePtr)
{
    ParserContext ctx;
    DefaultTextureParser::parseDefault(framePtr, ctx);
}

void
DefaultTextureParser::parseDefault(Scene *framePtr, ParserContext &ctx)
{
    (void)framePtr;
    PovrayMaterial *parsedDefaultTexture = MaterialUtils::instance().defaultTexture();
    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);
    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::TEXTURE_TOKEN:
                parsedDefaultTexture = TextureParser::parseTexture(
                    MaterialUtils::instance().defaultTexture(), ctx);
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
    parsedDefaultTexture->setConstant(true);
    MaterialUtils::instance().setDefaultTexture(parsedDefaultTexture);
}
