/****************************************************************************
 *                     tokenize.c
 *
 *  This module implements the first part of a two part parser for the scene
 *  description files.  This phase changes the input file into tokens.
 *
 *  from Persistence of Vision Raytracer
 *  Copyright 1992 Persistence of Vision Team
 *---------------------------------------------------------------------------
 *  Copying, distribution and legal info is in the file povlegal.doc which
 *  should be distributed with this file. If povlegal.doc is not available
 *  or for more info please contact:
 *
 *         Drew Wells [POV-Team Leader]
 *         CIS: 73767,1244  Internet: 73767.1244@compuserve.com
 *         Phone: (213) 254-4041
 *
 * This program is based on the popular DKB raytracer version 2.12.
 * DKBTrace was originally written by David K. Buck.
 * DKBTrace Ver 2.0-2.12 were written by David K. Buck & Aaron A. Collins.
 *
 *****************************************************************************/

#include "io/tokenize.h"
#include "common/frame.h"
#include "common/povproto.h"
#include <ctype.h>

/* This module tokenizes the input file and sends the tokens created
to the parser (the second stage).  Tokens sent to the parser contain a
token ID, the line number of the token, and if necessary, some data for
the token.  */

#define MAX_STRING_INDEX 41
char String[MAX_STRING_INDEX];
int String_Index;
extern char Library_Path[];
extern int Stop_Flag;
static int pov_stricmp(const char *s1, const char *s2);

/* Here are the reserved words.  If you need to add new words, be sure
to declare them in frame.h */

