/**
tokenize.c

This module implements the first part of a two part parser for the scene
description files.  This phase changes the input file into tokens.
*/

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "vsdk/toolkit/common/logging/Logger.h"
#include "io/binaryIo/FileLocator.h"
#include "io/pov/lexer/Tokenizer.h"

/**
This module tokenizes the input file and sends the tokens created
to the parser (the second stage).  Tokens sent to the parser contain a
token ID, the line number of the token, and if necessary, some data for
the token.
*/

char Tokenizer::sString[Tokenizer::MAX_STRING_INDEX];
int Tokenizer::sStringIndex = 0;

/**
Here are the reserved words.  If you need to add new words, be sure
to declare them in frame.h
*/

ReservedWord Tokenizer::sReservedWords[Tokenizer::LAST_TOKEN] = {{Tokenizer::AGATE_TOKEN, "agate"},
    {Tokenizer::ALL_TOKEN, "all"}, {Tokenizer::ALPHA_TOKEN, "alpha"}, {Tokenizer::AMBIENT_TOKEN, "ambient"},
    {Tokenizer::AMPERSAND_TOKEN, "&"}, {Tokenizer::AT_TOKEN, "@"}, {Tokenizer::BACK_QUOTE_TOKEN, "`"},
    {Tokenizer::BACK_SLASH_TOKEN, "\\"}, {Tokenizer::BAR_TOKEN, "|"},
    {Tokenizer::BICUBIC_PATCH_TOKEN, "bicubic_patch"}, {Tokenizer::BLUE_TOKEN, "blue"},
    {Tokenizer::BRICK_TOKEN, "brick"}, {Tokenizer::BRILLIANCE_TOKEN, "brilliance"},
    {Tokenizer::BLOB_TOKEN, "blob"}, {Tokenizer::BOX_TOKEN, "box"}, {Tokenizer::BOZO_TOKEN, "bozo"},
    {Tokenizer::BOUNDED_TOKEN, "bounded_by"}, {Tokenizer::BUMPS_TOKEN, "bumps"},
    {Tokenizer::BUMPSIZE_TOKEN, "bump_size"}, {Tokenizer::BUMPMAP_TOKEN, "bump_map"},
    {Tokenizer::BUMPY1_TOKEN, "bumpy1"}, {Tokenizer::BUMPY2_TOKEN, "bumpy2"},
    {Tokenizer::BUMPY3_TOKEN, "bumpy3"}, {Tokenizer::CENTER_TOKEN, "center"},
    {Tokenizer::CHECKER_TOKEN, "checker"}, {Tokenizer::CHECKER_TEXTURE_TOKEN, "tiles"},
    {Tokenizer::CLIPPED_TOKEN, "clipped_by"}, {Tokenizer::COLON_TOKEN, ":"}, {Tokenizer::COLOUR_TOKEN, "color"},
    {Tokenizer::COLOUR_TOKEN, "colour"}, {Tokenizer::COLOUR_MAP_TOKEN, "color_map"},
    {Tokenizer::COLOUR_MAP_TOKEN, "colour_map"}, {Tokenizer::COMMA_TOKEN, ","},
    {Tokenizer::COMPONENT_TOKEN, "component"}, {Tokenizer::COMPOSITE_TOKEN, "composite"},
    {Tokenizer::CONCENTRATION_TOKEN, "concentration"}, {Tokenizer::CUBIC_TOKEN, "cubic"},
    {Tokenizer::DASH_TOKEN, "-"}, {Tokenizer::DECLARE_TOKEN, "declare"}, {Tokenizer::DEFAULT_TOKEN, "default"},
    {Tokenizer::DENTS_TOKEN, "dents"}, {Tokenizer::DIFFERENCE_TOKEN, "difference"},
    {Tokenizer::DIFFUSE_TOKEN, "diffuse"}, {Tokenizer::DIRECTION_TOKEN, "direction"},
    {Tokenizer::DOLLAR_TOKEN, "$"}, {Tokenizer::DUMP_TOKEN, "dump"},
    {Tokenizer::END_OF_FILE_TOKEN, "End of File"}, {Tokenizer::EQUALS_TOKEN, "="},
    {Tokenizer::EXCLAMATION_TOKEN, "!"}, {Tokenizer::FALLOFF_TOKEN, "falloff"},
    {Tokenizer::FLOAT_TOKEN, "float"}, {Tokenizer::FOG_TOKEN, "fog"}, {Tokenizer::FREQUENCY_TOKEN, "frequency"},
    {Tokenizer::GIF_TOKEN, "gif"}, {Tokenizer::GRANITE_TOKEN, "granite"},
    {Tokenizer::GRADIENT_TOKEN, "gradient"}, {Tokenizer::GREEN_TOKEN, "green"}, {Tokenizer::HASH_TOKEN, "#"},
    {Tokenizer::HAT_TOKEN, "^"}, {Tokenizer::HEIGHT_FIELD_TOKEN, "height_field"},
    {Tokenizer::IDENTIFIER_TOKEN, "identifier"}, {Tokenizer::IFF_TOKEN, "iff"},
    {Tokenizer::IMAGEMAP_TOKEN, "image_map"}, {Tokenizer::INCLUDE_TOKEN, "include"},
    {Tokenizer::INTERPOLATE_TOKEN, "interpolate"}, {Tokenizer::INTERSECTION_TOKEN, "intersection"},
    {Tokenizer::INVERSE_TOKEN, "inverse"}, {Tokenizer::IOR_TOKEN, "ior"}, {Tokenizer::LEFT_ANGLE_TOKEN, "<"},
    {Tokenizer::LEFT_CURLY_TOKEN, "{"}, {Tokenizer::LEFT_PAREN_TOKEN, "("}, {Tokenizer::LEFT_SQUARE_TOKEN, "["},
    {Tokenizer::LEOPARD_TOKEN, "leopard"}, {Tokenizer::LIGHT_SOURCE_TOKEN, "light_source"},
    {Tokenizer::LOCATION_TOKEN, "location"}, {Tokenizer::LOOK_AT_TOKEN, "look_at"},
    {Tokenizer::MARBLE_TOKEN, "marble"}, {Tokenizer::MATERIAL_MAP_TOKEN, "material_map"},
    {Tokenizer::MAPTYPE_TOKEN, "map_type"}, {Tokenizer::MAX_TRACE_LEVEL_TOKEN, "max_trace_level"},
    {Tokenizer::METALLIC_TOKEN, "metallic"}, {Tokenizer::MORTAR_TOKEN, "mortar"},
    {Tokenizer::NO_SHADOW_TOKEN, "no_shadow"}, {Tokenizer::OBJECT_TOKEN, "object"},
    {Tokenizer::OCTAVES_TOKEN, "octaves"}, {Tokenizer::ONCE_TOKEN, "once"}, {Tokenizer::ONION_TOKEN, "onion"},
    {Tokenizer::PAINTED1_TOKEN, "painted1"}, {Tokenizer::PAINTED2_TOKEN, "painted2"},
    {Tokenizer::PAINTED3_TOKEN, "painted3"}, {Tokenizer::PERCENT_TOKEN, "%"}, {Tokenizer::PHASE_TOKEN, "phase"},
    {Tokenizer::PHONG_TOKEN, "phong"}, {Tokenizer::PHONGSIZE_TOKEN, "phong_size"},
    {Tokenizer::PLANE_TOKEN, "plane"}, {Tokenizer::PLUS_TOKEN, "+"}, {Tokenizer::POINTS_TOKEN, "points"},
    {Tokenizer::POINT_AT_TOKEN, "point_at"}, {Tokenizer::POLY_TOKEN, "poly"},
    {Tokenizer::POLYGON_TOKEN, "polygon"}, {Tokenizer::POT_TOKEN, "pot"}, {Tokenizer::QUADRIC_TOKEN, "quadric"},
    {Tokenizer::QUARTIC_TOKEN, "quartic"}, {Tokenizer::QUESTION_TOKEN, "?"}, {Tokenizer::RADIUS_TOKEN, "radius"},
    {Tokenizer::RAW_TOKEN, "raw"}, {Tokenizer::RED_TOKEN, "red"}, {Tokenizer::REFLECTION_TOKEN, "reflection"},
    {Tokenizer::REFRACTION_TOKEN, "refraction"}, {Tokenizer::REVOLVE_TOKEN, "revolve"},
    {Tokenizer::RIGHT_TOKEN, "right"}, {Tokenizer::RIGHT_CURLY_TOKEN, "}"}, {Tokenizer::RIGHT_ANGLE_TOKEN, ">"},
    {Tokenizer::RIGHT_PAREN_TOKEN, ")"}, {Tokenizer::RIGHT_SQUARE_TOKEN, "]"},
    {Tokenizer::RIPPLES_TOKEN, "ripples"}, {Tokenizer::ROTATE_TOKEN, "rotate"},
    {Tokenizer::ROUGHNESS_TOKEN, "roughness"}, {Tokenizer::SCALE_TOKEN, "scale"},
    {Tokenizer::SEMI_COLON_TOKEN, ";"}, {Tokenizer::SHAPE_TOKEN, "shape"}, {Tokenizer::SKY_TOKEN, "sky"},
    {Tokenizer::SINGLE_QUOTE_TOKEN, "'"}, {Tokenizer::SIZE_TOKEN, "size"}, {Tokenizer::SLASH_TOKEN, "/"},
    {Tokenizer::SMOOTH_TRIANGLE_TOKEN, "smooth_triangle"}, {Tokenizer::SPECULAR_TOKEN, "specular"},
    {Tokenizer::SPHERE_TOKEN, "sphere"}, {Tokenizer::SPOTLIGHT_TOKEN, "spotlight"},
    {Tokenizer::SPOTTED_TOKEN, "spotted"}, {Tokenizer::STAR_TOKEN, "*"}, {Tokenizer::STRING_TOKEN, "string"},
    {Tokenizer::STURM_TOKEN, "sturm"}, {Tokenizer::TEXTURE_TOKEN, "texture"}, {Tokenizer::TGA_TOKEN, "tga"},
    {Tokenizer::TIGHTNESS_TOKEN, "tightness"}, {Tokenizer::TILDE_TOKEN, "~"}, {Tokenizer::TILE2_TOKEN, "tile2"},
    {Tokenizer::THRESHOLD_TOKEN, "threshold"}, {Tokenizer::TRANSLATE_TOKEN, "translate"},
    {Tokenizer::TRANSMIT_TOKEN, "* disabled *"}, // Transmit disabled 2/92
    {Tokenizer::TRIANGLE_TOKEN, "triangle"}, {Tokenizer::TURBULENCE_TOKEN, "turbulence"},
    {Tokenizer::UNION_TOKEN, "union"}, {Tokenizer::UP_TOKEN, "up"}, {Tokenizer::USE_COLOUR_TOKEN, "use_color"},
    {Tokenizer::USE_COLOUR_TOKEN, "use_colour"}, {Tokenizer::USE_INDEX_TOKEN, "use_index"},
    {Tokenizer::VIEW_POINT_TOKEN, "camera"}, {Tokenizer::WATER_LEVEL_TOKEN, "water_level"},
    {Tokenizer::WAVES_TOKEN, "waves"}, {Tokenizer::WOOD_TOKEN, "wood"}, {Tokenizer::WRINKLES_TOKEN, "wrinkles"},
    {Tokenizer::LAST_TOKEN, "orville"}};

