/****************************************************************************
 *                     parse.c
 *
 *  This module implements a parser for the scene description files.
 *
 *****************************************************************************/

#include "io/pov/Parse.h"
#include "common/FrameConfig.h"
#include "common/Transformation.h"
#include "app/PovApp.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "io/GifFormat.h"
#include "io/IffFormat.h"
#include "io/TargaFormat.h"
#include "io/DumpFormat.h"
#include "render/RenderEngine.h"
#include "render/RenderFrame.h"

#include "geom/Bezier.h"
#include "geom/Blob.h"
#include "geom/Box.h"
#include "geom/CSG.h"
#include "geom/HeightField.h"
#include "geom/Light.h"
#include "geom/Composite.h"
#include "geom/InfinitePlane.h"
#include "geom/PolynomialShape.h"
#include "geom/Quadric.h"
#include "geom/Sphere.h"
#include "geom/Triangle.h"
#include "geom/Viewpoint.h"

extern ReservedWord globalReservedWords[];
extern double antialiasThreshold;
extern double maxTraceLevel;
extern char verboseFormat;
extern unsigned int Options;
extern char statFileName[FILE_NAME_LENGTH];

extern int termCounts[MAX_ORDER + 1];
extern TokenStruct globalToken;

RenderFrame *parsingFramePtr;

RGBAColorPaletteSpan *constructionMap = nullptr;

Constant constants[MAX_CONSTANTS];
int numberOfConstants;
int degenerateTriangles;