ReservedWord GLOBAL_reservedWords[LAST_TOKEN] = {{AGATE_TOKEN, "agate"},
    {ALL_TOKEN, "all"}, {ALPHA_TOKEN, "alpha"}, {AMBIENT_TOKEN, "ambient"},
    {AMPERSAND_TOKEN, "&"}, {AT_TOKEN, "@"}, {BACK_QUOTE_TOKEN, "`"},
    {BACK_SLASH_TOKEN, "\\"}, {BAR_TOKEN, "|"},
    {BICUBIC_PATCH_TOKEN, "bicubic_patch"}, {BLUE_TOKEN, "blue"},
    {BRICK_TOKEN, "brick"}, {BRILLIANCE_TOKEN, "brilliance"},
    {BLOB_TOKEN, "blob"}, {BOX_TOKEN, "box"}, {BOZO_TOKEN, "bozo"},
    {BOUNDED_TOKEN, "bounded_by"}, {BUMPS_TOKEN, "bumps"},
    {BUMPSIZE_TOKEN, "bump_size"}, {BUMPMAP_TOKEN, "bump_map"},
    {BUMPY1_TOKEN, "bumpy1"}, {BUMPY2_TOKEN, "bumpy2"},
    {BUMPY3_TOKEN, "bumpy3"}, {CENTER_TOKEN, "center"},
    {CHECKER_TOKEN, "checker"}, {CHECKER_TEXTURE_TOKEN, "tiles"},
    {CLIPPED_TOKEN, "clipped_by"}, {COLON_TOKEN, ":"}, {COLOUR_TOKEN, "color"},
    {COLOUR_TOKEN, "colour"}, {COLOUR_MAP_TOKEN, "color_map"},
    {COLOUR_MAP_TOKEN, "colour_map"}, {COMMA_TOKEN, ","},
    {COMPONENT_TOKEN, "component"}, {COMPOSITE_TOKEN, "composite"},
    {CONCENTRATION_TOKEN, "concentration"}, {CUBIC_TOKEN, "cubic"},
    {DASH_TOKEN, "-"}, {DECLARE_TOKEN, "declare"}, {DEFAULT_TOKEN, "default"},
    {DENTS_TOKEN, "dents"}, {DIFFERENCE_TOKEN, "difference"},
    {DIFFUSE_TOKEN, "diffuse"}, {DIRECTION_TOKEN, "direction"},
    {DOLLAR_TOKEN, "$"}, {DUMP_TOKEN, "dump"},
    {END_OF_FILE_TOKEN, "End of File"}, {EQUALS_TOKEN, "="},
    {EXCLAMATION_TOKEN, "!"}, {FALLOFF_TOKEN, "falloff"},
    {FLOAT_TOKEN, "float"}, {FOG_TOKEN, "fog"}, {FREQUENCY_TOKEN, "frequency"},
    {GIF_TOKEN, "gif"}, {GRANITE_TOKEN, "granite"},
    {GRADIENT_TOKEN, "gradient"}, {GREEN_TOKEN, "green"}, {HASH_TOKEN, "#"},
    {HAT_TOKEN, "^"}, {HEIGHT_FIELD_TOKEN, "height_field"},
    {IDENTIFIER_TOKEN, "identifier"}, {IFF_TOKEN, "iff"},
    {IMAGEMAP_TOKEN, "image_map"}, {INCLUDE_TOKEN, "include"},
    {INTERPOLATE_TOKEN, "interpolate"}, {INTERSECTION_TOKEN, "intersection"},
    {INVERSE_TOKEN, "inverse"}, {IOR_TOKEN, "ior"}, {LEFT_ANGLE_TOKEN, "<"},
    {LEFT_CURLY_TOKEN, "{"}, {LEFT_PAREN_TOKEN, "("}, {LEFT_SQUARE_TOKEN, "["},
    {LEOPARD_TOKEN, "leopard"}, {LIGHT_SOURCE_TOKEN, "light_source"},
    {LOCATION_TOKEN, "location"}, {LOOK_AT_TOKEN, "look_at"},
    {MARBLE_TOKEN, "marble"}, {MATERIAL_MAP_TOKEN, "material_map"},
    {MAPTYPE_TOKEN, "map_type"}, {MAX_TRACE_LEVEL_TOKEN, "max_trace_level"},
    {METALLIC_TOKEN, "metallic"}, {MORTAR_TOKEN, "mortar"},
    {NO_SHADOW_TOKEN, "no_shadow"}, {OBJECT_TOKEN, "object"},
    {OCTAVES_TOKEN, "octaves"}, {ONCE_TOKEN, "once"}, {ONION_TOKEN, "onion"},
    {PAINTED1_TOKEN, "painted1"}, {PAINTED2_TOKEN, "painted2"},
    {PAINTED3_TOKEN, "painted3"}, {PERCENT_TOKEN, "%"}, {PHASE_TOKEN, "phase"},
    {PHONG_TOKEN, "phong"}, {PHONGSIZE_TOKEN, "phong_size"},
    {PLANE_TOKEN, "plane"}, {PLUS_TOKEN, "+"}, {POINTS_TOKEN, "points"},
    {POINT_AT_TOKEN, "point_at"}, {POLY_TOKEN, "poly"},
    {POLYGON_TOKEN, "polygon"}, {POT_TOKEN, "pot"}, {QUADRIC_TOKEN, "quadric"},
    {QUARTIC_TOKEN, "quartic"}, {QUESTION_TOKEN, "?"}, {RADIUS_TOKEN, "radius"},
    {RAW_TOKEN, "raw"}, {RED_TOKEN, "red"}, {REFLECTION_TOKEN, "reflection"},
    {REFRACTION_TOKEN, "refraction"}, {REVOLVE_TOKEN, "revolve"},
    {RIGHT_TOKEN, "right"}, {RIGHT_CURLY_TOKEN, "}"}, {RIGHT_ANGLE_TOKEN, ">"},
    {RIGHT_PAREN_TOKEN, ")"}, {RIGHT_SQUARE_TOKEN, "]"},
    {RIPPLES_TOKEN, "ripples"}, {ROTATE_TOKEN, "rotate"},
    {ROUGHNESS_TOKEN, "roughness"}, {SCALE_TOKEN, "scale"},
    {SEMI_COLON_TOKEN, ";"}, {SHAPE_TOKEN, "shape"}, {SKY_TOKEN, "sky"},
    {SINGLE_QUOTE_TOKEN, "'"}, {SIZE_TOKEN, "size"}, {SLASH_TOKEN, "/"},
    {SMOOTH_TRIANGLE_TOKEN, "smooth_triangle"}, {SPECULAR_TOKEN, "specular"},
    {SPHERE_TOKEN, "sphere"}, {SPOTLIGHT_TOKEN, "spotlight"},
    {SPOTTED_TOKEN, "spotted"}, {STAR_TOKEN, "*"}, {STRING_TOKEN, "string"},
    {STURM_TOKEN, "sturm"}, {TEXTURE_TOKEN, "texture"}, {TGA_TOKEN, "tga"},
    {TIGHTNESS_TOKEN, "tightness"}, {TILDE_TOKEN, "~"}, {TILE2_TOKEN, "tile2"},
    {THRESHOLD_TOKEN, "threshold"}, {TRANSLATE_TOKEN, "translate"},
    {TRANSMIT_TOKEN, "* disabled *"}, /* Transmit disabled 2/92 */
    {TRIANGLE_TOKEN, "triangle"}, {TURBULENCE_TOKEN, "turbulence"},
    {UNION_TOKEN, "union"}, {UP_TOKEN, "up"}, {USE_COLOUR_TOKEN, "use_color"},
    {USE_COLOUR_TOKEN, "use_colour"}, {USE_INDEX_TOKEN, "use_index"},
    {VIEW_POINT_TOKEN, "camera"}, {WATER_LEVEL_TOKEN, "water_level"},
    {WAVES_TOKEN, "waves"}, {WOOD_TOKEN, "wood"}, {WRINKLES_TOKEN, "wrinkles"},
    {LAST_TOKEN, "orville"}};