/**
Make a table for user-defined symbols.  500 symbols should be more
than enough.
*/

int Tokenizer::maxSymbols = 500;
int Tokenizer::sCaseSensitiveFlag = 0;
int Tokenizer::sTokenCount = 0;
int Tokenizer::sLineCount = 0;
char **Tokenizer::sSymbolTable = nullptr;
int Tokenizer::sNumberOfSymbols = 0;
DataFile Tokenizer::sGlobalIncludeFiles[Tokenizer::MAX_INCLUDE_FILES];
DataFile *Tokenizer::sGlobalDataFile = nullptr;
int Tokenizer::sGlobalIncludeFileIndex = 0;

TokenStruct Tokenizer::sToken;

ReservedWord *
Tokenizer::reservedWords()
{
    return sReservedWords;
}

TokenStruct &
Tokenizer::token()
{
    return sToken;
}

void
Tokenizer::setCaseSensitiveIdentifiers(int mode)
{
    Tokenizer::sCaseSensitiveFlag = mode;
}

void
Tokenizer::setMaxSymbols(int value)
{
    maxSymbols = value;
}

int
Tokenizer::getMaxSymbols()
{
    return maxSymbols;
}

DataFile *
Tokenizer::getGlobalDataFile()
{
    return Tokenizer::sGlobalDataFile;
}

