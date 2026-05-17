#ifndef __BEZIER_H__
#define __BEZIER_H__

#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Geometry.h"

static constexpr int BEZIER_INTERIOR_NODE = 0;
static constexpr int BEZIER_LEAF_NODE = 1;

class BezierVertices;
class BezierNode;
#include "geom/BezierChild.h"
#include "geom/BezierNode.h"
#include "geom/BezierVertices.h"
#include "geom/BicubicPatch.h"

extern Methods Bicubic_Patch_Methods;

#endif
