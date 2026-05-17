#ifndef __BEZIER_H__
#define __BEZIER_H__

#include "common/FrameConfig.h"
#include "app/PovApp.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "geom/GeometryOperations.h"

static constexpr int BEZIER_INTERIOR_NODE = 0;
static constexpr int BEZIER_LEAF_NODE = 1;

#include "geom/BezierChild.h"
#include "geom/BezierIntersection.h"
#include "geom/BezierPatch.h"
#include "geom/BezierNode.h"
#include "geom/BezierVertices.h"
#include "geom/BicubicPatch.h"

extern Methods Bicubic_Patch_Methods;

#endif
