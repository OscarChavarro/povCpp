#include "io/pov/ParserContext.h"
#include "io/pov/SceneFrameParser.h"
#include "io/pov/DeclarationParser.h"
#include "io/pov/mediaParser/DefaultTextureParser.h"
#include "io/pov/FogParser.h"
#include "io/pov/Parse.h"
#include "io/pov/RenderSettingsParser.h"
#include "io/pov/cameraParser/CameraParser.h"
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
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case FOG_TOKEN:
                FogParser::parseFog(framePtr, ctx);
                break;

            case DEFAULT_TOKEN:
                DefaultTextureParser::parseDefault(framePtr, ctx);
                break;

            case MAX_TRACE_LEVEL_TOKEN:
                RenderSettingsParser::parseMaxTraceLevel(ctx);
                break;

            case OBJECT_TOKEN:
                localObject = ObjectParser::parseObject(ctx);
                SimpleBodyFactory::link(localObject, &(localObject->nextObject),
                    &(framePtr->Objects));
                break;

            case COMPOSITE_TOKEN:
                localObject = ObjectParser::parseComposite(ctx);
                SimpleBodyFactory::link(localObject, &(localObject->nextObject),
                    &(framePtr->Objects));
                break;

            case VIEW_POINT_TOKEN:
                CameraParser::parseCamera(&(framePtr->viewPoint), ctx);
                break;

            case DECLARE_TOKEN:
                DeclarationParser::parseDeclare(ctx);
                break;

            case END_OF_FILE_TOKEN:
                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(OBJECT_TOKEN, ctx);
                break;
            }
        }
    }
}
