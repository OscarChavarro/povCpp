#include "io/pov/SceneFrameParser.h"
#include "io/pov/DeclarationParser.h"
#include "io/pov/mediaParser/DefaultTextureParser.h"
#include "io/pov/FogParser.h"
#include "io/pov/Parse.h"
#include "io/pov/RenderSettingsParser.h"
#include "io/pov/cameraParser/CameraParser.h"
#include "environment/scene/SceneFrame.h"

extern TokenStruct globalToken;
extern RenderFrame *parsingFramePtr;

void
SceneFrameParser::parseFrame(RenderFrame *framePtr)
{
    SimpleBody *localObject;
    parsingFramePtr = framePtr;

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.tokenId) {
            case FOG_TOKEN:
                FogParser::parseFog(framePtr);
                break;

            case DEFAULT_TOKEN:
                DefaultTextureParser::parseDefault(framePtr);
                break;

            case MAX_TRACE_LEVEL_TOKEN:
                RenderSettingsParser::parseMaxTraceLevel();
                break;

            case OBJECT_TOKEN:
                localObject = ObjectParser::parseObject();
                ObjectUtils::link(localObject, &(localObject->nextObject),
                    &(framePtr->Objects));
                break;

            case COMPOSITE_TOKEN:
                localObject = ObjectParser::parseComposite();
                ObjectUtils::link(localObject, &(localObject->nextObject),
                    &(framePtr->Objects));
                break;

            case VIEW_POINT_TOKEN:
                CameraParser::parseCamera(&(framePtr->viewPoint));
                break;

            case DECLARE_TOKEN:
                DeclarationParser::parseDeclare();
                break;

            case END_OF_FILE_TOKEN:
                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(OBJECT_TOKEN);
                break;
            }
        }
    }
}
