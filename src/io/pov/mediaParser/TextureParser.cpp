#include "app/PovApp.h"
#include "common/FrameConfig.h"
#include "common/linealAlgebra/Transformation.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "io/image/DumpFormat.h"
#include "io/image/GifFormat.h"
#include "io/image/IffFormat.h"
#include "io/image/TargaFormat.h"
#include "io/pov/Parse.h"
#include "render/RenderEngine.h"

#include "environment/camera/Camera.h"
#include "environment/geometry/elements/Triangle.h"
#include "environment/geometry/surface/InfinitePlane.h"
#include "environment/geometry/surface/parametric/ParametricPatch.h"
#include "environment/geometry/volume/Blob.h"
#include "environment/geometry/volume/Box.h"
#include "environment/geometry/volume/HeightField.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/geometry/volume/Sphere.h"
#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/geometry/volume/polynomial/PolynomialShape.h"
#include "environment/light/Light.h"

extern ReservedWord globalReservedWords[];
extern double antialiasThreshold;
extern int termCounts[MAX_ORDER + 1];
extern TokenStruct globalToken;
extern double maxTraceLevel;
extern char verboseFormat;
extern unsigned int Options;
extern char statFileName[FILE_NAME_LENGTH];

extern RenderFrame *parsingFramePtr;
extern Constant constants[MAX_CONSTANTS];
extern int numberOfConstants;
extern int degenerateTriangles;
extern RGBAColorPaletteSpan *constructionMap;

Texture *
TextureParser::copyTexture(Texture *texture)
{
    return TextureUtils::copyTexture(texture);
}

