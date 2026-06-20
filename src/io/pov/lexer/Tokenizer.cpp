/**
tokenize.c

This module implements the first part of a two part parser for the scene
description files.  This phase changes the input file into tokens.
*/

#include <cstdio>
#include <cstring>
#include <cctype>

#include "vsdk/toolkit/common/logging/Logger.h"
#include "io/binaryIo/FileLocator.h"
#include "io/pov/lexer/Tokenizer.h"

/**
This module tokenizes the input file and sends the tokens created
to the parser (the second stage).  Tokens sent to the parser contain a
token ID, the line number of the token, and if necessary, some data for
the token.
*/

Tokenizer::Tokenizer()
    : sToken(),
      maxSymbols(500),
      sString(),
      sStringIndex(0),
      sCaseSensitiveFlag(0),
      sTokenCount(0),
      sLineCount(0),
      sSymbolTable(nullptr),
      sNumberOfSymbols(0),
      sGlobalIncludeFiles(),
      sGlobalDataFile(nullptr),
      sGlobalIncludeFileIndex(0),
      mFileLocator(nullptr)
{
    for (int i = 0; i < MAX_INCLUDE_FILES; i++) {
        sGlobalIncludeFiles[i].setFile(nullptr);
        sGlobalIncludeFiles[i].setFilename(nullptr);
        sGlobalIncludeFiles[i].setTokenizer(this);
        sGlobalIncludeFiles[i].setLineNumber(0);
    }
}

/**
Here are the reserved words.  If you need to add new words, be sure
to declare them in frame.h
*/

