#ifndef __TOKENIZER_H__
#define __TOKENIZER_H__

#include "io/pov/lexer/DataFile.h"
#include "io/pov/lexer/ReservedWord.h"
#include "io/pov/lexer/TokenStruct.h"

typedef int TOKEN;

/**
Here's where you dump the information on the current token
*/
class Tokenizer {
  public:

    // Token Definitions for Parser
    // This list must have the same number of tokens as list in tokenize.c
    static constexpr int AGATE_TOKEN = 0;
    static constexpr int ALL_TOKEN = 1;
    static constexpr int ALPHA_TOKEN = 2;
    static constexpr int AMBIENT_TOKEN = 3;
    static constexpr int AMPERSAND_TOKEN = 4;
    static constexpr int AT_TOKEN = 5;
    static constexpr int BACK_QUOTE_TOKEN = 6;
    static constexpr int BACK_SLASH_TOKEN = 7;
    static constexpr int BAR_TOKEN = 8;
    static constexpr int BICUBIC_PATCH_TOKEN = 9;
    static constexpr int BLUE_TOKEN = 10;
    static constexpr int BRILLIANCE_TOKEN = 11;
    static constexpr int BOZO_TOKEN = 12;
    static constexpr int BOUNDED_TOKEN = 13;
    static constexpr int BUMPS_TOKEN = 14;
    static constexpr int CHECKER_TOKEN = 15;
    static constexpr int CHECKER_TEXTURE_TOKEN = 16;
    static constexpr int CLIPPED_TOKEN = 17;
    static constexpr int COLON_TOKEN = 18;
    static constexpr int COLOR_TOKEN = 19;
    static constexpr int COLOUR_TOKEN = 20;
    static constexpr int COLOR_MAP_TOKEN = 21;
    static constexpr int COLOUR_MAP_TOKEN = 22;
    static constexpr int COMMA_TOKEN = 23;
    static constexpr int COMPOSITE_TOKEN = 24;
    static constexpr int CONCENTRATION_TOKEN = 25;
    static constexpr int CUBIC_TOKEN = 26;
    static constexpr int DASH_TOKEN = 27;
    static constexpr int DECLARE_TOKEN = 28;
    static constexpr int DENTS_TOKEN = 29;
    static constexpr int DIFFERENCE_TOKEN = 30;
    static constexpr int DIFFUSE_TOKEN = 31;
    static constexpr int DIRECTION_TOKEN = 32;
    static constexpr int DOLLAR_TOKEN = 33;
    static constexpr int DUMP_TOKEN = 34;
    static constexpr int EQUALS_TOKEN = 35;
    static constexpr int EXCLAMATION_TOKEN = 36;
    static constexpr int FLOAT_TOKEN = 37;
    static constexpr int FOG_TOKEN = 38;
    static constexpr int FREQUENCY_TOKEN = 39;
    static constexpr int GIF_TOKEN = 40;
    static constexpr int GRADIENT_TOKEN = 41;
    static constexpr int GRANITE_TOKEN = 42;
    static constexpr int GREEN_TOKEN = 43;
    static constexpr int HASH_TOKEN = 44;
    static constexpr int HAT_TOKEN = 45;
    static constexpr int IDENTIFIER_TOKEN = 46;
    static constexpr int IFF_TOKEN = 47;
    static constexpr int IMAGEMAP_TOKEN = 48;
    static constexpr int INCLUDE_TOKEN = 49;
    static constexpr int INTERSECTION_TOKEN = 50;
    static constexpr int INVERSE_TOKEN = 51;
    static constexpr int IOR_TOKEN = 52;
    static constexpr int LEFT_ANGLE_TOKEN = 53;
    static constexpr int LEFT_CURLY_TOKEN = 54;
    static constexpr int LEFT_SQUARE_TOKEN = 55;
    static constexpr int LIGHT_SOURCE_TOKEN = 56;
    static constexpr int LOCATION_TOKEN = 57;
    static constexpr int LOOK_AT_TOKEN = 58;
    static constexpr int MARBLE_TOKEN = 59;
    static constexpr int METALLIC_TOKEN = 60;
    static constexpr int OBJECT_TOKEN = 61;
    static constexpr int ONCE_TOKEN = 62;
    static constexpr int PERCENT_TOKEN = 63;
    static constexpr int PHASE_TOKEN = 64;
    static constexpr int PHONG_TOKEN = 65;
    static constexpr int PHONGSIZE_TOKEN = 66;
    static constexpr int PLANE_TOKEN = 67;
    static constexpr int PLUS_TOKEN = 68;
    static constexpr int POINTS_TOKEN = 69;
    static constexpr int POINT_AT_TOKEN = 70;
    static constexpr int POLYGON_TOKEN = 71;
    static constexpr int POLY_TOKEN = 72;
    static constexpr int QUADRIC_TOKEN = 73;
    static constexpr int QUARTIC_TOKEN = 74;
    static constexpr int QUESTION_TOKEN = 75;
    static constexpr int RAW_TOKEN = 76;
    static constexpr int RED_TOKEN = 77;
    static constexpr int REFLECTION_TOKEN = 78;
    static constexpr int REFRACTION_TOKEN = 79;
    static constexpr int REVOLVE_TOKEN = 80;
    static constexpr int RIGHT_TOKEN = 81;
    static constexpr int RIGHT_ANGLE_TOKEN = 82;
    static constexpr int RIGHT_PAREN_TOKEN = 83;
    static constexpr int RIGHT_SQUARE_TOKEN = 84;
    static constexpr int RIPPLES_TOKEN = 85;
    static constexpr int ROTATE_TOKEN = 86;
    static constexpr int ROUGHNESS_TOKEN = 87;
    static constexpr int SCALE_TOKEN = 88;
    static constexpr int SEMI_COLON_TOKEN = 89;
    static constexpr int SHAPE_TOKEN = 90;
    static constexpr int SINGLE_QUOTE_TOKEN = 91;
    static constexpr int SIZE_TOKEN = 92;
    static constexpr int SKY_TOKEN = 93;
    static constexpr int SLASH_TOKEN = 94;
    static constexpr int SMOOTH_TRIANGLE_TOKEN = 95;
    static constexpr int SPECULAR_TOKEN = 96;
    static constexpr int SPHERE_TOKEN = 97;
    static constexpr int SPOTLIGHT_TOKEN = 98;
    static constexpr int SPOTTED_TOKEN = 99;
    static constexpr int STAR_TOKEN = 100;
    static constexpr int STRING_TOKEN = 101;
    static constexpr int STURM_TOKEN = 102;
    static constexpr int TEXTURE_TOKEN = 103;
    static constexpr int TILDE_TOKEN = 104;
    static constexpr int TILE2_TOKEN = 105;
    static constexpr int TRANSLATE_TOKEN = 106;
    static constexpr int TRANSMIT_TOKEN = 107;
    static constexpr int TRIANGLE_TOKEN = 108;
    static constexpr int TURBULENCE_TOKEN = 109;
    static constexpr int UNION_TOKEN = 110;
    static constexpr int UP_TOKEN = 111;
    static constexpr int VIEW_POINT_TOKEN = 112;
    static constexpr int WAVES_TOKEN = 113;
    static constexpr int WOOD_TOKEN = 114;
    static constexpr int WRINKLES_TOKEN = 115;
    static constexpr int PAINTED1_TOKEN = 116;
    static constexpr int PAINTED2_TOKEN = 117;
    static constexpr int PAINTED3_TOKEN = 118;
    static constexpr int BUMPY1_TOKEN = 119;
    static constexpr int BUMPY2_TOKEN = 120;
    static constexpr int BUMPY3_TOKEN = 121;
    static constexpr int BUMPMAP_TOKEN = 122;
    static constexpr int BUMPSIZE_TOKEN = 123;
    static constexpr int MATERIAL_MAP_TOKEN = 124;
    static constexpr int ONION_TOKEN = 125;
    static constexpr int LEOPARD_TOKEN = 126;
    static constexpr int DUMMY_TOKEN =
        127; // Dummy token to pad list because some colour_tokens are used twice
    static constexpr int INTERPOLATE_TOKEN = 128;
    static constexpr int HEIGHT_FIELD_TOKEN = 129;
    static constexpr int POT_TOKEN = 130;
    static constexpr int WATER_LEVEL_TOKEN = 131;
    static constexpr int USE_COLOUR_TOKEN = 132;
    static constexpr int USE_INDEX_TOKEN = 133;
    static constexpr int MAPTYPE_TOKEN = 134;
    static constexpr int RIGHT_CURLY_TOKEN = 135;
    static constexpr int LEFT_PAREN_TOKEN = 136;
    static constexpr int TGA_TOKEN = 137;       // ARE 11/91 for tga mapp/heights
    static constexpr int CENTER_TOKEN = 138;    // ARE 11/91 for spotlight
    static constexpr int FALLOFF_TOKEN = 139;   // ARE 11/91 for spotlight
    static constexpr int TIGHTNESS_TOKEN = 140; // ARE 11/91 for spotlight
    static constexpr int RADIUS_TOKEN = 141;    // ARE 11/91 for spotlight
    static constexpr int NO_SHADOW_TOKEN =
        142; // CEY 12/91 for shadowless objects
    static constexpr int END_OF_FILE_TOKEN = 143;
    static constexpr int MAX_TRACE_LEVEL_TOKEN = 144;
    static constexpr int DEFAULT_TOKEN = 145;
    static constexpr int BOX_TOKEN = 146;       // ARE 1/92 for boxes
    static constexpr int BLOB_TOKEN = 147;      // ARE 1/92 for blobs
    static constexpr int THRESHOLD_TOKEN = 148; // ARE 1/92 for blobs
    static constexpr int COMPONENT_TOKEN = 149; // ARE 1/92 for blobs
    static constexpr int OCTAVES_TOKEN = 150;   // DMF 2/92 for turb
    static constexpr int BRICK_TOKEN = 151;     // RHA 2/92 for brick
    static constexpr int MORTAR_TOKEN = 152;    // RHA 2/92 for brick
    static constexpr int LOST_TOKEN = 153;
    static constexpr int LAST_TOKEN = 154;

