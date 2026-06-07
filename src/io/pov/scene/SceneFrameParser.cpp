#include "io/pov/context/ParserContext.h"
#include "io/pov/scene/SceneFrameParser.h"
#include "io/pov/scene/DeclarationParser.h"
#include "io/pov/texture/DefaultTextureParser.h"
#include "io/pov/FogParser.h"
#include "io/pov/ParseErrorReporter.h"
#include "io/pov/geometry/ObjectParser.h"
#include "io/pov/RenderSettingsParser.h"
#include "io/pov/camera/CameraParser.h"
#include "environment/scene/SceneFrame.h"
#include "environment/scene/SimpleBodyFactory.h"


void
SceneFrameParser::parseFrame(RenderFrame *framePtr)
{
    ParserContext ctx;
    SceneFrameParser::parseFrame(framePtr, ctx);
}

void
SceneFrameParser::parseFrame(RenderFrame *framePtr, ParserContext &ctx)
{
    SimpleBody *localObject;
    ctx.parsingFrame() = framePtr;

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::FOG_TOKEN:
                FogParser::parseFog(framePtr, ctx);
                break;

            case Tokenizer::DEFAULT_TOKEN:
                DefaultTextureParser::parseDefault(framePtr, ctx);
                break;

            case Tokenizer::MAX_TRACE_LEVEL_TOKEN:
                RenderSettingsParser::parseMaxTraceLevel(ctx);
                break;

            case Tokenizer::OBJECT_TOKEN:
                localObject = ObjectParser::parseObject(ctx);
                SimpleBodyFactory::link(localObject, &(localObject->nextObject),
                    &(framePtr->Objects));
                break;

            case Tokenizer::COMPOSITE_TOKEN:
                localObject = ObjectParser::parseComposite(ctx);
                SimpleBodyFactory::link(localObject, &(localObject->nextObject),
                    &(framePtr->Objects));
                break;

            case Tokenizer::VIEW_POINT_TOKEN:
                CameraParser::parseCamera(&(framePtr->viewPoint), ctx);
                break;

            case Tokenizer::DECLARE_TOKEN:
                DeclarationParser::parseDeclare(ctx);
                break;

            case Tokenizer::END_OF_FILE_TOKEN:
                Exit_Flag = true;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::OBJECT_TOKEN, ctx);
                break;
            }
        }
    }
}