/* Make a table for user-defined symbols.  500 symbols should be more
than enough. */

/* Now defined in POVRAY.c */
extern int Max_Symbols;

int token_count = 0, line_count = 0; /* moved here to allow reinitialization */

char **Symbol_Table;
int Number_Of_Symbols;
extern int Case_Sensitive_Flag; /* defined & init in povray.c */

#define MAX_INCLUDE_FILES 10

static DataFile GLOBAL_includeFiles[MAX_INCLUDE_FILES];
static DataFile *GLOBAL_dataFile;
static int GLOBAL_includeFileIndex;

TokenStruct GLOBAL_token;

#define CALL(x)                                                                \
    {                                                                          \
        if (!(x))                                                              \
            return (FALSE);                                                    \
    }

void
Initialize_Tokenizer(char *filename)
{
    Symbol_Table = NULL;
    GLOBAL_dataFile = NULL;

    GLOBAL_includeFileIndex = 0;
    GLOBAL_dataFile = &GLOBAL_includeFiles[0];

    GLOBAL_dataFile->File = Locate_File(filename, "r");
    if (GLOBAL_dataFile->File == NULL) {
        fprintf(stderr, "Cannot open input file\n");
        exit(1);
    }

    GLOBAL_dataFile->Filename = new char[strlen(filename) + 1];
    strcpy(GLOBAL_dataFile->Filename, filename);
    GLOBAL_dataFile->Line_Number = 0;

    if ((Symbol_Table = new char *[Max_Symbols]) == NULL) {
        fprintf(
            stderr, "Out of Memory. Cannot allocate space for symbol table\n");
        exit(1);
    }

    GLOBAL_token.End_Of_File = FALSE;
    Number_Of_Symbols = 0;
}

void
Terminate_Tokenizer()
{
    int i;

    if (Symbol_Table != NULL) {
        for (i = 1; i < Number_Of_Symbols; i++) {
            delete Symbol_Table[i];
        }

        delete Symbol_Table;
    }

    if (GLOBAL_dataFile != NULL) {
        fclose(GLOBAL_dataFile->File);
        delete GLOBAL_dataFile->Filename;
    }
}

/* The main tokenizing routine.  Set up the files and continue parsing
until the end of file */

/* This function performs most of the work involved in tokenizing.  It
    reads the first character of the token and decides which function to
    call to tokenize the rest.  For simple tokens, it simply writes them
    out to the token buffer.  */

/* Read a token from the input file and store it in the Token variable.
If the token is an INCLUDE token, then set the include file name and
read another token. */