char *
Tokenizer::getString()
{
    return Tokenizer::sString;
}

int &
Tokenizer::getNumberOfSymbols()
{
    return Tokenizer::sNumberOfSymbols;
}

char **
Tokenizer::getSymbolTable()
{
    return Tokenizer::sSymbolTable;
}

void
Tokenizer::initializeTokenizer(char *filename)
{
    Tokenizer::sSymbolTable = nullptr;
    Tokenizer::sGlobalDataFile = nullptr;

    Tokenizer::sGlobalIncludeFileIndex = 0;
    Tokenizer::sGlobalDataFile = &Tokenizer::sGlobalIncludeFiles[0];

    Tokenizer::sGlobalDataFile->File = FileLocator::locate(filename, "r");
    if (Tokenizer::sGlobalDataFile->File == nullptr) {
        Logger::reportMessage("Tokenizer", Logger::FATAL_ERROR, "", "Cannot open input file\n");
    }

    Tokenizer::sGlobalDataFile->Filename = new char[strlen(filename) + 1];
    strcpy(Tokenizer::sGlobalDataFile->Filename, filename);
    Tokenizer::sGlobalDataFile->lineNumber = 0;

    if ((Tokenizer::sSymbolTable = new char *[maxSymbols]) == nullptr) {
        Logger::reportMessage("Tokenizer", Logger::FATAL_ERROR, "", "Out of Memory. Cannot allocate space for symbol table\n");
    }

    Tokenizer::token().endOfFile = false;
    Tokenizer::sNumberOfSymbols = 0;
}

