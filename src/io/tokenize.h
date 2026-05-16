#ifndef __TOKENIZE_H__
#define __TOKENIZE_H__

#include <stdio.h>
#include "common/frame.h"

/* Token Definitions for Parser */
/* This list must have the same number of tokens as list in tokenize.c */
#define AGATE_TOKEN 0
#define ALL_TOKEN 1
#define ALPHA_TOKEN 2
#define AMBIENT_TOKEN 3
#define AMPERSAND_TOKEN 4
#define AT_TOKEN 5
#define BACK_QUOTE_TOKEN 6
#define BACK_SLASH_TOKEN 7
#define BAR_TOKEN 8
#define BICUBIC_PATCH_TOKEN 9
#define BLUE_TOKEN 10
#define BRILLIANCE_TOKEN 11
#define BOZO_TOKEN 12
#define BOUNDED_TOKEN 13
#define BUMPS_TOKEN 14
#define CHECKER_TOKEN 15
#define CHECKER_TEXTURE_TOKEN 16
#define CLIPPED_TOKEN 17
#define COLON_TOKEN 18
#define COLOR_TOKEN 19
#define COLOUR_TOKEN 20
#define COLOR_MAP_TOKEN 21
#define COLOUR_MAP_TOKEN 22
#define COMMA_TOKEN 23
#define COMPOSITE_TOKEN 24
#define CONCENTRATION_TOKEN 25
#define CUBIC_TOKEN 26
#define DASH_TOKEN 27
#define DECLARE_TOKEN 28
#define DENTS_TOKEN 29
#define DIFFERENCE_TOKEN 30
#define DIFFUSE_TOKEN 31
#define DIRECTION_TOKEN 32
#define DOLLAR_TOKEN 33
#define DUMP_TOKEN 34
#define EQUALS_TOKEN 35
#define EXCLAMATION_TOKEN 36
#define FLOAT_TOKEN 37
#define FOG_TOKEN 38
#define FREQUENCY_TOKEN 39
#define GIF_TOKEN 40
#define GRADIENT_TOKEN 41
#define GRANITE_TOKEN 42
#define GREEN_TOKEN 43
#define HASH_TOKEN 44
#define HAT_TOKEN 45
#define IDENTIFIER_TOKEN 46
#define IFF_TOKEN 47
#define IMAGEMAP_TOKEN 48
#define INCLUDE_TOKEN 49
#define INTERSECTION_TOKEN 50
#define INVERSE_TOKEN 51
#define IOR_TOKEN 52
#define LEFT_ANGLE_TOKEN 53
#define LEFT_CURLY_TOKEN 54
#define LEFT_SQUARE_TOKEN 55
#define LIGHT_SOURCE_TOKEN 56
#define LOCATION_TOKEN 57
#define LOOK_AT_TOKEN 58
#define MARBLE_TOKEN 59
#define METALLIC_TOKEN 60
#define OBJECT_TOKEN 61
#define ONCE_TOKEN 62
#define PERCENT_TOKEN 63
#define PHASE_TOKEN 64
#define PHONG_TOKEN 65
#define PHONGSIZE_TOKEN 66
#define PLANE_TOKEN 67
#define PLUS_TOKEN 68
#define POINTS_TOKEN 69
#define POINT_AT_TOKEN 70
#define POLYGON_TOKEN 71
#define POLY_TOKEN 72
#define QUADRIC_TOKEN 73
#define QUARTIC_TOKEN 74
#define QUESTION_TOKEN 75
#define RAW_TOKEN 76
#define RED_TOKEN 77
#define REFLECTION_TOKEN 78
#define REFRACTION_TOKEN 79
#define REVOLVE_TOKEN 80
#define RIGHT_TOKEN 81
#define RIGHT_ANGLE_TOKEN 82
#define RIGHT_PAREN_TOKEN 83
#define RIGHT_SQUARE_TOKEN 84
#define RIPPLES_TOKEN 85
#define ROTATE_TOKEN 86
#define ROUGHNESS_TOKEN 87
#define SCALE_TOKEN 88
#define SEMI_COLON_TOKEN 89
#define SHAPE_TOKEN 90
#define SINGLE_QUOTE_TOKEN 91
#define SIZE_TOKEN 92
#define SKY_TOKEN 93
#define SLASH_TOKEN 94
#define SMOOTH_TRIANGLE_TOKEN 95
#define SPECULAR_TOKEN 96
#define SPHERE_TOKEN 97
#define SPOTLIGHT_TOKEN 98
#define SPOTTED_TOKEN 99
#define STAR_TOKEN 100
#define STRING_TOKEN 101
#define STURM_TOKEN 102
#define TEXTURE_TOKEN 103
#define TILDE_TOKEN 104
#define TILE2_TOKEN 105
#define TRANSLATE_TOKEN 106
#define TRANSMIT_TOKEN 107
#define TRIANGLE_TOKEN 108
#define TURBULENCE_TOKEN 109
#define UNION_TOKEN 110
#define UP_TOKEN 111
#define VIEW_POINT_TOKEN 112
#define WAVES_TOKEN 113
#define WOOD_TOKEN 114
#define WRINKLES_TOKEN 115
#define PAINTED1_TOKEN 116
#define PAINTED2_TOKEN 117
#define PAINTED3_TOKEN 118
#define BUMPY1_TOKEN 119
#define BUMPY2_TOKEN 120
#define BUMPY3_TOKEN 121
#define BUMPMAP_TOKEN 122
#define BUMPSIZE_TOKEN 123
#define MATERIAL_MAP_TOKEN 124
#define ONION_TOKEN 125
#define LEOPARD_TOKEN 126
#define DUMMY_TOKEN                                                            \
    127 /* Dummy token to pad list because some colour_tokens are used twice   \
         */