void
Get_Token()
{
    register int c, c2;
    COOPERATE
    if (Stop_Flag) {
        close_all();
        exit(1);
    }
    if (GLOBAL_token.Unget_Token) {
        GLOBAL_token.Unget_Token = FALSE;
        return;
    }

    if (GLOBAL_token.End_Of_File) {
        return;
    }

    GLOBAL_token.Token_Id = END_OF_FILE_TOKEN;

    while (GLOBAL_token.Token_Id == END_OF_FILE_TOKEN) {

        GLOBAL_dataFile->SkipSpaces();

        c = getc(GLOBAL_dataFile->File);
        if (c == EOF) {
            if (GLOBAL_includeFileIndex == 0) {
                GLOBAL_token.Token_Id = END_OF_FILE_TOKEN;
                GLOBAL_token.End_Of_File = TRUE;
                /*putchar ('\n');*/
                fprintf(stderr, "\n");
                return;
            }

            fclose(GLOBAL_dataFile
                       ->File); /* added to fix open file buildup JLN 12/91 */

            GLOBAL_dataFile = &GLOBAL_includeFiles[--GLOBAL_includeFileIndex];
            continue;
        }

        String[0] = '\0';
        String_Index = 0;

        switch (c) {
        case '\n':
            GLOBAL_dataFile->Line_Number++;
            break;

        case '{':
            Write_Token(LEFT_CURLY_TOKEN, GLOBAL_dataFile);
            /* Parse_Comments(GLOBAL_dataFile); */
            break;

        case '}':
            Write_Token(RIGHT_CURLY_TOKEN, GLOBAL_dataFile);
            break;

        case '@':
            Write_Token(AT_TOKEN, GLOBAL_dataFile);
            break;

        case '&':
            Write_Token(AMPERSAND_TOKEN, GLOBAL_dataFile);
            break;

        case '`':
            Write_Token(BACK_QUOTE_TOKEN, GLOBAL_dataFile);
            break;

        case '\\':
            Write_Token(BACK_SLASH_TOKEN, GLOBAL_dataFile);
            break;

        case '|':
            Write_Token(BAR_TOKEN, GLOBAL_dataFile);
            break;

        case ':':
            Write_Token(COLON_TOKEN, GLOBAL_dataFile);
            break;

        case ',':
            Write_Token(COMMA_TOKEN, GLOBAL_dataFile);
            break;

        case '-':
            Write_Token(DASH_TOKEN, GLOBAL_dataFile);
            break;

        case '$':
            Write_Token(DOLLAR_TOKEN, GLOBAL_dataFile);
            break;

        case '=':
            Write_Token(EQUALS_TOKEN, GLOBAL_dataFile);
            break;

        case '!':
            Write_Token(EXCLAMATION_TOKEN, GLOBAL_dataFile);
            break;

        case '#': /* Parser doesn't use it, so let's ignore it */
            /* Write_Token (HASH_TOKEN, GLOBAL_dataFile); */
            break;

        case '^':
            Write_Token(HAT_TOKEN, GLOBAL_dataFile);
            break;

        case '<':
            Write_Token(LEFT_ANGLE_TOKEN, GLOBAL_dataFile);
            break;

        case '(':
            Write_Token(LEFT_PAREN_TOKEN, GLOBAL_dataFile);
            break;

        case '[':
            Write_Token(LEFT_SQUARE_TOKEN, GLOBAL_dataFile);
            break;

        case '%':
            Write_Token(PERCENT_TOKEN, GLOBAL_dataFile);
            break;

        case '+':
            Write_Token(PLUS_TOKEN, GLOBAL_dataFile);
            break;

        case '?':
            Write_Token(QUESTION_TOKEN, GLOBAL_dataFile);
            break;

        case '>':
            Write_Token(RIGHT_ANGLE_TOKEN, GLOBAL_dataFile);
            break;

        case ')':
            Write_Token(RIGHT_PAREN_TOKEN, GLOBAL_dataFile);
            break;

        case ']':
            Write_Token(RIGHT_SQUARE_TOKEN, GLOBAL_dataFile);
            break;

        case ';': /* Parser doesn't use it, so let's ignore it */
            /* Write_Token (SEMI_COLON_TOKEN, GLOBAL_dataFile); */
            break;

        case '\'':
            Write_Token(SINGLE_QUOTE_TOKEN, GLOBAL_dataFile);
            break;

            /* enable C++ style commenting */
        case '/':
            c2 = getc(GLOBAL_dataFile->File);
            if (c2 != (int)'/' && c2 != (int)'*') {
                ungetc(c2, GLOBAL_dataFile->File);
                Write_Token(SLASH_TOKEN, GLOBAL_dataFile);
                break;
            }
            if (c2 == (int)'*') {
                GLOBAL_dataFile->ParseCComments();
                break;
            }
            while (c2 != (int)'\n') {
                c2 = getc(GLOBAL_dataFile->File);
                if (c2 == EOF) {
                    ungetc(c2, GLOBAL_dataFile->File);
                    break;
                }
            }
            GLOBAL_dataFile->Line_Number++;
            break;

        case '*':
            Write_Token(STAR_TOKEN, GLOBAL_dataFile);
            break;

        case '~':
            Write_Token(TILDE_TOKEN, GLOBAL_dataFile);
            break;

        case '"':
            GLOBAL_dataFile->ParseString();
            break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '.':
            ungetc(c, GLOBAL_dataFile->File);
            if (GLOBAL_dataFile->ReadFloat() != TRUE) {
                return;
            }
            break;

        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h':
        case 'i':
        case 'j':
        case 'k':
        case 'l':
        case 'm':
        case 'n':
        case 'o':
        case 'p':
        case 'q':
        case 'r':
        case 's':
        case 't':
        case 'u':
        case 'v':
        case 'w':
        case 'x':
        case 'y':
        case 'z':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'H':
        case 'I':
        case 'J':
        case 'K':
        case 'L':
        case 'M':
        case 'N':
        case 'O':
        case 'P':
        case 'Q':
        case 'R':
        case 'S':
        case 'T':
        case 'U':
        case 'V':
        case 'W':
        case 'X':
        case 'Y':
        case 'Z':
        case '_':
            ungetc(c, GLOBAL_dataFile->File);
            if (GLOBAL_dataFile->ReadSymbol() != TRUE) {
                return;
            }
            break;
        case '\t':
        case '\r':
        case '\032': /* Control Z - EOF on many systems */
        case '\0':
            break;

        default:
            fprintf(stderr, "Error in %s line %d\n", GLOBAL_dataFile->Filename,
                GLOBAL_dataFile->Line_Number + 1);
            fprintf(
                stderr, "Illegal character in input file, value is %02x\n", c);
            break;
        }
        if (GLOBAL_token.Token_Id == INCLUDE_TOKEN) {
            if (GLOBAL_dataFile->SkipSpaces() != TRUE) {
                Token_Error(
                    GLOBAL_dataFile, "Expecting a string after INCLUDE\n");
            }

            if ((c = getc(GLOBAL_dataFile->File)) != '"') {
                Token_Error(
                    GLOBAL_dataFile, "Expecting a string after INCLUDE\n");
            }

            GLOBAL_dataFile->ParseString();
            GLOBAL_includeFileIndex++;
            if (GLOBAL_includeFileIndex > MAX_INCLUDE_FILES) {
                Token_Error(GLOBAL_dataFile, "Too many nested include files\n");
            }

            GLOBAL_dataFile = &GLOBAL_includeFiles[GLOBAL_includeFileIndex];
            GLOBAL_dataFile->Line_Number = 0;

            GLOBAL_dataFile->Filename =
                new char[strlen(GLOBAL_token.Token_String) + 1];
            if (GLOBAL_dataFile->Filename == NULL) {
                fprintf(stderr, "Out of memory opening include file: %s\n",
                    GLOBAL_token.Token_String);
                exit(1);
            }

            strcpy(GLOBAL_dataFile->Filename, GLOBAL_token.Token_String);

            if ((GLOBAL_dataFile->File = Locate_File(
                     GLOBAL_token.Token_String, "r")) == NULL) {
                fprintf(stderr, "Cannot open include file: %s\n",
                    GLOBAL_token.Token_String);
                exit(1);
            }
            GLOBAL_token.Token_Id = END_OF_FILE_TOKEN;
        }
    }

    token_count++;
    if (token_count > 1000) {
        token_count = 0;
        fprintf(stderr, ".");
        fflush(stderr);
        line_count++;
        if (line_count > 78) {
            line_count = 0;
            fprintf(stderr, "\n");
        }
    }
}