const ReservedWord Tokenizer::sReservedWords[Tokenizer::LAST_TOKEN] = {{Tokenizer::AGATE_TOKEN, "agate"},
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
    {Tokenizer::TRANSMIT_TOKEN, "* disabled *"}, // Transmit disabled
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

const ReservedWord *
Tokenizer::reservedWords() const
{
    return sReservedWords;
}

PovToken &
Tokenizer::token()
{
    return sToken;
}

void
Tokenizer::setCaseSensitiveIdentifiers(int mode)
{
    sCaseSensitiveFlag = mode;
}

void
Tokenizer::setMaxSymbols(int value)
{
    maxSymbols = value;
}

int
Tokenizer::getMaxSymbols() const
{
    return maxSymbols;
}

DataFile *
Tokenizer::getGlobalDataFile()
{
    return sGlobalDataFile;
}

char *
Tokenizer::getString()
{
    return sString;
}

int &
Tokenizer::getNumberOfSymbols()
{
    return sNumberOfSymbols;
}

char **
Tokenizer::getSymbolTable()
{
    return sSymbolTable;
}

void
Tokenizer::initializeTokenizer(const char *filename)
{
    sSymbolTable = nullptr;
    sGlobalDataFile = nullptr;

    sGlobalIncludeFileIndex = 0;
    sGlobalDataFile = &sGlobalIncludeFiles[0];

    sGlobalDataFile->setFile(mFileLocator->locate(filename, "r"));
    if (sGlobalDataFile->getFile() == nullptr) {
        Logger::reportMessage("Tokenizer", Logger::FATAL_ERROR, "", "Cannot open input file\n");
    }

    sGlobalDataFile->setFilename(new char[strlen(filename) + 1]);
    strcpy(sGlobalDataFile->getFilename(), filename);
    sGlobalDataFile->setLineNumber(0);

    if ((sSymbolTable = new char *[maxSymbols]) == nullptr) {
        Logger::reportMessage("Tokenizer", Logger::FATAL_ERROR, "", "Out of Memory. Cannot allocate space for symbol table\n");
    }

    token().setEndOfFile(false);
    sNumberOfSymbols = 0;
}

void
Tokenizer::terminateTokenizer()
{
    int i;

    if (sSymbolTable != nullptr) {
        for (i = 1; i <= sNumberOfSymbols; i++) {
            delete[] sSymbolTable[i];
        }

        delete[] sSymbolTable;
    }

    if (sGlobalDataFile != nullptr) {
        fclose(sGlobalDataFile->getFile());
        delete[] sGlobalDataFile->getFilename();
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
    if (token().isUnGetToken()) {
        token().setUnGetToken(false);
        return;
    }

    if (token().isEndOfFile()) {
        return;
    }

    token().setTokenId(Tokenizer::END_OF_FILE_TOKEN);

    while (token().getTokenId() == Tokenizer::END_OF_FILE_TOKEN) {

        sGlobalDataFile->skipSpaces();

        c = getc(sGlobalDataFile->getFile());
        if (c == EOF) {
            if (sGlobalIncludeFileIndex == 0) {
                token().setTokenId(Tokenizer::END_OF_FILE_TOKEN);
                token().setEndOfFile(true);
                // putchar ('\n');
                fprintf(stderr, "\n");
                return;
            }

            fclose(sGlobalDataFile->getFile());
            delete[] sGlobalDataFile->getFilename();
            sGlobalDataFile->setFilename(nullptr);

            sGlobalDataFile = &sGlobalIncludeFiles[--sGlobalIncludeFileIndex];
            continue;
        }

        sString[0] = '\0';
        sStringIndex = 0;

        switch (c) {
        case '\n':
            sGlobalDataFile->incrementLineNumber();
            break;

        case '{':
            writeToken(Tokenizer::LEFT_CURLY_TOKEN, sGlobalDataFile);
            // parseComments(GLOBAL_dataFile);
            break;

        case '}':
            writeToken(Tokenizer::RIGHT_CURLY_TOKEN, sGlobalDataFile);
            break;

        case '@':
            writeToken(Tokenizer::AT_TOKEN, sGlobalDataFile);
            break;

        case '&':
            writeToken(Tokenizer::AMPERSAND_TOKEN, sGlobalDataFile);
            break;

        case '`':
            writeToken(Tokenizer::BACK_QUOTE_TOKEN, sGlobalDataFile);
            break;

        case '\\':
            writeToken(Tokenizer::BACK_SLASH_TOKEN, sGlobalDataFile);
            break;

        case '|':
            writeToken(Tokenizer::BAR_TOKEN, sGlobalDataFile);
            break;

        case ':':
            writeToken(Tokenizer::COLON_TOKEN, sGlobalDataFile);
            break;

        case ',':
            writeToken(Tokenizer::COMMA_TOKEN, sGlobalDataFile);
            break;

        case '-':
            writeToken(Tokenizer::DASH_TOKEN, sGlobalDataFile);
            break;

        case '$':
            writeToken(Tokenizer::DOLLAR_TOKEN, sGlobalDataFile);
            break;

        case '=':
            writeToken(Tokenizer::EQUALS_TOKEN, sGlobalDataFile);
            break;

        case '!':
            writeToken(Tokenizer::EXCLAMATION_TOKEN, sGlobalDataFile);
            break;

        case '#': // Parser doesn't use it, so let's ignore it
            // writeToken (Tokenizer::HASH_TOKEN, GLOBAL_dataFile);
            break;

        case '^':
            writeToken(Tokenizer::HAT_TOKEN, sGlobalDataFile);
            break;

        case '<':
            writeToken(Tokenizer::LEFT_ANGLE_TOKEN, sGlobalDataFile);
            break;

        case '(':
            writeToken(Tokenizer::LEFT_PAREN_TOKEN, sGlobalDataFile);
            break;

        case '[':
            writeToken(Tokenizer::LEFT_SQUARE_TOKEN, sGlobalDataFile);
            break;

        case '%':
            writeToken(Tokenizer::PERCENT_TOKEN, sGlobalDataFile);
            break;

        case '+':
            writeToken(Tokenizer::PLUS_TOKEN, sGlobalDataFile);
            break;

        case '?':
            writeToken(Tokenizer::QUESTION_TOKEN, sGlobalDataFile);
            break;

        case '>':
            writeToken(Tokenizer::RIGHT_ANGLE_TOKEN, sGlobalDataFile);
            break;

        case ')':
            writeToken(Tokenizer::RIGHT_PAREN_TOKEN, sGlobalDataFile);
            break;

        case ']':
            writeToken(Tokenizer::RIGHT_SQUARE_TOKEN, sGlobalDataFile);
            break;

        case ';': // Parser doesn't use it, so let's ignore it
            // writeToken (Tokenizer::SEMI_COLON_TOKEN, GLOBAL_dataFile);
            break;

        case '\'':
            writeToken(Tokenizer::SINGLE_QUOTE_TOKEN, sGlobalDataFile);
            break;

            // Enable C++ style commenting
        case '/':
            c2 = getc(sGlobalDataFile->getFile());
            if (c2 != (int)'/' && c2 != (int)'*') {
                ungetc(c2, sGlobalDataFile->getFile());
                writeToken(Tokenizer::SLASH_TOKEN, sGlobalDataFile);
                break;
            }
            if (c2 == (int)'*') {
                sGlobalDataFile->parseCComments();
                break;
            }
            while (c2 != (int)'\n') {
                c2 = getc(sGlobalDataFile->getFile());
                if (c2 == EOF) {
                    ungetc(c2, sGlobalDataFile->getFile());
                    break;
                }
            }
            sGlobalDataFile->incrementLineNumber();
            break;

        case '*':
            writeToken(Tokenizer::STAR_TOKEN, sGlobalDataFile);
            break;

        case '~':
            writeToken(Tokenizer::TILDE_TOKEN, sGlobalDataFile);
            break;

        case '"':
            sGlobalDataFile->parseString();
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
            ungetc(c, sGlobalDataFile->getFile());
            if (sGlobalDataFile->readFloat() != true) {
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
            ungetc(c, sGlobalDataFile->getFile());
            if (sGlobalDataFile->readSymbol() != true) {
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
                snprintf(_logMsg, sizeof(_logMsg), "Error in %s line %d\n", sGlobalDataFile->getFilename(),                 sGlobalDataFile->getLineNumber() + 1);
                Logger::reportMessage("Tokenizer", Logger::ERROR, "", _logMsg);
            }
            {
                char _logMsg[1024];
                snprintf(_logMsg, sizeof(_logMsg), "Illegal character in input file, value is %02x\n", c);
                Logger::reportMessage("Tokenizer", Logger::ERROR, "", _logMsg);
            }
            break;
        }
        if (token().getTokenId() == Tokenizer::INCLUDE_TOKEN) {
            if (sGlobalDataFile->skipSpaces() != true) {
                tokenError(
                    sGlobalDataFile, "Expecting a sString after INCLUDE\n");
            }

            if ((c = getc(sGlobalDataFile->getFile())) != '"') {
                tokenError(
                    sGlobalDataFile, "Expecting a sString after INCLUDE\n");
            }

            sGlobalDataFile->parseString();
            sGlobalIncludeFileIndex++;
            if (sGlobalIncludeFileIndex > Tokenizer::MAX_INCLUDE_FILES) {
                tokenError(
                    sGlobalDataFile, "Too many nested include files\n");
            }

            sGlobalDataFile = &sGlobalIncludeFiles[sGlobalIncludeFileIndex];
            sGlobalDataFile->setLineNumber(0);

            sGlobalDataFile->setFilename(
                new char[strlen(token().getTokenString()) + 1]);
            if (sGlobalDataFile->getFilename() == nullptr) {
                {
                    char _logMsg[1024];
                    snprintf(_logMsg, sizeof(_logMsg), "Out of memory opening include file: %s\n", token().getTokenString());
                    Logger::reportMessage("Tokenizer", Logger::FATAL_ERROR, "", _logMsg);
                }
            }

            strcpy(sGlobalDataFile->getFilename(), token().getTokenString());

            sGlobalDataFile->setFile(mFileLocator->locate(
                token().getTokenString(), "r"));
            if (sGlobalDataFile->getFile() == nullptr) {
                {
                    char _logMsg[1024];
                    snprintf(_logMsg, sizeof(_logMsg), "Cannot open include file: %s\n", token().getTokenString());
                    Logger::reportMessage("Tokenizer", Logger::FATAL_ERROR, "", _logMsg);
                }
            }
            token().setTokenId(Tokenizer::END_OF_FILE_TOKEN);
        }
    }

    sTokenCount++;
    if (sTokenCount > 1000) {
        sTokenCount = 0;
        fprintf(stderr, ".");
        fflush(stderr);
        sLineCount++;
        if (sLineCount > 78) {
            sLineCount = 0;
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
    token().setUnGetToken(true);
}

// Skip over spaces in the input file

int
DataFile::skipSpaces()
{
    int c;

    while (true) {
        c = getc(this->getFile());
        if (c == EOF) {
            return (false);
        }

        if (!(isspace(c) || c == 0x0A)) {
            break;
        }

        if (c == '\n') {
            this->incrementLineNumber();
        }
    }

    ungetc(c, this->getFile());
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
        c = getc(this->getFile());
        if (c == EOF) {
            getTokenizer()->tokenError(getTokenizer()->getGlobalDataFile(), "No closing comment found");
            return (false);
        }

        if (c == (int)'\n') {
            this->incrementLineNumber();
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

// C style comments with asterik and slash

int
DataFile::parseCComments()
{
    int c;
    int c2;
    bool endOfComment;

    endOfComment = false;
    while (!endOfComment) {
        c = getc(this->getFile());
        if (c == EOF) {
            getTokenizer()->tokenError(
                getTokenizer()->getGlobalDataFile(), "No */ closing comment found");
            return (false);
        }

        if (c == (int)'\n') {
            this->incrementLineNumber();
        }

        if (c == (int)'*') {
            c2 = getc(this->getFile());
            if (c2 != (int)'/') {
                ungetc(c2, this->getFile());
            } else {
                endOfComment = true;
            }
        }
        // Check for and handle nested comments
        if (c == (int)'/') {
            c2 = getc(this->getFile());
            if (c2 != (int)'*') {
                ungetc(c2, this->getFile());
            } else {
                this->parseCComments();
            }
        }
    }
    return (true);
}

/**
The following routines make it easier to handle strings.  They stuff
characters into a sString buffer one at a time making all the proper
range checks.  Call Begin_String to start, Stuff_Character to put
characters in, and End_String to finish.  The String variable contains
the final sString.
*/

void
Tokenizer::beginString()
{
    sStringIndex = 0;
}

void
Tokenizer::stuffCharacter(int c, const DataFile *dataFile)
{
    if (sStringIndex < Tokenizer::MAX_STRING_INDEX) {
        sString[sStringIndex++] = (char)c;
        if (sStringIndex >= Tokenizer::MAX_STRING_INDEX) {
            tokenError(dataFile, "String too long");
            sString[sStringIndex - 1] = '\0';
        }
    }
}

void
DataFile::endString()
{
    getTokenizer()->stuffCharacter((int)'\0', getTokenizer()->getGlobalDataFile());
}

/**
Read a float from the input file and tokenize it as one token. The phase
variable is 0 for the first character, 1 for all subsequent characters
up to the decimal point, 2 for all characters after the decimal
point, 3 for the E+/- and 4 for the exponent.  This helps to insure
that the number is formatted properly.
*/

int
DataFile::readFloat()
{
    int c;
    bool finished;
    int phase;

    finished = false;
    phase = 0;

    getTokenizer()->beginString();
    while (!finished) {
        c = getc(this->getFile());
        if (c == EOF) {
            getTokenizer()->tokenError(getTokenizer()->getGlobalDataFile(), "Unexpected end of file");
            return (false);
        }

        switch (phase) {
        case 0:
            if (isdigit(c)) {
                getTokenizer()->stuffCharacter(c, getTokenizer()->getGlobalDataFile());
            } else if (c == '.') {
                getTokenizer()->stuffCharacter('0', getTokenizer()->getGlobalDataFile());
                ungetc(c, this->getFile());
            } else {
                getTokenizer()->tokenError(
                    getTokenizer()->getGlobalDataFile(), "Error in decimal number");
            }
            phase = 1;
            break;

        case 1:
            if (isdigit(c)) {
                getTokenizer()->stuffCharacter(c, getTokenizer()->getGlobalDataFile());
            } else if (c == (int)'.') {
                getTokenizer()->stuffCharacter(c, getTokenizer()->getGlobalDataFile());
                phase = 2;
            } else if ((c == 'e') || (c == 'E')) {
                getTokenizer()->stuffCharacter(c, getTokenizer()->getGlobalDataFile());
                phase = 3;
            } else {
                finished = true;
            }
            break;

        case 2:
            if (isdigit(c)) {
                getTokenizer()->stuffCharacter(c, getTokenizer()->getGlobalDataFile());
            } else if ((c == 'e') || (c == 'E')) {
                getTokenizer()->stuffCharacter(c, getTokenizer()->getGlobalDataFile());
                phase = 3;
            } else {
                finished = true;
            }
            break;

        case 3:
            if (isdigit(c) || (c == '+') || (c == '-')) {
                getTokenizer()->stuffCharacter(c, getTokenizer()->getGlobalDataFile());
                phase = 4;
            } else {
                finished = true;
            }
            break;

        case 4:
            if (isdigit(c)) {
                getTokenizer()->stuffCharacter(c, getTokenizer()->getGlobalDataFile());
            } else {
                finished = true;
            }
            break;
        }
    }

    ungetc(c, this->getFile());
    this->endString();

    getTokenizer()->writeToken(Tokenizer::FLOAT_TOKEN, getTokenizer()->getGlobalDataFile());
    double tokenFloat = 0.0;
    if (sscanf(getTokenizer()->getString(), "%lf", &tokenFloat) == 0) {
        return (false);
    }
    getTokenizer()->token().setTokenFloat(tokenFloat);

    return (true);
}

// Parse a sString from the input file into a token
void
DataFile::parseString()
{
    int c;

    getTokenizer()->beginString();
    while (true) {
        c = getc(this->getFile());
        if (c == EOF) {
            getTokenizer()->tokenError(getTokenizer()->getGlobalDataFile(), "No end quote for sString");
        }

        if (c != (int)'"') {
            getTokenizer()->stuffCharacter(c, getTokenizer()->getGlobalDataFile());
        } else {
            break;
        }
    }
    this->endString();

    getTokenizer()->writeToken(Tokenizer::STRING_TOKEN, getTokenizer()->getGlobalDataFile());
    getTokenizer()->token().setTokenString(getTokenizer()->getString());
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

    getTokenizer()->beginString();
    while (true) {
        c = getc(this->getFile());
        if (c == EOF) {
            getTokenizer()->tokenError(getTokenizer()->getGlobalDataFile(), "Unexpected end of file");
            return (false);
        }

        if (isalpha(c) || isdigit(c) || c == (int)'_') {
            getTokenizer()->stuffCharacter(c, getTokenizer()->getGlobalDataFile());
        } else {
            ungetc(c, this->getFile());
            break;
        }
    }
    this->endString();

    // Ignore the symbol if it was meant for the tokenizer (-2)
    if ((symbolId = getTokenizer()->findReserved()) != -1 && symbolId != -2) {
        getTokenizer()->writeToken(symbolId, getTokenizer()->getGlobalDataFile());
    } else {
        // Ignore the symbol if it was meant for the tokenizer (-2)
        if (symbolId == -2) {
            return (true);
        }

        if ((symbolId = getTokenizer()->findSymbol()) == -1) {
            if (++getTokenizer()->getNumberOfSymbols() < getTokenizer()->getMaxSymbols()) {
                if ((getTokenizer()->getSymbolTable()[getTokenizer()->getNumberOfSymbols()] =
                            new char[strlen(getTokenizer()->getString()) + 1]) == nullptr) {
                    getTokenizer()->tokenError(getTokenizer()->getGlobalDataFile(),
                        "Out of memory. Cannot allocate space for identifier");
                }

                strcpy(getTokenizer()->getSymbolTable()[getTokenizer()->getNumberOfSymbols()],
                    getTokenizer()->getString());
                symbolId = getTokenizer()->getNumberOfSymbols();
            } else {
                Logger::reportMessage("Tokenizer", Logger::FATAL_ERROR, "", "\nToo many symbols. Use +ms### option to raise "                     "Max_Symbols.\n");
            }
        }

        getTokenizer()->writeToken(Tokenizer::LAST_TOKEN + symbolId, getTokenizer()->getGlobalDataFile());
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

    if (povStricmp("case_sensitive_yes", &(sString[0])) == 0) {
        sCaseSensitiveFlag = 0;
        return (-2);
    }
    if (povStricmp("case_sensitive_no", &(sString[0])) == 0) {
        sCaseSensitiveFlag = 1;
        return (-2);
    }
    // The optional case sensitive option only checks keywords unsensitive
    // Symbols can be upper/lower, but not be the same as a keyword
    if (povStricmp("case_sensitive_opt", &(sString[0])) == 0) {
        sCaseSensitiveFlag = 2;
        return (-2);
    }

    for (i = 0; i < Tokenizer::LAST_TOKEN; i++) {
        if (sCaseSensitiveFlag == 0) {
            if (strcmp(reservedWords()[i].getTokenName(), &(sString[0])) == 0) {
                return (reservedWords()[i].getTokenNumber());
            }
        } else {
            if (povStricmp(reservedWords()[i].getTokenName(),
                    &(sString[0])) == 0) {
                return (reservedWords()[i].getTokenNumber());
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

    for (i = 1; i <= sNumberOfSymbols; i++) {
        if (sCaseSensitiveFlag == 0 || sCaseSensitiveFlag == 2) {
            if (strcmp(sSymbolTable[i], sString) == 0) {
                return (i);
            }
            if (sCaseSensitiveFlag == 1) {
                if (povStricmp(sSymbolTable[i], sString) == 0) {
                    return (i);
                }
            }
        }
    }

    return (-1);
}

// Write a token out to the token file
void
Tokenizer::writeToken(TOKEN tokenId, const DataFile *dataFile)
{
    token().setTokenId(tokenId);
    token().setTokenLineNumber(dataFile->getLineNumber());
    token().setTokenColumnNumber(1);
    token().setFileName(dataFile->getFilename());
    token().setTokenString(sString);

    if (token().getTokenId() > Tokenizer::LAST_TOKEN) {
        token().setIdentifierNumber(
            (int)token().getTokenId() - (int)Tokenizer::LAST_TOKEN);
        token().setTokenId(Tokenizer::IDENTIFIER_TOKEN);
    }
}

// Report an error
void
Tokenizer::tokenError(const DataFile *dataFile, const char *str)
{
    {
        char _logMsg[1024];
        snprintf(_logMsg, sizeof(_logMsg), "Error in %s line %d\n", dataFile->getFilename(),         dataFile->getLineNumber());
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
Tokenizer::povStricmp(const char *s1, const char *s2) const
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
