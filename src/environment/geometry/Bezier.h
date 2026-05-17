#ifndef __BEZIER_H__
#define __BEZIER_H__

#include "common/FrameConfig.h"
#include "app/PovApp.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"

static constexpr int BEZIER_INTERIOR_NODE = 0;
static constexpr int BEZIER_LEAF_NODE = 1;

#include "environment/geometry/BezierChild.h"
#include "environment/geometry/BezierIntersection.h"
#include "environment/geometry/BezierPatch.h"
#include "environment/geometry/BezierNode.h"
#include "environment/geometry/BezierVertices.h"
#include "environment/geometry/BicubicPatch.h"

extern Methods Bicubic_Patch_Methods;

#endif