/* Mark that the token has been put back into the input stream.  The next
call to Get_Token will return the last-read token instead of reading a
new one from the file. */

void
Unget_Token()
{
    GLOBAL_token.Unget_Token = TRUE;
}

/* Skip over spaces in the input file */

int
DataFile::SkipSpaces()
{
    register int c;

    while (TRUE) {
        c = getc(this->File);
        if (c == EOF) {
            return (FALSE);
        }

        if (!(isspace(c) || c == 0x0A)) {
            break;
        }

        if (c == '\n') {
            this->Line_Number++;
        }
    }

    ungetc(c, this->File);
    return (TRUE);
}

/* The comments on comments are outdated and incorrect */
/* Comments start with an open brace ({) and end with a close brace (}).
    The open brace has been read already.  Continue reading until a close
    brace is encountered. Be sure to count the lines while you're at it.
    Incidently, nested comments are supported (in case you do such esoteric
    things) */

int
DataFile::ParseComments()
{
    register int c;
    int End_Of_Comment;

    End_Of_Comment = FALSE;
    while (!End_Of_Comment) {
        c = getc(this->File);
        if (c == EOF) {
            Token_Error(GLOBAL_dataFile, "No closing comment found");
            return (FALSE);
        }

        if (c == (int)'\n') {
            this->Line_Number++;
        }

        if (c == (int)'{')
            CALL(this->ParseComments())
        else {
            End_Of_Comment = (c == (int)'}');
        }
    }

    return (TRUE);
}

