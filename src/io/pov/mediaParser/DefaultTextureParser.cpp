#include "io/pov/mediaParser/DefaultTextureParser.h"
#include "io/pov/Parse.h"
#include "environment/scene/SceneFrame.h"

extern TokenStruct globalToken;
extern Texture *Default_Texture;

void
DefaultTextureParser::parseDefault(RenderFrame *framePtr)
{
    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);
    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.tokenId) {
            case TEXTURE_TOKEN:
                Default_Texture->constantFlag = FALSE;
                Default_Texture = TextureParser::parseTexture();
                Default_Texture->constantFlag = TRUE;
                break;
            case RIGHT_CURLY_TOKEN:
                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN);
                break;
            }
        }
    }
}