void
Tokenizer::terminateTokenizer()
{
    int i;

    if (Tokenizer::sSymbolTable != nullptr) {
        for (i = 1; i <= Tokenizer::sNumberOfSymbols; i++) {
            delete[] Tokenizer::sSymbolTable[i];
        }

        delete[] Tokenizer::sSymbolTable;
    }

    if (Tokenizer::sGlobalDataFile != nullptr) {
        fclose(Tokenizer::sGlobalDataFile->File);
        delete[] Tokenizer::sGlobalDataFile->Filename;
    }
}

/**
The main tokenizing routine.  Set up the files and continue parsing
until the end of file.

This function performs most of the work involved in tokenizing.  It
reads the first character of the token and decides which function to
call to tokenize the rest.  For simple tokens, it simply writes them
out to the token buffer.

Read a token from the input file and store it in the Token variable.
If the token is an INCLUDE token, then set the include file name and
read another token.
*/

void
Tokenizer::getToken()
{
    int c;
    int c2;
    if (Tokenizer::token().ungetToken) {
        Tokenizer::token().ungetToken = false;
        return;
    }

    if (Tokenizer::token().endOfFile) {
        return;
    }

    Tokenizer::token().tokenId = Tokenizer::END_OF_FILE_TOKEN;

    while (Tokenizer::token().tokenId == Tokenizer::END_OF_FILE_TOKEN) {

        Tokenizer::sGlobalDataFile->skipSpaces();

        c = getc(Tokenizer::sGlobalDataFile->File);
        if (c == EOF) {
            if (Tokenizer::sGlobalIncludeFileIndex == 0) {
                Tokenizer::token().tokenId = Tokenizer::END_OF_FILE_TOKEN;
                Tokenizer::token().endOfFile = true;
                // putchar ('\n');
                fprintf(stderr, "\n");
                return;
            }

            fclose(Tokenizer::sGlobalDataFile->File);
            delete[] Tokenizer::sGlobalDataFile->Filename;
            Tokenizer::sGlobalDataFile->Filename = nullptr;

            Tokenizer::sGlobalDataFile = &Tokenizer::sGlobalIncludeFiles[--Tokenizer::sGlobalIncludeFileIndex];
            continue;
        }

        Tokenizer::sString[0] = '\0';
        Tokenizer::sStringIndex = 0;

        switch (c) {
        case '\n':
            Tokenizer::sGlobalDataFile->lineNumber++;
            break;

        case '{':
            Tokenizer::writeToken(Tokenizer::LEFT_CURLY_TOKEN, Tokenizer::sGlobalDataFile);
            // parseComments(GLOBAL_dataFile);
            break;

        case '}':
            Tokenizer::writeToken(Tokenizer::RIGHT_CURLY_TOKEN, Tokenizer::sGlobalDataFile);
            break;

        case '@':
            Tokenizer::writeToken(Tokenizer::AT_TOKEN, Tokenizer::sGlobalDataFile);
            break;

        case '&':
            Tokenizer::writeToken(Tokenizer::AMPERSAND_TOKEN, Tokenizer::sGlobalDataFile);
            break;

        case '`':
            Tokenizer::writeToken(Tokenizer::BACK_QUOTE_TOKEN, Tokenizer::sGlobalDataFile);
            break;

        case '\\':
            Tokenizer::writeToken(Tokenizer::BACK_SLASH_TOKEN, Tokenizer::sGlobalDataFile);
            break;

        case '|':
            Tokenizer::writeToken(Tokenizer::BAR_TOKEN, Tokenizer::sGlobalDataFile);
            break;

        case ':':
            Tokenizer::writeToken(Tokenizer::COLON_TOKEN, Tokenizer::sGlobalDataFile);
            break;

        case ',':
            Tokenizer::writeToken(Tokenizer::COMMA_TOKEN, Tokenizer::sGlobalDataFile);
            break;

        case '-':
            Tokenizer::writeToken(Tokenizer::DASH_TOKEN, Tokenizer::sGlobalDataFile);
            break;

        case '$':
            Tokenizer::writeToken(Tokenizer::DOLLAR_TOKEN, Tokenizer::sGlobalDataFile);
            break;

        case '=':
            Tokenizer::writeToken(Tokenizer::EQUALS_TOKEN, Tokenizer::sGlobalDataFile);
            break;

        case '!':
            Tokenizer::writeToken(Tokenizer::EXCLAMATION_TOKEN, Tokenizer::sGlobalDataFile);
            break;

        case '#': // Parser doesn't use it, so let's ignore it
            // writeToken (Tokenizer::HASH_TOKEN, GLOBAL_dataFile);
            break;

        case '^':
            Tokenizer::writeToken(Tokenizer::HAT_TOKEN, Tokenizer::sGlobalDataFile);
            break;

        case '<':
            Tokenizer::writeToken(Tokenizer::LEFT_ANGLE_TOKEN, Tokenizer::sGlobalDataFile);
            break;

        case '(':
            Tokenizer::writeToken(Tokenizer::LEFT_PAREN_TOKEN, Tokenizer::sGlobalDataFile);
            break;

        case '[':
            Tokenizer::writeToken(Tokenizer::LEFT_SQUARE_TOKEN, Tokenizer::sGlobalDataFile);
            break;

        case '%':
            Tokenizer::writeToken(Tokenizer::PERCENT_TOKEN, Tokenizer::sGlobalDataFile);
            break;

        case '+':
            Tokenizer::writeToken(Tokenizer::PLUS_TOKEN, Tokenizer::sGlobalDataFile);
            break;

        case '?':
            Tokenizer::writeToken(Tokenizer::QUESTION_TOKEN, Tokenizer::sGlobalDataFile);
            break;

        case '>':
            Tokenizer::writeToken(Tokenizer::RIGHT_ANGLE_TOKEN, Tokenizer::sGlobalDataFile);
            break;

        case ')':
            Tokenizer::writeToken(Tokenizer::RIGHT_PAREN_TOKEN, Tokenizer::sGlobalDataFile);
            break;

        case ']':
            Tokenizer::writeToken(Tokenizer::RIGHT_SQUARE_TOKEN, Tokenizer::sGlobalDataFile);
            break;

        case ';': // Parser doesn't use it, so let's ignore it
            // writeToken (Tokenizer::SEMI_COLON_TOKEN, GLOBAL_dataFile);
            break;

        case '\'':
            Tokenizer::writeToken(Tokenizer::SINGLE_QUOTE_TOKEN, Tokenizer::sGlobalDataFile);
            break;

            // Enable C++ style commenting
        case '/':
            c2 = getc(Tokenizer::sGlobalDataFile->File);
            if (c2 != (int)'/' && c2 != (int)'*') {
                ungetc(c2, Tokenizer::sGlobalDataFile->File);
                Tokenizer::writeToken(Tokenizer::SLASH_TOKEN, Tokenizer::sGlobalDataFile);
                break;
            }
            if (c2 == (int)'*') {
                Tokenizer::sGlobalDataFile->parseCComments();
                break;
            }
            while (c2 != (int)'\n') {
                c2 = getc(Tokenizer::sGlobalDataFile->File);
                if (c2 == EOF) {
                    ungetc(c2, Tokenizer::sGlobalDataFile->File);
                    break;
                }
            }
            Tokenizer::sGlobalDataFile->lineNumber++;
            break;

        case '*':
            Tokenizer::writeToken(Tokenizer::STAR_TOKEN, Tokenizer::sGlobalDataFile);
            break;

        case '~':
            Tokenizer::writeToken(Tokenizer::TILDE_TOKEN, Tokenizer::sGlobalDataFile);
            break;

        case '"':
            Tokenizer::sGlobalDataFile->parseString();
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
            ungetc(c, Tokenizer::sGlobalDataFile->File);
            if (Tokenizer::sGlobalDataFile->readFloat() != true) {
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
            ungetc(c, Tokenizer::sGlobalDataFile->File);
            if (Tokenizer::sGlobalDataFile->readSymbol() != true) {
                return;
            }
            break;
        case '\t':
        case '\r':
        case '\032': // Control Z - EOF on many systems
        case '\0':
            break;

        default:
            {
                char _logMsg[1024];
                snprintf(_logMsg, sizeof(_logMsg), "Error in %s line %d\n", Tokenizer::sGlobalDataFile->Filename,                 Tokenizer::sGlobalDataFile->lineNumber + 1);
                Logger::reportMessage("Tokenizer", Logger::ERROR, "", _logMsg);
            }
            {
                char _logMsg[1024];
                snprintf(_logMsg, sizeof(_logMsg), "Illegal character in input file, value is %02x\n", c);
                Logger::reportMessage("Tokenizer", Logger::ERROR, "", _logMsg);
            }
            break;
        }
        if (Tokenizer::token().tokenId == Tokenizer::INCLUDE_TOKEN) {
            if (Tokenizer::sGlobalDataFile->skipSpaces() != true) {
                Tokenizer::tokenError(
                    Tokenizer::sGlobalDataFile, "Expecting a Tokenizer::sString after INCLUDE\n");
            }

            if ((c = getc(Tokenizer::sGlobalDataFile->File)) != '"') {
                Tokenizer::tokenError(
                    Tokenizer::sGlobalDataFile, "Expecting a Tokenizer::sString after INCLUDE\n");
            }

            Tokenizer::sGlobalDataFile->parseString();
            Tokenizer::sGlobalIncludeFileIndex++;
            if (Tokenizer::sGlobalIncludeFileIndex > Tokenizer::MAX_INCLUDE_FILES) {
                Tokenizer::tokenError(
                    Tokenizer::sGlobalDataFile, "Too many nested include files\n");
            }

            Tokenizer::sGlobalDataFile = &Tokenizer::sGlobalIncludeFiles[Tokenizer::sGlobalIncludeFileIndex];
            Tokenizer::sGlobalDataFile->lineNumber = 0;

            Tokenizer::sGlobalDataFile->Filename =
                new char[strlen(Tokenizer::token().Token_String) + 1];
            if (Tokenizer::sGlobalDataFile->Filename == nullptr) {
                {
                    char _logMsg[1024];
                    snprintf(_logMsg, sizeof(_logMsg), "Out of memory opening include file: %s\n", Tokenizer::token().Token_String);
                    Logger::reportMessage("Tokenizer", Logger::FATAL_ERROR, "", _logMsg);
                }
            }

            strcpy(Tokenizer::sGlobalDataFile->Filename, Tokenizer::token().Token_String);

            if ((Tokenizer::sGlobalDataFile->File = FileLocator::locate(
                     Tokenizer::token().Token_String, "r")) == nullptr) {
                {
                    char _logMsg[1024];
                    snprintf(_logMsg, sizeof(_logMsg), "Cannot open include file: %s\n", Tokenizer::token().Token_String);
                    Logger::reportMessage("Tokenizer", Logger::FATAL_ERROR, "", _logMsg);
                }
            }
            Tokenizer::token().tokenId = Tokenizer::END_OF_FILE_TOKEN;
        }
    }

    Tokenizer::sTokenCount++;
    if (Tokenizer::sTokenCount > 1000) {
        Tokenizer::sTokenCount = 0;
        fprintf(stderr, ".");
        fflush(stderr);
        Tokenizer::sLineCount++;
        if (Tokenizer::sLineCount > 78) {
            Tokenizer::sLineCount = 0;
            fprintf(stderr, "\n");
        }
    }
}

/**
Mark that the token has been put back into the input stream.  The next
call to Get_Token will return the last-read token instead of reading a
new one from the file.
*/

void
Tokenizer::ungetToken()
{
    Tokenizer::token().ungetToken = true;
}

// Skip over spaces in the input file

int
DataFile::skipSpaces()
{
    int c;

    while (true) {
        c = getc(this->File);
        if (c == EOF) {
            return (false);
        }

        if (!(isspace(c) || c == 0x0A)) {
            break;
        }

        if (c == '\n') {
            this->lineNumber++;
        }
    }

    ungetc(c, this->File);
    return (true);
}

/**
The comments on comments are outdated and incorrect.

Comments start with an open brace ({) and end with a close brace (}).
The open brace has been read already.  Continue reading until a close
brace is encountered. Be sure to count the lines while you're at it.
Incidently, nested comments are supported (in case you do such esoteric
things)
*/

int
DataFile::parseComments()
{
    int c;
    bool endOfComment;

    endOfComment = false;
    while (!endOfComment) {
        c = getc(this->File);
        if (c == EOF) {
            Tokenizer::tokenError(Tokenizer::getGlobalDataFile(), "No closing comment found");
            return (false);
        }

        if (c == (int)'\n') {
            this->lineNumber++;
        }

        if (c == (int)'{') {
            if (!this->parseComments()) {
                return (false);
            }
        } else {
            endOfComment = (c == (int)'}');
        }
    }

    return (true);
}

// C style comments with asterik and slash - CdW 8/91

int
DataFile::parseCComments()
{
    int c;
    int c2;
    bool endOfComment;

    endOfComment = false;
    while (!endOfComment) {
        c = getc(this->File);
        if (c == EOF) {
            Tokenizer::tokenError(
                Tokenizer::getGlobalDataFile(), "No */ closing comment found");
            return (false);
        }

        if (c == (int)'\n') {
            this->lineNumber++;
        }

        if (c == (int)'*') {
            c2 = getc(this->File);
            if (c2 != (int)'/') {
                ungetc(c2, this->File);
            } else {
                endOfComment = true;
            }
        }
        // Check for and handle nested comments
        if (c == (int)'/') {
            c2 = getc(this->File);
            if (c2 != (int)'*') {
                ungetc(c2, this->File);
            } else {
                this->parseCComments();
            }
        }
    }
    return (true);
}

/**
The following routines make it easier to handle strings.  They stuff
characters into a Tokenizer::sString buffer one at a time making all the proper
range checks.  Call Begin_String to start, Stuff_Character to put
characters in, and End_String to finish.  The String variable contains
the final Tokenizer::sString.
*/

void
Tokenizer::beginString()
{
    Tokenizer::sStringIndex = 0;
}

void
Tokenizer::stuffCharacter(int c, DataFile *dataFile)
{
    if (Tokenizer::sStringIndex < Tokenizer::MAX_STRING_INDEX) {
        Tokenizer::sString[Tokenizer::sStringIndex++] = (char)c;
        if (Tokenizer::sStringIndex >= Tokenizer::MAX_STRING_INDEX) {
            Tokenizer::tokenError(dataFile, "String too long");
            Tokenizer::sString[Tokenizer::sStringIndex - 1] = '\0';
        }
    }
}

void
DataFile::endString()
{
    Tokenizer::stuffCharacter((int)'\0', Tokenizer::getGlobalDataFile());
}

/**
Read a float from the input file and tokenize it as one token. The phase
variable is 0 for the first character, 1 for all subsequent characters
up to the decimal point, 2 for all characters after the decimal
point, 3 for the E+/- and 4 for the exponent.  This helps to insure
that the number is formatted properly. E format added 9/91 CEY
*/

int
DataFile::readFloat()
{
    int c;
    bool finished;
    int phase;

    finished = false;
    phase = 0;

    Tokenizer::beginString();
    while (!finished) {
        c = getc(this->File);
        if (c == EOF) {
            Tokenizer::tokenError(Tokenizer::getGlobalDataFile(), "Unexpected end of file");
            return (false);
        }

        switch (phase) {
        case 0:
            if (isdigit(c)) {
                Tokenizer::stuffCharacter(c, Tokenizer::getGlobalDataFile());
            } else if (c == '.') {
                Tokenizer::stuffCharacter('0', Tokenizer::getGlobalDataFile());
                ungetc(c, this->File);
            } else {
                Tokenizer::tokenError(
                    Tokenizer::getGlobalDataFile(), "Error in decimal number");
            }
            phase = 1;
            break;

        case 1:
            if (isdigit(c)) {
                Tokenizer::stuffCharacter(c, Tokenizer::getGlobalDataFile());
            } else if (c == (int)'.') {
                Tokenizer::stuffCharacter(c, Tokenizer::getGlobalDataFile());
                phase = 2;
            } else if ((c == 'e') || (c == 'E')) {
                Tokenizer::stuffCharacter(c, Tokenizer::getGlobalDataFile());
                phase = 3;
            } else {
                finished = true;
            }
            break;

        case 2:
            if (isdigit(c)) {
                Tokenizer::stuffCharacter(c, Tokenizer::getGlobalDataFile());
            } else if ((c == 'e') || (c == 'E')) {
                Tokenizer::stuffCharacter(c, Tokenizer::getGlobalDataFile());
                phase = 3;
            } else {
                finished = true;
            }
            break;

        case 3:
            if (isdigit(c) || (c == '+') || (c == '-')) {
                Tokenizer::stuffCharacter(c, Tokenizer::getGlobalDataFile());
                phase = 4;
            } else {
                finished = true;
            }
            break;

        case 4:
            if (isdigit(c)) {
                Tokenizer::stuffCharacter(c, Tokenizer::getGlobalDataFile());
            } else {
                finished = true;
            }
            break;
        }
    }

    ungetc(c, this->File);
    this->endString();

    Tokenizer::writeToken(Tokenizer::FLOAT_TOKEN, Tokenizer::getGlobalDataFile());
    if (sscanf(Tokenizer::getString(), "%lf", &Tokenizer::token().tokenFloat) == 0) {
        return (false);
    }

    return (true);
}

// Parse a Tokenizer::sString from the input file into a token
void
DataFile::parseString()
{
    int c;

    Tokenizer::beginString();
    while (true) {
        c = getc(this->File);
        if (c == EOF) {
            Tokenizer::tokenError(Tokenizer::getGlobalDataFile(), "No end quote for Tokenizer::sString");
        }

        if (c != (int)'"') {
            Tokenizer::stuffCharacter(c, Tokenizer::getGlobalDataFile());
        } else {
            break;
        }
    }
    this->endString();

    Tokenizer::writeToken(Tokenizer::STRING_TOKEN, Tokenizer::getGlobalDataFile());
    Tokenizer::token().Token_String = Tokenizer::getString();
}

/**
Read in a symbol from the input file.  Check to see if it is a reserved
word.  If it is, write out the appropriate token.  Otherwise, write the
symbol out to the Symbol file and write out an IDENTIFIER token. An
Identifier token is a token whose token number is greater than the
highest reserved word.
*/

int
DataFile::readSymbol()
{
    int c;
    int symbolId;

    Tokenizer::beginString();
    while (true) {
        c = getc(this->File);
        if (c == EOF) {
            Tokenizer::tokenError(Tokenizer::getGlobalDataFile(), "Unexpected end of file");
            return (false);
        }

        if (isalpha(c) || isdigit(c) || c == (int)'_') {
            Tokenizer::stuffCharacter(c, Tokenizer::getGlobalDataFile());
        } else {
            ungetc(c, this->File);
            break;
        }
    }
    this->endString();

    // Ignore the symbol if it was meant for the tokenizer (-2)
    if ((symbolId = Tokenizer::findReserved()) != -1 && symbolId != -2) {
        Tokenizer::writeToken(symbolId, Tokenizer::getGlobalDataFile());
    } else {
        // Ignore the symbol if it was meant for the tokenizer (-2)
        if (symbolId == -2) {
            return (true);
        }

        if ((symbolId = Tokenizer::findSymbol()) == -1) {
            if (++Tokenizer::getNumberOfSymbols() < Tokenizer::getMaxSymbols()) {
                if ((Tokenizer::getSymbolTable()[Tokenizer::getNumberOfSymbols()] =
                            new char[strlen(Tokenizer::getString()) + 1]) == nullptr) {
                    Tokenizer::tokenError(Tokenizer::getGlobalDataFile(),
                        "Out of memory. Cannot allocate space for identifier");
                }

                strcpy(Tokenizer::getSymbolTable()[Tokenizer::getNumberOfSymbols()], Tokenizer::getString());
                symbolId = Tokenizer::getNumberOfSymbols();
            } else {
                Logger::reportMessage("Tokenizer", Logger::FATAL_ERROR, "", "\nToo many symbols. Use +ms### option to raise "                     "Max_Symbols.\n");
            }
        }

        Tokenizer::writeToken(Tokenizer::LAST_TOKEN + symbolId, Tokenizer::getGlobalDataFile());
    }

    return (true);
}

/**
Return the index the token in the reserved words table or -1 if it
isn't there.
*/
int
Tokenizer::findReserved()
{
    int i;

    if (Tokenizer::povStricmp("case_sensitive_yes", &(Tokenizer::sString[0])) == 0) {
        Tokenizer::sCaseSensitiveFlag = 0;
        return (-2);
    }
    if (Tokenizer::povStricmp("case_sensitive_no", &(Tokenizer::sString[0])) == 0) {
        Tokenizer::sCaseSensitiveFlag = 1;
        return (-2);
    }
    // The optional case sensitive option only checks keywords unsensitive
    // Symbols can be upper/lower, but not be the same as a keyword
    if (Tokenizer::povStricmp("case_sensitive_opt", &(Tokenizer::sString[0])) == 0) {
        Tokenizer::sCaseSensitiveFlag = 2;
        return (-2);
    }

    for (i = 0; i < Tokenizer::LAST_TOKEN; i++) {
        if (Tokenizer::sCaseSensitiveFlag == 0) {
            if (strcmp(Tokenizer::reservedWords()[i].Token_Name, &(Tokenizer::sString[0])) == 0) {
                return (Tokenizer::reservedWords()[i].tokenNumber);
            }
        } else {
            if (Tokenizer::povStricmp((char *)Tokenizer::reservedWords()[i].Token_Name,
                    &(Tokenizer::sString[0])) == 0) {
                return (Tokenizer::reservedWords()[i].tokenNumber);
            }
        }
    }

    return (-1);
}

/**
Check to see if a symbol already exists with this name.  If so, return
its symbol ID.
*/

int
Tokenizer::findSymbol()
{
    int i;

    for (i = 1; i <= Tokenizer::sNumberOfSymbols; i++) {
        if (Tokenizer::sCaseSensitiveFlag == 0 || Tokenizer::sCaseSensitiveFlag == 2) {
            if (strcmp(Tokenizer::sSymbolTable[i], Tokenizer::sString) == 0) {
                return (i);
            }
            if (Tokenizer::sCaseSensitiveFlag == 1) {
                if (Tokenizer::povStricmp(Tokenizer::sSymbolTable[i], Tokenizer::sString) == 0) {
                    return (i);
                }
            }
        }
    }

    return (-1);
}

// Write a token out to the token file
void
Tokenizer::writeToken(TOKEN tokenId, DataFile *dataFile)
{
    Tokenizer::token().tokenId = tokenId;
    Tokenizer::token().tokenLineNo = dataFile->lineNumber;
    Tokenizer::token().tokenColumnNo = 1;
    Tokenizer::token().Filename = dataFile->Filename;
    Tokenizer::token().Token_String = Tokenizer::sString;

    if (Tokenizer::token().tokenId > Tokenizer::LAST_TOKEN) {
        Tokenizer::token().identifierNumber =
            (int)Tokenizer::token().tokenId - (int)Tokenizer::LAST_TOKEN;
        Tokenizer::token().tokenId = Tokenizer::IDENTIFIER_TOKEN;
    }
}

// Report an error
void
Tokenizer::tokenError(DataFile *dataFile, const char *str)
{
    {
        char _logMsg[1024];
        snprintf(_logMsg, sizeof(_logMsg), "Error in %s line %d\n", dataFile->Filename,         dataFile->lineNumber);
        Logger::reportMessage("Tokenizer", Logger::ERROR, "", _logMsg);
    }
    {
        char _logMsg[1024];
        snprintf(_logMsg, sizeof(_logMsg), "%s\n\n", str);
        Logger::reportMessage("Tokenizer", Logger::FATAL_ERROR, "", _logMsg);
    }
}
/**
Since the stricmp function isn't available on all systems, we've
provided a simplified version of it here.
*/

int
Tokenizer::povStricmp(const char *s1, const char *s2)
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
    }
    return (1);
}