    static ReservedWord *reservedWords();
    static TokenStruct &token();
    static void setCaseSensitiveIdentifiers(int mode);
    static void initializeTokenizer(const char *inputFileName);
    static void terminateTokenizer(void);
    static void ungetToken(void);
    static void beginString(void);
    static void stuffCharacter(int c, const DataFile *dataFile);
    static int findReserved(void);
    static int findSymbol(void);
    static void writeToken(TOKEN tokenId, const DataFile *dataFile);
    static void tokenError(const DataFile *dataFile, const char *str);
    static void getToken(void);
    static void setMaxSymbols(int value);
    static int getMaxSymbols();
    static DataFile *getGlobalDataFile();
    static char *getString();
    static int &getNumberOfSymbols();
    static char **getSymbolTable();

  private:
    static constexpr int MAX_STRING_INDEX = 41;
    static constexpr int MAX_INCLUDE_FILES = 10;
    static ReservedWord sReservedWords[LAST_TOKEN];
    static TokenStruct sToken;
    static int maxSymbols;
    static char sString[MAX_STRING_INDEX];
    static int sStringIndex;
    static int sCaseSensitiveFlag;
    static int sTokenCount;
    static int sLineCount;
    static char **sSymbolTable;
    static int sNumberOfSymbols;
    static DataFile sGlobalIncludeFiles[MAX_INCLUDE_FILES];
    static DataFile *sGlobalDataFile;
    static int sGlobalIncludeFileIndex;

    static int povStricmp(const char *s1, const char *s2);
};

#endif