/* C style comments with asterik and slash - CdW 8/91 */

int
DataFile::ParseCComments()
{
    register int c, c2;
    int End_Of_Comment;

    End_Of_Comment = FALSE;
    while (!End_Of_Comment) {
        c = getc(this->File);
        if (c == EOF) {
            Token_Error(GLOBAL_dataFile, "No */ closing comment found");
            return (FALSE);
        }

        if (c == (int)'\n') {
            this->Line_Number++;
        }

        if (c == (int)'*') {
            c2 = getc(this->File);
            if (c2 != (int)'/') {
                ungetc(c2, this->File);
            } else {
                End_Of_Comment = TRUE;
            }
        }
        /* Check for and handle nested comments */
        if (c == (int)'/') {
            c2 = getc(this->File);
            if (c2 != (int)'*') {
                ungetc(c2, this->File);
            } else {
                this->ParseCComments();
            }
        }
    }
    return (TRUE);
}

/* The following routines make it easier to handle strings.  They stuff
    characters into a string buffer one at a time making all the proper
    range checks.  Call Begin_String to start, Stuff_Character to put
    characters in, and End_String to finish.  The String variable contains
    the final string. */

void
Begin_String()
{
    String_Index = 0;
}

void
Stuff_Character(int c, DataFile *GLOBAL_dataFile)
{
    if (String_Index < MAX_STRING_INDEX) {
        String[String_Index++] = (char)c;
        if (String_Index >= MAX_STRING_INDEX) {
            Token_Error(GLOBAL_dataFile, "String too long");
            String[String_Index - 1] = '\0';
        }
    }
}

void
DataFile::EndString()
{
    Stuff_Character((int)'\0', GLOBAL_dataFile);
}

/* Read a float from the input file and tokenize it as one token. The phase
    variable is 0 for the first character, 1 for all subsequent characters
    up to the decimal point, 2 for all characters after the decimal
    point, 3 for the E+/- and 4 for the exponent.  This helps to insure
    that the number is formatted properly. E format added 9/91 CEY */