#define INTERPOLATE_TOKEN 128
#define HEIGHT_FIELD_TOKEN 129
#define POT_TOKEN 130
#define WATER_LEVEL_TOKEN 131
#define USE_COLOUR_TOKEN 132
#define USE_INDEX_TOKEN 133
#define MAPTYPE_TOKEN 134
#define RIGHT_CURLY_TOKEN 135
#define LEFT_PAREN_TOKEN 136
#define TGA_TOKEN 137       /* ARE 11/91 for tga mapp/heights */
#define CENTER_TOKEN 138    /* ARE 11/91 for spotlight */
#define FALLOFF_TOKEN 139   /* ARE 11/91 for spotlight */
#define TIGHTNESS_TOKEN 140 /* ARE 11/91 for spotlight */
#define RADIUS_TOKEN 141    /* ARE 11/91 for spotlight */
#define NO_SHADOW_TOKEN 142 /* CEY 12/91 for shadowless objects */
#define END_OF_FILE_TOKEN 143
#define MAX_TRACE_LEVEL_TOKEN 144
#define DEFAULT_TOKEN 145
#define BOX_TOKEN 146       /* ARE 1/92 for boxes */
#define BLOB_TOKEN 147      /* ARE 1/92 for blobs */
#define THRESHOLD_TOKEN 148 /* ARE 1/92 for blobs */
#define COMPONENT_TOKEN 149 /* ARE 1/92 for blobs */
#define OCTAVES_TOKEN 150   /* DMF 2/92 for turb  */
#define BRICK_TOKEN 151     /* RHA 2/92 for brick */
#define MORTAR_TOKEN 152    /* RHA 2/92 for brick */
#define LOST_TOKEN 153
#define LAST_TOKEN 154

typedef int TOKEN;

class ReservedWord {
  public:
    TOKEN Token_Number;
    const char *Token_Name;
};

/* Here's where you dump the information on the current token (fm. PARSE.C) */

class TokenStruct {
  public:
    TOKEN Token_Id;
    int Token_Line_No;
    char *Token_String;
    DBL Token_Float;
    int Identifier_Number;
    int Unget_Token, End_Of_File;
    char *Filename;
};

class DataFile {
  public:
    FILE *File;
    char *Filename;
    int Line_Number;
};

extern void Initialize_Tokenizer(char *Input_File_Name);
extern void Terminate_Tokenizer(void);
extern void Tokenize(char *name);
extern int Process_Token(void);
extern int Skip_Spaces(DataFile *Data_File);
extern int Parse_Comments(DataFile *Data_File);
extern int Parse_C_Comments(DataFile *Data_File);
extern void Begin_String(void);
extern void Stuff_Character(int c, DataFile *Data_File);
extern void End_String(DataFile *Data_File);
extern int Read_Float(DataFile *Data_File);
extern void Parse_String(DataFile *Data_File);
extern int Read_Symbol(DataFile *Data_File);
extern int Find_Reserved(void);
extern int Find_Symbol(void);
extern void Write_Token(int Token_Id, DataFile *Data_File);
extern void Token_Error(DataFile *Data_File, const char *str);
extern void Get_Token(void);
extern void Unget_Token(void);

#endif
