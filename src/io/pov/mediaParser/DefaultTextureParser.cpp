#include "io/pov/mediaParser/DefaultTextureParser.h"
#include "io/pov/Parse.h"
#include "render/RenderFrame.h"

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
            switch (globalToken.Token_Id) {
            case TEXTURE_TOKEN:
                Default_Texture->Constant_Flag = FALSE;
                Default_Texture = TextureParser::parseTexture();
                Default_Texture->Constant_Flag = TRUE;
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
