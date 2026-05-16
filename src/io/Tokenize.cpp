/****************************************************************************
 *                     tokenize.c
 *
 *  This module implements the first part of a two part parser for the scene
 *  description files.  This phase changes the input file into tokens.
 *
 *****************************************************************************/

#include "io/Tokenize.h"
#include "common/Frame.h"
#include "common/PovProto.h"
#include <cctype>

/* This module tokenizes the input file and sends the tokens created
to the parser (the second stage).  Tokens sent to the parser contain a
token ID, the line number of the token, and if necessary, some data for
the token.  */

static constexpr int MAX_STRING_INDEX = 41;
char string[MAX_STRING_INDEX];
int stringIndex;
extern char libraryPath[];
extern int stopFlag;
static int povStricmp(const char *s1, const char *s2);

/* Here are the reserved words.  If you need to add new words, be sure
to declare them in frame.h */

ReservedWord globalReservedWords[LAST_TOKEN] = {{AGATE_TOKEN, "agate"},
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
extern int maxSymbols;

int tokenCount = 0, lineCount = 0; /* moved here to allow reinitialization */

char **symbolTable;
int numberOfSymbols;
extern int caseSensitiveFlag; /* defined & init in povray.c */

static constexpr int MAX_INCLUDE_FILES = 10;

static DataFile globalIncludeFiles[MAX_INCLUDE_FILES];
static DataFile *globalDataFile;
static int globalIncludeFileIndex;

TokenStruct globalToken;

void
initializeTokenizer(char *filename)
{
    symbolTable = nullptr;
    globalDataFile = nullptr;

    globalIncludeFileIndex = 0;
    globalDataFile = &globalIncludeFiles[0];

    globalDataFile->File = PovApp::locateFile(filename, "r");
    if (globalDataFile->File == nullptr) {
        fprintf(stderr, "Cannot open input file\n");
        exit(1);
    }

    globalDataFile->Filename = new char[strlen(filename) + 1];
    strcpy(globalDataFile->Filename, filename);
    globalDataFile->Line_Number = 0;

    if ((symbolTable = new char *[maxSymbols]) == nullptr) {
        fprintf(
            stderr, "Out of Memory. Cannot allocate space for symbol table\n");
        exit(1);
    }

    globalToken.End_Of_File = FALSE;
    numberOfSymbols = 0;
}

void
terminateTokenizer()
{
    int i;

    if (symbolTable != nullptr) {
        for (i = 1; i < numberOfSymbols; i++) {
            delete symbolTable[i];
        }

        delete symbolTable;
    }

    if (globalDataFile != nullptr) {
        fclose(globalDataFile->File);
        delete globalDataFile->Filename;
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
getToken()
{
    register int c;
    register int c2;
    cooperate();
    if (stopFlag) {
        PovApp::closeAll();
        exit(1);
    }
    if (globalToken.Unget_Token) {
        globalToken.Unget_Token = FALSE;
        return;
    }

    if (globalToken.End_Of_File) {
        return;
    }

    globalToken.Token_Id = END_OF_FILE_TOKEN;

    while (globalToken.Token_Id == END_OF_FILE_TOKEN) {

        globalDataFile->skipSpaces();

        c = getc(globalDataFile->File);
        if (c == EOF) {
            if (globalIncludeFileIndex == 0) {
                globalToken.Token_Id = END_OF_FILE_TOKEN;
                globalToken.End_Of_File = TRUE;
                /*putchar ('\n');*/
                fprintf(stderr, "\n");
                return;
            }

            fclose(globalDataFile
                       ->File); /* added to fix open file buildup JLN 12/91 */

            globalDataFile = &globalIncludeFiles[--globalIncludeFileIndex];
            continue;
        }

        string[0] = '\0';
        stringIndex = 0;

        switch (c) {
        case '\n':
            globalDataFile->Line_Number++;
            break;

        case '{':
            writeToken(LEFT_CURLY_TOKEN, globalDataFile);
            /* parseComments(GLOBAL_dataFile); */
            break;

        case '}':
            writeToken(RIGHT_CURLY_TOKEN, globalDataFile);
            break;

        case '@':
            writeToken(AT_TOKEN, globalDataFile);
            break;

        case '&':
            writeToken(AMPERSAND_TOKEN, globalDataFile);
            break;

        case '`':
            writeToken(BACK_QUOTE_TOKEN, globalDataFile);
            break;

        case '\\':
            writeToken(BACK_SLASH_TOKEN, globalDataFile);
            break;

        case '|':
            writeToken(BAR_TOKEN, globalDataFile);
            break;

        case ':':
            writeToken(COLON_TOKEN, globalDataFile);
            break;

        case ',':
            writeToken(COMMA_TOKEN, globalDataFile);
            break;

        case '-':
            writeToken(DASH_TOKEN, globalDataFile);
            break;

        case '$':
            writeToken(DOLLAR_TOKEN, globalDataFile);
            break;

        case '=':
            writeToken(EQUALS_TOKEN, globalDataFile);
            break;

        case '!':
            writeToken(EXCLAMATION_TOKEN, globalDataFile);
            break;

        case '#': /* Parser doesn't use it, so let's ignore it */
            /* writeToken (HASH_TOKEN, GLOBAL_dataFile); */
            break;

        case '^':
            writeToken(HAT_TOKEN, globalDataFile);
            break;

        case '<':
            writeToken(LEFT_ANGLE_TOKEN, globalDataFile);
            break;

        case '(':
            writeToken(LEFT_PAREN_TOKEN, globalDataFile);
            break;

        case '[':
            writeToken(LEFT_SQUARE_TOKEN, globalDataFile);
            break;

        case '%':
            writeToken(PERCENT_TOKEN, globalDataFile);
            break;

        case '+':
            writeToken(PLUS_TOKEN, globalDataFile);
            break;

        case '?':
            writeToken(QUESTION_TOKEN, globalDataFile);
            break;

        case '>':
            writeToken(RIGHT_ANGLE_TOKEN, globalDataFile);
            break;

        case ')':
            writeToken(RIGHT_PAREN_TOKEN, globalDataFile);
            break;

        case ']':
            writeToken(RIGHT_SQUARE_TOKEN, globalDataFile);
            break;

        case ';': /* Parser doesn't use it, so let's ignore it */
            /* writeToken (SEMI_COLON_TOKEN, GLOBAL_dataFile); */
            break;

        case '\'':
            writeToken(SINGLE_QUOTE_TOKEN, globalDataFile);
            break;

            /* enable C++ style commenting */
        case '/':
            c2 = getc(globalDataFile->File);
            if (c2 != (int)'/' && c2 != (int)'*') {
                ungetc(c2, globalDataFile->File);
                writeToken(SLASH_TOKEN, globalDataFile);
                break;
            }
            if (c2 == (int)'*') {
                globalDataFile->parseCComments();
                break;
            }
            while (c2 != (int)'\n') {
                c2 = getc(globalDataFile->File);
                if (c2 == EOF) {
                    ungetc(c2, globalDataFile->File);
                    break;
                }
            }
            globalDataFile->Line_Number++;
            break;

        case '*':
            writeToken(STAR_TOKEN, globalDataFile);
            break;

        case '~':
            writeToken(TILDE_TOKEN, globalDataFile);
            break;

        case '"':
            globalDataFile->parseString();
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
            ungetc(c, globalDataFile->File);
            if (globalDataFile->readFloat() != TRUE) {
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
            ungetc(c, globalDataFile->File);
            if (globalDataFile->readSymbol() != TRUE) {
                return;
            }
            break;
        case '\t':
        case '\r':
        case '\032': /* Control Z - EOF on many systems */
        case '\0':
            break;

        default:
            fprintf(stderr, "Error in %s line %d\n", globalDataFile->Filename,
                globalDataFile->Line_Number + 1);
            fprintf(
                stderr, "Illegal character in input file, value is %02x\n", c);
            break;
        }
        if (globalToken.Token_Id == INCLUDE_TOKEN) {
            if (globalDataFile->skipSpaces() != TRUE) {
                tokenError(
                    globalDataFile, "Expecting a string after INCLUDE\n");
            }

            if ((c = getc(globalDataFile->File)) != '"') {
                tokenError(
                    globalDataFile, "Expecting a string after INCLUDE\n");
            }

            globalDataFile->parseString();
            globalIncludeFileIndex++;
            if (globalIncludeFileIndex > MAX_INCLUDE_FILES) {
                tokenError(globalDataFile, "Too many nested include files\n");
            }

            globalDataFile = &globalIncludeFiles[globalIncludeFileIndex];
            globalDataFile->Line_Number = 0;

            globalDataFile->Filename =
                new char[strlen(globalToken.Token_String) + 1];
            if (globalDataFile->Filename == nullptr) {
                fprintf(stderr, "Out of memory opening include file: %s\n",
                    globalToken.Token_String);
                exit(1);
            }

            strcpy(globalDataFile->Filename, globalToken.Token_String);

            if ((globalDataFile->File = PovApp::locateFile(
                     globalToken.Token_String, "r")) == nullptr) {
                fprintf(stderr, "Cannot open include file: %s\n",
                    globalToken.Token_String);
                exit(1);
            }
            globalToken.Token_Id = END_OF_FILE_TOKEN;
        }
    }

    tokenCount++;
    if (tokenCount > 1000) {
        tokenCount = 0;
        fprintf(stderr, ".");
        fflush(stderr);
        lineCount++;
        if (lineCount > 78) {
            lineCount = 0;
            fprintf(stderr, "\n");
        }
    }
}

/* Mark that the token has been put back into the input stream.  The next
call to Get_Token will return the last-read token instead of reading a
new one from the file. */

void
ungetToken()
{
    globalToken.Unget_Token = TRUE;
}

/* Skip over spaces in the input file */

int
DataFile::skipSpaces()
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
DataFile::parseComments()
{
    register int c;
    int endOfComment;

    endOfComment = FALSE;
    while (!endOfComment) {
        c = getc(this->File);
        if (c == EOF) {
            tokenError(globalDataFile, "No closing comment found");
            return (FALSE);
        }

        if (c == (int)'\n') {
            this->Line_Number++;
        }

        if (c == (int)'{') {
            if (!this->parseComments()) {
                return (FALSE);
            }
        }
        else {
            endOfComment = (c == (int)'}');
        }
    }

    return (TRUE);
}

/* C style comments with asterik and slash - CdW 8/91 */

int
DataFile::parseCComments()
{
    register int c;
    register int c2;
    int endOfComment;

    endOfComment = FALSE;
    while (!endOfComment) {
        c = getc(this->File);
        if (c == EOF) {
            tokenError(globalDataFile, "No */ closing comment found");
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
                endOfComment = TRUE;
            }
        }
        /* Check for and handle nested comments */
        if (c == (int)'/') {
            c2 = getc(this->File);
            if (c2 != (int)'*') {
                ungetc(c2, this->File);
            } else {
                this->parseCComments();
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
beginString()
{
    stringIndex = 0;
}

void
stuffCharacter(int c, DataFile *globalDataFile)
{
    if (stringIndex < MAX_STRING_INDEX) {
        string[stringIndex++] = (char)c;
        if (stringIndex >= MAX_STRING_INDEX) {
            tokenError(globalDataFile, "String too long");
            string[stringIndex - 1] = '\0';
        }
    }
}

void
DataFile::endString()
{
    stuffCharacter((int)'\0', globalDataFile);
}

/* Read a float from the input file and tokenize it as one token. The phase
    variable is 0 for the first character, 1 for all subsequent characters
    up to the decimal point, 2 for all characters after the decimal
    point, 3 for the E+/- and 4 for the exponent.  This helps to insure
    that the number is formatted properly. E format added 9/91 CEY */

int
DataFile::readFloat()
{
    register int c;
    register int finished;
    register int phase;

    finished = FALSE;
    phase = 0;

    beginString();
    while (!finished) {
        c = getc(this->File);
        if (c == EOF) {
            tokenError(globalDataFile, "Unexpected end of file");
            return (FALSE);
        }

        switch (phase) {
        case 0:
            if (isdigit(c)) {
                stuffCharacter(c, globalDataFile);
            } else if (c == '.') {
                stuffCharacter('0', globalDataFile);
                ungetc(c, this->File);
            } else {
                tokenError(globalDataFile, "Error in decimal number");
            }
            phase = 1;
            break;

        case 1:
            if (isdigit(c)) {
                stuffCharacter(c, globalDataFile);
            } else if (c == (int)'.') {
                stuffCharacter(c, globalDataFile);
                phase = 2;
            } else if ((c == 'e') || (c == 'E')) {
                stuffCharacter(c, globalDataFile);
                phase = 3;
            } else {
                finished = TRUE;
            }
            break;

        case 2:
            if (isdigit(c)) {
                stuffCharacter(c, globalDataFile);
            } else if ((c == 'e') || (c == 'E')) {
                stuffCharacter(c, globalDataFile);
                phase = 3;
            } else {
                finished = TRUE;
            }
            break;

        case 3:
            if (isdigit(c) || (c == '+') || (c == '-')) {
                stuffCharacter(c, globalDataFile);
                phase = 4;
            } else {
                finished = TRUE;
            }
            break;

        case 4:
            if (isdigit(c)) {
                stuffCharacter(c, globalDataFile);
            } else {
                finished = TRUE;
            }
            break;
        }
    }

    ungetc(c, this->File);
    this->endString();

    writeToken(FLOAT_TOKEN, globalDataFile);
    if (sscanf(string, DBL_FORMAT_STRING, &globalToken.Token_Float) == 0) {
        return (FALSE);
    }

    return (TRUE);
}

/* Parse a string from the input file into a token. */
void
DataFile::parseString()
{
    register int c;

    beginString();
    while (TRUE) {
        c = getc(this->File);
        if (c == EOF) {
            tokenError(globalDataFile, "No end quote for string");
        }

        if (c != (int)'"') {
            stuffCharacter(c, globalDataFile);
        } else {
            break;
        }
    }
    this->endString();

    writeToken(STRING_TOKEN, globalDataFile);
    globalToken.Token_String = string;
}

/* Read in a symbol from the input file.  Check to see if it is a reserved
    word.  If it is, write out the appropriate token.  Otherwise, write the
    symbol out to the Symbol file and write out an IDENTIFIER token. An
    Identifier token is a token whose token number is greater than the
    highest reserved word. */

int
DataFile::readSymbol()
{
    register int c;
    register int symbolId;

    beginString();
    while (TRUE) {
        c = getc(this->File);
        if (c == EOF) {
            tokenError(globalDataFile, "Unexpected end of file");
            return (FALSE);
        }

        if (isalpha(c) || isdigit(c) || c == (int)'_') {
            stuffCharacter(c, globalDataFile);
        } else {
            ungetc(c, this->File);
            break;
        }
    }
    this->endString();

    /* Ignore the symbol if it was meant for the tokenizer (-2) */
    if ((symbolId = findReserved()) != -1 && symbolId != -2) {
        writeToken(symbolId, globalDataFile);
    } else {
        /* Ignore the symbol if it was meant for the tokenizer (-2) */
        if (symbolId == -2) {
            return (TRUE);
        }

        if ((symbolId = findSymbol()) == -1) {
            if (++numberOfSymbols < maxSymbols) {
                if ((symbolTable[numberOfSymbols] =
                            new char[strlen(string) + 1]) == nullptr) {
                    tokenError(globalDataFile,
                        "Out of memory. Cannot allocate space for identifier");
                }

                strcpy(symbolTable[numberOfSymbols], string);
                symbolId = numberOfSymbols;
            } else {
                fprintf(stderr,
                    "\nToo many symbols. Use +ms### option to raise "
                    "Max_Symbols.\n");
                exit(1);
            }
        }

        writeToken(LAST_TOKEN + symbolId, globalDataFile);
    }

    return (TRUE);
}

/* Return the index the token in the reserved words table or -1 if it
    isn't there. */
int
findReserved()
{
    register int i;

    if (povStricmp("case_sensitive_yes", &(string[0])) == 0) {
        caseSensitiveFlag = 0;
        return (-2);
    }
    if (povStricmp("case_sensitive_no", &(string[0])) == 0) {
        caseSensitiveFlag = 1;
        return (-2);
    }
    /* The optional case sensitive option only checks keywords unsensitive */
    /* Symbols can be upper/lower, but not be the same as a keyword */
    if (povStricmp("case_sensitive_opt", &(string[0])) == 0) {
        caseSensitiveFlag = 2;
        return (-2);
    }

    for (i = 0; i < LAST_TOKEN; i++) {
        if (caseSensitiveFlag == 0) {
            if (strcmp(globalReservedWords[i].Token_Name, &(string[0])) == 0) {
                return (globalReservedWords[i].Token_Number);
            }
        } else {
            if (povStricmp((char *)globalReservedWords[i].Token_Name,
                    &(string[0])) == 0) {
                return (globalReservedWords[i].Token_Number);
            }
        }
    }

    return (-1);
}

/* Check to see if a symbol already exists with this name.  If so, return
     its symbol ID. */

int
findSymbol()
{
    register int i;

    for (i = 1; i <= numberOfSymbols; i++) {
        if (caseSensitiveFlag == 0 || caseSensitiveFlag == 2) {
            if (strcmp(symbolTable[i], string) == 0) {
                return (i);
            }
            if (caseSensitiveFlag == 1) {
                if (povStricmp(symbolTable[i], string) == 0) {
                    return (i);
                }
            }
        }
    }

    return (-1);
}

/* Write a token out to the token file */
void
writeToken(TOKEN tokenId, DataFile *globalDataFile)
{
    globalToken.Token_Id = tokenId;
    globalToken.Token_Line_No = globalDataFile->Line_Number;
    globalToken.Filename = globalDataFile->Filename;
    globalToken.Token_String = string;

    if (globalToken.Token_Id > LAST_TOKEN) {
        globalToken.Identifier_Number =
            (int)globalToken.Token_Id - (int)LAST_TOKEN;
        globalToken.Token_Id = IDENTIFIER_TOKEN;
    }
}

/* Report an error */
void
tokenError(DataFile *globalDataFile, const char *str)
{
    fprintf(stderr, "Error in %s line %d\n", globalDataFile->Filename,
        globalDataFile->Line_Number);
    fputs(str, stderr);
    fputs("\n\n", stderr);
    exit(1);
}
/* Since the stricmp function isn't available on all systems, we've
    provided a simplified version of it here. */

static int
povStricmp(const char *s1, const char *s2)
{
    char c1;
    char c2;

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
        }
        return (-1);

    } else {
        return (1);
    }
}