int
DataFile::ReadFloat()
{
    register int c, Finished, Phase;

    Finished = FALSE;
    Phase = 0;

    Begin_String();
    while (!Finished) {
        c = getc(this->File);
        if (c == EOF) {
            Token_Error(GLOBAL_dataFile, "Unexpected end of file");
            return (FALSE);
        }

        switch (Phase) {
        case 0:
            if (isdigit(c)) {
                Stuff_Character(c, GLOBAL_dataFile);
            } else if (c == '.') {
                Stuff_Character('0', GLOBAL_dataFile);
                ungetc(c, this->File);
            } else {
                Token_Error(GLOBAL_dataFile, "Error in decimal number");
            }
            Phase = 1;
            break;

        case 1:
            if (isdigit(c)) {
                Stuff_Character(c, GLOBAL_dataFile);
            } else if (c == (int)'.') {
                Stuff_Character(c, GLOBAL_dataFile);
                Phase = 2;
            } else if ((c == 'e') || (c == 'E')) {
                Stuff_Character(c, GLOBAL_dataFile);
                Phase = 3;
            } else {
                Finished = TRUE;
            }
            break;

        case 2:
            if (isdigit(c)) {
                Stuff_Character(c, GLOBAL_dataFile);
            } else if ((c == 'e') || (c == 'E')) {
                Stuff_Character(c, GLOBAL_dataFile);
                Phase = 3;
            } else {
                Finished = TRUE;
            }
            break;

        case 3:
            if (isdigit(c) || (c == '+') || (c == '-')) {
                Stuff_Character(c, GLOBAL_dataFile);
                Phase = 4;
            } else {
                Finished = TRUE;
            }
            break;

        case 4:
            if (isdigit(c)) {
                Stuff_Character(c, GLOBAL_dataFile);
            } else {
                Finished = TRUE;
            }
            break;
        }
    }

    ungetc(c, this->File);
    this->EndString();

    Write_Token(FLOAT_TOKEN, GLOBAL_dataFile);
    if (sscanf(String, DBL_FORMAT_STRING, &GLOBAL_token.Token_Float) == 0) {
        return (FALSE);
    }

    return (TRUE);
}

/* Parse a string from the input file into a token. */
void
DataFile::ParseString()
{
    register int c;

    Begin_String();
    while (TRUE) {
        c = getc(this->File);
        if (c == EOF) {
            Token_Error(GLOBAL_dataFile, "No end quote for string");
        }

        if (c != (int)'"') {
            Stuff_Character(c, GLOBAL_dataFile);
        } else {
            break;
        }
    }
    this->EndString();

    Write_Token(STRING_TOKEN, GLOBAL_dataFile);
    GLOBAL_token.Token_String = String;
}

/* Read in a symbol from the input file.  Check to see if it is a reserved
    word.  If it is, write out the appropriate token.  Otherwise, write the
    symbol out to the Symbol file and write out an IDENTIFIER token. An
    Identifier token is a token whose token number is greater than the
    highest reserved word. */

int
DataFile::ReadSymbol()
{
    register int c, Symbol_Id;

    Begin_String();
    while (TRUE) {
        c = getc(this->File);
        if (c == EOF) {
            Token_Error(GLOBAL_dataFile, "Unexpected end of file");
            return (FALSE);
        }

        if (isalpha(c) || isdigit(c) || c == (int)'_') {
            Stuff_Character(c, GLOBAL_dataFile);
        } else {
            ungetc(c, this->File);
            break;
        }
    }
    this->EndString();

    /* Ignore the symbol if it was meant for the tokenizer (-2) */
    if ((Symbol_Id = Find_Reserved()) != -1 && Symbol_Id != -2) {
        Write_Token(Symbol_Id, GLOBAL_dataFile);
    } else {
        /* Ignore the symbol if it was meant for the tokenizer (-2) */
        if (Symbol_Id == -2) {
            return (TRUE);
        }

        if ((Symbol_Id = Find_Symbol()) == -1) {
            if (++Number_Of_Symbols < Max_Symbols) {
                if ((Symbol_Table[Number_Of_Symbols] =
                            new char[strlen(String) + 1]) == NULL) {
                    Token_Error(GLOBAL_dataFile,
                        "Out of memory. Cannot allocate space for identifier");
                }

                strcpy(Symbol_Table[Number_Of_Symbols], String);
                Symbol_Id = Number_Of_Symbols;
            } else {
                fprintf(stderr,
                    "\nToo many symbols. Use +ms### option to raise "
                    "Max_Symbols.\n");
                exit(1);
            }
        }

        Write_Token(LAST_TOKEN + Symbol_Id, GLOBAL_dataFile);
    }

    return (TRUE);
}

