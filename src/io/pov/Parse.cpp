/****************************************************************************
 *                     parse.c
 *
 *  This module implements a parser for the scene description files.
 *
 *****************************************************************************/

#include "io/pov/Parse.h"
#include "app/PovApp.h"
#include "common/FrameConfig.h"
#include "common/Transformation.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "io/DumpFormat.h"
#include "io/GifFormat.h"
#include "io/IffFormat.h"
#include "io/TargaFormat.h"
#include "render/RenderEngine.h"
#include "render/RenderFrame.h"

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
