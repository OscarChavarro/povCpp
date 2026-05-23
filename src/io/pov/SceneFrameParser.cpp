#include "io/pov/SceneFrameParser.h"
#include "app/PovApp.h"
#include "io/pov/DeclarationParser.h"
#include "io/pov/DefaultTextureParser.h"
#include "io/pov/FogParser.h"
#include "io/pov/Parse.h"
#include "io/pov/RenderSettingsParser.h"
#include "io/pov/CameraParser.h"
#include "render/RenderFrame.h"

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
            switch (globalToken.Token_Id) {
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
                ObjectUtils::link(localObject, &(localObject->Next_Object),
                    &(framePtr->Objects));
                break;

            case COMPOSITE_TOKEN:
                localObject = ObjectParser::parseComposite();
                ObjectUtils::link(localObject, &(localObject->Next_Object),
                    &(framePtr->Objects));
                break;

            case VIEW_POINT_TOKEN:
                CameraParser::parseCamera(&(framePtr->View_Point));
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