/* Return the index the token in the reserved words table or -1 if it
    isn't there. */
int
Find_Reserved()
{
    register int i;

    if (pov_stricmp("case_sensitive_yes", &(String[0])) == 0) {
        Case_Sensitive_Flag = 0;
        return (-2);
    }
    if (pov_stricmp("case_sensitive_no", &(String[0])) == 0) {
        Case_Sensitive_Flag = 1;
        return (-2);
    }
    /* The optional case sensitive option only checks keywords unsensitive */
    /* Symbols can be upper/lower, but not be the same as a keyword */
    else if (pov_stricmp("case_sensitive_opt", &(String[0])) == 0) {
        Case_Sensitive_Flag = 2;
        return (-2);
    }

    for (i = 0; i < LAST_TOKEN; i++) {
        if (Case_Sensitive_Flag == 0) {
            if (strcmp(GLOBAL_reservedWords[i].Token_Name, &(String[0])) == 0) {
                return (GLOBAL_reservedWords[i].Token_Number);
            }
        } else {
            if (pov_stricmp((char *)GLOBAL_reservedWords[i].Token_Name,
                    &(String[0])) == 0) {
                return (GLOBAL_reservedWords[i].Token_Number);
            }
        }
    }

    return (-1);
}

/* Check to see if a symbol already exists with this name.  If so, return
     its symbol ID. */

int
Find_Symbol()
{
    register int i;

    for (i = 1; i <= Number_Of_Symbols; i++) {
        if (Case_Sensitive_Flag == 0 || Case_Sensitive_Flag == 2) {
            if (strcmp(Symbol_Table[i], String) == 0) {
                return (i);
            }
            if (Case_Sensitive_Flag == 1) {
                if (pov_stricmp(Symbol_Table[i], String) == 0) {
                    return (i);
                }
            }
        }
    }

    return (-1);
}

/* Write a token out to the token file */
void
Write_Token(TOKEN Token_Id, DataFile *GLOBAL_dataFile)
{
    GLOBAL_token.Token_Id = Token_Id;
    GLOBAL_token.Token_Line_No = GLOBAL_dataFile->Line_Number;
    GLOBAL_token.Filename = GLOBAL_dataFile->Filename;
    GLOBAL_token.Token_String = String;

    if (GLOBAL_token.Token_Id > LAST_TOKEN) {
        GLOBAL_token.Identifier_Number =
            (int)GLOBAL_token.Token_Id - (int)LAST_TOKEN;
        GLOBAL_token.Token_Id = IDENTIFIER_TOKEN;
    }
}

/* Report an error */
void
Token_Error(DataFile *GLOBAL_dataFile, const char *str)
{
    fprintf(stderr, "Error in %s line %d\n", GLOBAL_dataFile->Filename,
        GLOBAL_dataFile->Line_Number);
    fputs(str, stderr);
    fputs("\n\n", stderr);
    exit(1);
}
/* Since the stricmp function isn't available on all systems, we've
    provided a simplified version of it here. */

static int
pov_stricmp(const char *s1, const char *s2)
{
    char c1, c2;

    while ((*s1 != '\0') && (*s2 != '\0')) {
        c1 = *s1++;
        c2 = *s2++;
        if (islower(c1)) {
            c1 = (char)toupper(c1);
        }
        if (islower(c2)) {
            c2 = (char)toupper(c2);
        }
        if (c1 < c2) {
            return (-1);
        }
        if (c1 > c2) {
            return (1);
        }
    }

    if (*s1 == '\0') {
        if (*s2 == '\0') {
            return (0);
        } else {
            return (-1);
        }
    } else {
        return (1);
    }
}