Texture *
TextureParser::parseTexture()
{
    Vector3Dd localVector;
    CONSTANT constantId;
    Texture *texture;
    Texture *localTexture;
    Texture *firstTexture;
    Texture *tempTexture;
    int reg;

    texture = Default_Texture;

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) {
            case IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant()) != -1) {
                    if (constants[(int)constantId].Constant_Type ==
                        TEXTURE_CONSTANT) {
                        texture = ((Texture *)constants[(int)constantId]
                                .Constant_Data);
                    } else {
                        ParseErrorReporter::typeError();
                    }
                } else {
                    ParseErrorReporter::Undeclared();
                }
                break;

            case FLOAT_TOKEN:
                Tokenizer::ungetToken();
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Texture_Randomness = PrimitiveParser::parseFloat();
                break;

            case ONCE_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Once_Flag = TRUE;
                break;

            case TURBULENCE_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Turbulence = PrimitiveParser::parseFloat();
                break;

            case OCTAVES_TOKEN: /* dmf 02/05 for turb */
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Octaves = (int)PrimitiveParser::parseFloat();
                if (texture->Octaves < 1) {
                    texture->Octaves = 6;
                }
                if (texture->Octaves > 10) { /* Avoid DOMAIN errors */
                    texture->Octaves = 10;
                }
                break;

            case BOZO_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Texture_Number = BOZO_TEXTURE;
                break;

            case MORTAR_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Mortar = PrimitiveParser::parseFloat();
                if (texture->Mortar < 0) {
                    texture->Mortar = 0.2;
                }
                break;

            case BRICK_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Texture_Number = BRICK_TEXTURE;
                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (globalToken.Token_Id) {
                        case COLOUR_TOKEN:
                            texture->Colour1 = SceneFactory::getColour();
                            texture->Colour2 = SceneFactory::getColour();
                            PrimitiveParser::parseColour(texture->Colour1);
                            ParseHelpers::getExpectedToken(COLOUR_TOKEN);
                            PrimitiveParser::parseColour(texture->Colour2);
                            break;

                        default:
                            Tokenizer::ungetToken();
                            Exit_Flag = TRUE;
                            break;
                        }
                    }
                }
                break;

            case CHECKER_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Texture_Number = CHECKER_TEXTURE;
                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (globalToken.Token_Id) {
                        case COLOUR_TOKEN:
                            texture->Colour1 = SceneFactory::getColour();
                            texture->Colour2 = SceneFactory::getColour();
                            PrimitiveParser::parseColour(texture->Colour1);
                            ParseHelpers::getExpectedToken(COLOUR_TOKEN);
                            PrimitiveParser::parseColour(texture->Colour2);
                            break;

                        default:
                            Tokenizer::ungetToken();
                            Exit_Flag = TRUE;
                            break;
                        }
                    }
                }
                break;

            case CHECKER_TEXTURE_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Texture_Number = CHECKER_TEXTURE_TEXTURE;

                ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);

                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (globalToken.Token_Id) {
                        case TEXTURE_TOKEN:
                            localTexture = TextureParser::parseTexture();
                            if (localTexture->Constant_Flag) {
                                localTexture =
                                    TextureParser::copyTexture(localTexture);
                            }
                            {
                                for (tempTexture = localTexture;
                                    tempTexture->Next_Texture != nullptr;
                                    tempTexture = tempTexture->Next_Texture) {
                                }

                                tempTexture->Next_Texture =
                                    (Texture *)texture->Colour1;
                                texture->Colour1 = (RGBAColor *)localTexture;
                            }
                            break;
                        default:
                            Tokenizer::ungetToken();
                            Exit_Flag = TRUE;
                            break;
                        }
                    }
                }

                ParseHelpers::getExpectedToken(TILE2_TOKEN);
                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (globalToken.Token_Id) {
                        case TEXTURE_TOKEN:
                            localTexture = TextureParser::parseTexture();
                            if (localTexture->Constant_Flag) {
                                localTexture =
                                    TextureParser::copyTexture(localTexture);
                            }

                            {
                                for (tempTexture = localTexture;
                                    tempTexture->Next_Texture != nullptr;
                                    tempTexture = tempTexture->Next_Texture) {
                                }

                                tempTexture->Next_Texture =
                                    (Texture *)texture->Colour2;
                                texture->Colour2 = (RGBAColor *)localTexture;
                            }
                            break;
                        default:
                            Tokenizer::ungetToken();
                            Exit_Flag = TRUE;
                            break;
                        }
                    }
                }
                ParseHelpers::getExpectedToken(RIGHT_CURLY_TOKEN);
                break;

            case MARBLE_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Texture_Number = MARBLE_TEXTURE;
                break;

            case WOOD_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Texture_Number = WOOD_TEXTURE;
                break;

            case SPOTTED_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Texture_Number = SPOTTED_TEXTURE;
                break;

            case AGATE_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Texture_Number = AGATE_TEXTURE;
                break;

            case GRANITE_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Texture_Number = GRANITE_TEXTURE;
                break;

            case GRADIENT_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Texture_Number = GRADIENT_TEXTURE;
                PrimitiveParser::parseVector(&(texture->Texture_Gradient));
                break;

            case AMBIENT_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                (texture->Object_Ambient) = PrimitiveParser::parseFloat();
                break;

            case BRILLIANCE_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                (texture->Object_Brilliance) = PrimitiveParser::parseFloat();
                break;

            case ROUGHNESS_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                (texture->Object_Roughness) = PrimitiveParser::parseFloat();
                /* No training wheels */
                /* if (texture -> Object_Roughness > 1.0)
                    texture -> Object_Roughness = 1.0;
                if (texture -> Object_Roughness < 0.001)
                    texture -> Object_Roughness = 0.001; */
                break;

            case PHONGSIZE_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                (texture->Object_PhongSize) = PrimitiveParser::parseFloat();
                /* No training wheels */
                /*if (texture -> Object_PhongSize < 1.0)
                    texture -> Object_PhongSize = 1.0;
                if (texture -> Object_PhongSize > 100)
                    texture -> Object_PhongSize = 100; */
                break;

            case DIFFUSE_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                (texture->Object_Diffuse) = PrimitiveParser::parseFloat();
                break;

            case SPECULAR_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                (texture->Object_Specular) = PrimitiveParser::parseFloat();
                break;

            case PHONG_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                (texture->Object_Phong) = PrimitiveParser::parseFloat();
                break;

            case METALLIC_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Metallic_Flag = TRUE;
                break;

            case IOR_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                (texture->Object_Index_Of_Refraction) =
                    PrimitiveParser::parseFloat();
                break;

            case REFRACTION_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                (texture->Object_Refraction) = PrimitiveParser::parseFloat();
                break;

            case TRANSMIT_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                (texture->Object_Transmit) = PrimitiveParser::parseFloat();
                break;

            case REFLECTION_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                (texture->Object_Reflection) = PrimitiveParser::parseFloat();
                break;

            case IMAGEMAP_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Texture_Number = IMAGEMAP_TEXTURE;
                texture->Image = new RGBAImage;
                if (texture->Image == nullptr) {
                    ParseErrorReporter::Error(
                        "Out of memory. Cannot allocate imagemap texture");
                }
                *&texture->Image->Image_Gradient = Vector3Dd(1.0, -1.0, 0.0);
                texture->Image->Map_Type = PLANAR_MAP;
                texture->Image->Interpolation_Type = NO_INTERPOLATION;
                texture->Image->Once_Flag = FALSE;
                texture->Image->Use_Colour_Flag = TRUE;

                ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);

                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (globalToken.Token_Id) {
                        case DASH_TOKEN:
                        case PLUS_TOKEN:
                        case FLOAT_TOKEN:
                            Tokenizer::ungetToken();
                            (texture->Image->Map_Type) =
                                (int)PrimitiveParser::parseFloat();
                            break;

                        case LEFT_ANGLE_TOKEN:
                            Tokenizer::ungetToken();
                            PrimitiveParser::parseVector(
                                &(texture->Image->Image_Gradient));
                            break;

                        case IFF_TOKEN:
                            ParseHelpers::getExpectedToken(STRING_TOKEN);
                            IffFormat::readIffImage(
                                texture->Image, globalToken.Token_String);
                            Exit_Flag = TRUE;
                            break;

                        case GIF_TOKEN:
                            ParseHelpers::getExpectedToken(STRING_TOKEN);
                            GifFormat::readGifImage(
                                texture->Image, globalToken.Token_String);
                            Exit_Flag = TRUE;
                            break;

                        case TGA_TOKEN:
                            ParseHelpers::getExpectedToken(STRING_TOKEN);
                            TargaFormat::readTargaImage(
                                texture->Image, globalToken.Token_String);
                            Exit_Flag = TRUE;
                            break;

                        case DUMP_TOKEN:
                            ParseHelpers::getExpectedToken(STRING_TOKEN);
                            DumpFormat::readDumpImage(
                                texture->Image, globalToken.Token_String);
                            Exit_Flag = TRUE;
                            break;

                        default:
                            ParseErrorReporter::parseError(GIF_TOKEN);
                            break;
                        }
                    }
                }

                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (globalToken.Token_Id) {
                        case ONCE_TOKEN:
                            texture->Image->Once_Flag = TRUE;
                            break;

                        case INTERPOLATE_TOKEN:
                            texture->Image->Interpolation_Type =
                                (int)PrimitiveParser::parseFloat();
                            break;

                        case MAPTYPE_TOKEN:
                            (texture->Image->Map_Type) =
                                (int)PrimitiveParser::parseFloat();
                            break;

                        case USE_COLOUR_TOKEN:
                            texture->Image->Use_Colour_Flag = TRUE;
                            break;

                        case USE_INDEX_TOKEN:
                            texture->Image->Use_Colour_Flag = FALSE;
                            break;

                        case ALPHA_TOKEN: {
                            int Exit_Flag;
                            Exit_Flag = FALSE;
                            while (!Exit_Flag) {
                                Tokenizer::getToken();
                                switch (globalToken.Token_Id) {
                                case FLOAT_TOKEN:
                                    reg = (int)(globalToken.Token_Float + 0.01);
                                    if (texture->Image->Colour_Map == nullptr) {
                                        ParseErrorReporter::Error(
                                            "Can't apply ALPHA to a non "
                                            "colour-mapped image\n");
                                    }

                                    if ((reg < 0) ||
                                        (reg >=
                                            texture->Image->Colour_Map_Size)) {
                                        ParseErrorReporter::Error(
                                            "ALPHA colour register value out "
                                            "of range.\n");
                                    }

                                    texture->Image->Colour_Map[reg].Alpha =
                                        (unsigned short)(255.0 *
                                                         PrimitiveParser::
                                                             parseFloat());
                                    Exit_Flag = TRUE;
                                    break;

                                case ALL_TOKEN: {
                                    double alpha;
                                    alpha = PrimitiveParser::parseFloat();

                                    for (reg = 0;
                                        reg < texture->Image->Colour_Map_Size;
                                        reg++) {
                                        texture->Image->Colour_Map[reg].Alpha =
                                            (unsigned short)(alpha * 255.0);
                                    }
                                    Exit_Flag = TRUE;
                                }

                                break;
                                }
                            }
                        } break;

                        case RIGHT_CURLY_TOKEN:
                            Exit_Flag = TRUE;
                            break;

                        default:
                            ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN);
                            break;
                        }
                    }
                }
                break;

            case WAVES_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Bump_Number = WAVES;
                texture->Bump_Amount = PrimitiveParser::parseFloat();
                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (globalToken.Token_Id) {
                        case PHASE_TOKEN:
                            texture->Phase = PrimitiveParser::parseFloat();
                            Exit_Flag = TRUE;
                            break;

                        default:
                            Tokenizer::ungetToken();
                            Exit_Flag = TRUE;
                            break;
                        }
                    }
                }
                break;

            case FREQUENCY_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Frequency = PrimitiveParser::parseFloat();
                break;

            case PHASE_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Phase = PrimitiveParser::parseFloat();
                break;

            case RIPPLES_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Bump_Number = RIPPLES;
                texture->Bump_Amount = PrimitiveParser::parseFloat();
                break;

            case WRINKLES_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Bump_Number = WRINKLES;
                texture->Bump_Amount = PrimitiveParser::parseFloat();
                break;

            case BUMPS_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Bump_Number = BUMPS;
                texture->Bump_Amount = PrimitiveParser::parseFloat();
                break;

            case DENTS_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Bump_Number = DENTS;
                texture->Bump_Amount = PrimitiveParser::parseFloat();
                break;

            case TRANSLATE_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                PrimitiveParser::parseVector(&localVector);
                TextureUtils::translateTexture(&texture, &localVector);
                break;

            case ROTATE_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                PrimitiveParser::parseVector(&localVector);
                TextureUtils::rotateTexture(&texture, &localVector);
                break;

            case SCALE_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                PrimitiveParser::parseVector(&localVector);
                TextureUtils::scaleTexture(&texture, &localVector);
                break;

            case COLOUR_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Colour1 = SceneFactory::getColour();
                PrimitiveParser::parseColour(texture->Colour1);
                texture->Texture_Number = COLOUR_TEXTURE;
                break;

            case COLOUR_MAP_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Colour_Map = ColorMapParser::parseColorMap();
                break;

            case ONION_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Texture_Number = ONION_TEXTURE;
                break;

            case LEOPARD_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Texture_Number = LEOPARD_TEXTURE;
                break;

            /* New Texture Parsing - Cdw */
            case PAINTED1_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Texture_Number = PAINTED1_TEXTURE;
                break;

            case PAINTED2_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Texture_Number = PAINTED2_TEXTURE;
                break;

            case PAINTED3_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Texture_Number = PAINTED3_TEXTURE;
                break;

            case BUMPY1_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Bump_Number = BUMPY1;
                texture->Bump_Amount = PrimitiveParser::parseFloat();
                break;

            case BUMPY2_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Bump_Number = BUMPY2;
                texture->Bump_Amount = PrimitiveParser::parseFloat();
                break;

            case BUMPY3_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Bump_Number = BUMPY3;
                texture->Bump_Amount = PrimitiveParser::parseFloat();
                break;

            case BUMPMAP_TOKEN:
                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Bump_Number = BUMPMAP;
                texture->Bump_Image = new RGBAImage;
                if (texture->Bump_Image == nullptr) {
                    ParseErrorReporter::Error(
                        "Out of memory. Cannot allocate bumpmap texture");
                }
                *&texture->Bump_Image->Image_Gradient =
                    Vector3Dd(1.0, -1.0, 0.0);
                texture->Bump_Image->Map_Type = PLANAR_MAP;
                texture->Bump_Image->Interpolation_Type = NO_INTERPOLATION;
                texture->Bump_Image->Once_Flag = FALSE;
                texture->Bump_Image->Use_Colour_Flag = TRUE;

                ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);

                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (globalToken.Token_Id) {
                        case DASH_TOKEN:
                        case PLUS_TOKEN:
                        case FLOAT_TOKEN:
                            Tokenizer::ungetToken();
                            (texture->Bump_Image->Map_Type) =
                                (int)PrimitiveParser::parseFloat();
                            break;

                        case LEFT_ANGLE_TOKEN:
                            Tokenizer::ungetToken();
                            PrimitiveParser::parseVector(
                                &(texture->Bump_Image->Image_Gradient));
                            break;

                        case IFF_TOKEN:
                            ParseHelpers::getExpectedToken(STRING_TOKEN);
                            IffFormat::readIffImage(
                                texture->Bump_Image, globalToken.Token_String);
                            Exit_Flag = TRUE;
                            break;

                        case GIF_TOKEN:
                            ParseHelpers::getExpectedToken(STRING_TOKEN);
                            GifFormat::readGifImage(
                                texture->Bump_Image, globalToken.Token_String);
                            Exit_Flag = TRUE;
                            break;

                        case TGA_TOKEN:
                            ParseHelpers::getExpectedToken(STRING_TOKEN);
                            TargaFormat::readTargaImage(
                                texture->Bump_Image, globalToken.Token_String);
                            Exit_Flag = TRUE;
                            break;

                        case DUMP_TOKEN:
                            ParseHelpers::getExpectedToken(STRING_TOKEN);
                            DumpFormat::readDumpImage(
                                texture->Bump_Image, globalToken.Token_String);
                            Exit_Flag = TRUE;
                            break;

                        default:
                            ParseErrorReporter::parseError(GIF_TOKEN);
                            break;
                        }
                    }
                }

                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (globalToken.Token_Id) {
                        case ONCE_TOKEN:
                            texture->Bump_Image->Once_Flag = TRUE;
                            break;

                        case MAPTYPE_TOKEN:
                            (texture->Bump_Image->Map_Type) =
                                (int)PrimitiveParser::parseFloat();
                            break;

                        case INTERPOLATE_TOKEN:
                            texture->Bump_Image->Interpolation_Type =
                                (int)PrimitiveParser::parseFloat();
                            break;

                        case BUMPSIZE_TOKEN:
                            texture->Bump_Amount =
                                PrimitiveParser::parseFloat();
                            break;

                        case USE_COLOUR_TOKEN:
                            texture->Bump_Image->Use_Colour_Flag = TRUE;
                            break;
                        case USE_INDEX_TOKEN:
                            texture->Bump_Image->Use_Colour_Flag = FALSE;
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
                break;

            case MATERIAL_MAP_TOKEN:

                if (texture->Constant_Flag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->Constant_Flag = FALSE;
                }
                texture->Texture_Number = MATERIAL_MAP_TEXTURE;
                texture->Material_Image = new RGBAImage;
                if (texture->Material_Image == nullptr) {
                    ParseErrorReporter::Error(
                        "Out of memory. Cannot allocate material map texture");
                }
                *&texture->Texture_Gradient = Vector3Dd(1.0, -1.0, 0.0);
                texture->Material_Image->Map_Type = PLANAR_MAP;
                texture->Material_Image->Interpolation_Type = NO_INTERPOLATION;
                texture->Material_Image->Once_Flag = FALSE;
                texture->Material_Image->Use_Colour_Flag = FALSE;

                ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);

                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (globalToken.Token_Id) {
                        case DASH_TOKEN:
                        case PLUS_TOKEN:
                        case FLOAT_TOKEN:
                            Tokenizer::ungetToken();
                            (texture->Image->Map_Type) =
                                (int)PrimitiveParser::parseFloat();
                            break;

                        case LEFT_ANGLE_TOKEN:
                            Tokenizer::ungetToken();
                            PrimitiveParser::parseVector(
                                &(texture->Material_Image->Image_Gradient));
                            break;

                        case IFF_TOKEN:
                            ParseHelpers::getExpectedToken(STRING_TOKEN);
                            IffFormat::readIffImage(texture->Material_Image,
                                globalToken.Token_String);
                            Exit_Flag = TRUE;
                            break;

                        case GIF_TOKEN:
                            ParseHelpers::getExpectedToken(STRING_TOKEN);
                            GifFormat::readGifImage(texture->Material_Image,
                                globalToken.Token_String);
                            Exit_Flag = TRUE;
                            break;

                        case TGA_TOKEN:
                            ParseHelpers::getExpectedToken(STRING_TOKEN);
                            TargaFormat::readTargaImage(texture->Material_Image,
                                globalToken.Token_String);
                            Exit_Flag = TRUE;
                            break;

                        case DUMP_TOKEN:
                            ParseHelpers::getExpectedToken(STRING_TOKEN);
                            DumpFormat::readDumpImage(texture->Material_Image,
                                globalToken.Token_String);
                            Exit_Flag = TRUE;
                            break;

                        default:
                            ParseErrorReporter::parseError(GIF_TOKEN);
                            break;
                        }
                    }
                }

                /* remember where the First_Texture is */
                firstTexture = texture;

                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (globalToken.Token_Id) {

                        case MAPTYPE_TOKEN:
                            (texture->Material_Image->Map_Type) =
                                (int)PrimitiveParser::parseFloat();
                            break;

                        case INTERPOLATE_TOKEN:
                            texture->Material_Image->Interpolation_Type =
                                (int)PrimitiveParser::parseFloat();
                            break;

                        case ONCE_TOKEN:
                            texture->Material_Image->Once_Flag = TRUE;
                            break;

                        case TEXTURE_TOKEN: {
                            texture->Next_Material =
                                TextureParser::parseTexture();
                            firstTexture->Number_Of_Materials++;
                            texture = texture->Next_Material;
                        }

                        break;

                        case RIGHT_CURLY_TOKEN: {
                            texture->Next_Material = nullptr;
                            texture = firstTexture;
                            Exit_Flag = TRUE;
                        } break;

                        default:
                            ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN);
                            break;
                        }
                    }
                }
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
    return (texture);
}
