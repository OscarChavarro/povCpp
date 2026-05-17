#include "app/PovApp.h"
#include "common/FrameConfig.h"
#include "common/Transformation.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "io/DumpFormat.h"
#include "io/GifFormat.h"
#include "io/IffFormat.h"
#include "io/TargaFormat.h"
#include "io/pov/Parse.h"
#include "render/RenderEngine.h"

#include "environment/camera/Viewpoint.h"
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

char *
ParseErrorReporter::getTokenString(TOKEN tokenId)
{
    int i;

    for (i = 0; i < LAST_TOKEN; i++) {
        if (globalReservedWords[i].Token_Number == tokenId) {
            return (char *)(globalReservedWords[i].Token_Name);
        }
    }
    return (char *)("");
}

void
ParseErrorReporter::parseError(TOKEN tokenId)
{
    char *expected;
    char *found;
    FILE *statFile;
    fprintf(stderr, "Error in file %s line %d\n", globalToken.Filename,
        globalToken.Token_Line_No + 1);
    expected = ParseErrorReporter::getTokenString(tokenId);
    found = ParseErrorReporter::getTokenString(globalToken.Token_Id);
    fprintf(stderr, "%s expected but %s found instead\n", expected, found);
    if (Options & VERBOSE_FILE) {
        statFile = fopen(statFileName, "w+t");
        fprintf(
            statFile, "%s expected but %s found instead\n", expected, found);
        fclose(statFile);
    }

    exit(1);
}

void
ParseErrorReporter::typeError()
{
    FILE *statFile;
    fprintf(stderr, "Error in file %s line %d\n", globalToken.Filename,
        globalToken.Token_Line_No + 1);
    fprintf(
        stderr, "Identifier %s is the wrong type\n", globalToken.Token_String);
    if (Options & VERBOSE_FILE) {
        statFile = fopen(statFileName, "w+t");
        fprintf(statFile, "Error in file %s line %d\n", globalToken.Filename,
            globalToken.Token_Line_No + 1);
        fprintf(statFile, "Identifier %s is the wrong type\n",
            globalToken.Token_String);

        fclose(statFile);
    }

    exit(1);
}

void
ParseErrorReporter::Undeclared()
{
    FILE *statFile;
    fprintf(stderr, "Error in file %s line %d\n", globalToken.Filename,
        globalToken.Token_Line_No + 1);
    fprintf(stderr, "Undeclared identifier %s\n", globalToken.Token_String);
    if (Options & VERBOSE_FILE) {
        statFile = fopen(statFileName, "w+t");
        fprintf(statFile, "Error in file %s line %d\n", globalToken.Filename,
            globalToken.Token_Line_No + 1);
        fprintf(
            statFile, "Undeclared identifier %s\n", globalToken.Token_String);
        fclose(statFile);
    }

    exit(1);
}

void
ParseErrorReporter::Error(const char *str)
{
    FILE *statFile;
    fprintf(stderr, "Error in file %s line %d\n", globalToken.Filename,
        globalToken.Token_Line_No + 1);
    fprintf(stderr, "%s\n", str);

    if (Options & VERBOSE_FILE) {
        statFile = fopen(statFileName, "w+t");
        fprintf(statFile, "Error in file %s line %d\n", globalToken.Filename,
            globalToken.Token_Line_No + 1);
        fprintf(statFile, "%s\n", str);
        fclose(statFile);
    }
    exit(1);
}
