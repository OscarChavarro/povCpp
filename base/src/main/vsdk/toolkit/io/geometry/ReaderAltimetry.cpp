#include <cstdio>
#include <cstdlib>

#include "java/lang/Double.h"
#include "java/lang/String.h"
#include "vsdk/toolkit/common/VSDKFatalException.h"
#include "vsdk/toolkit/environment/geometry/volume/HeightFieldSolid.h"
#include "vsdk/toolkit/io/geometry/ReaderAltimetry.h"

// Next whitespace separated token, skipping comments; false at end of file.
static bool readToken(FILE* file, char* token, int capacity)
{
    int c = fgetc(file);
    while ( c != EOF ) {
        if ( c == '#' ) {
            while ( c != EOF && c != '\n' ) c = fgetc(file);
        }
        else if ( c == ' ' || c == '\t' || c == '\r' || c == '\n' ) {
            c = fgetc(file);
        }
        else {
            break;
        }
    }
    if ( c == EOF ) return false;
    int length = 0;
    while ( c != EOF && c != ' ' && c != '\t' && c != '\r' && c != '\n' && c != '#' ) {
        if ( length + 1 >= capacity ) return false;
        token[length++] = (char)c;
        c = fgetc(file);
    }
    token[length] = '\0';
    if ( c == '#' ) ungetc(c, file);
    return true;
}

static bool parseNumber(const char* token, double& value)
{
    char* end = nullptr;
    value = strtod(token, &end);
    return end != token && *end == '\0' && java::Double::isFinite(value);
}

HeightFieldSolid* ReaderAltimetry::importHeightField(const char* fileName)
{
    FILE* file = fopen(fileName, "r");
    if ( !file ) {
        throw VSDKFatalException("ReaderAltimetry: could not open file");
    }
    char token[128];
    double columns = 0;
    double rows = 0;
    if ( !readToken(file, token, sizeof(token)) || java::String(token) != "altimetry" ||
         !readToken(file, token, sizeof(token)) || !parseNumber(token, columns) ||
         !readToken(file, token, sizeof(token)) || !parseNumber(token, rows) ||
         columns != (double)(long)columns || rows != (double)(long)rows ||
         columns < 2 || rows < 2 || columns*rows > 1.0e8 ) {
        fclose(file);
        throw VSDKFatalException("ReaderAltimetry: header must be \"altimetry <columns> <rows>\" with counts of at least 2");
    }
    const long columnCount = (long)columns;
    const long rowCount = (long)rows;
    HeightFieldSolid* solid = new HeightFieldSolid(columnCount, rowCount,
        Vector3Dd(0, 0, 0), Vector3Dd((double)(columnCount - 1), 1, (double)(rowCount - 1)));
    double lowest = java::Double::POSITIVE_INFINITY;
    double highest = java::Double::NEGATIVE_INFINITY;
    for ( long z = 0; z < rowCount; z++ ) {
        for ( long x = 0; x < columnCount; x++ ) {
            double height;
            if ( !readToken(file, token, sizeof(token)) || !parseNumber(token, height) ) {
                fclose(file);
                delete solid;
                throw VSDKFatalException("ReaderAltimetry: missing or invalid height");
            }
            solid->setHeight(x, z, height);
            if ( height < lowest ) lowest = height;
            if ( height > highest ) highest = height;
        }
    }
    const bool extra = readToken(file, token, sizeof(token));
    fclose(file);
    if ( extra ) {
        delete solid;
        throw VSDKFatalException("ReaderAltimetry: more heights than columns*rows");
    }
    if ( !(highest > lowest) ) highest = lowest + 1.0;
    solid->setBounds(Vector3Dd(0, lowest, 0),
        Vector3Dd((double)(columnCount - 1), highest, (double)(rowCount - 1)));
    solid->updateBlocks();
    return solid;
}
