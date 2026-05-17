#ifndef __BEZIER_H__
#define __BEZIER_H__

#include "common/FrameConfig.h"
#include "app/PovApp.h"
#include "common/Vector3D.h"
#include "geom/GeometryOps.h"

static constexpr int BEZIER_INTERIOR_NODE = 0;
static constexpr int BEZIER_LEAF_NODE = 1;

#include "geom/BezierChild.h"
#include "geom/BezierIntersection.h"
#include "geom/BezierPatches.h"
#include "geom/BezierNode.h"
#include "geom/BezierVertices.h"
#include "geom/BicubicPatch.h"

extern Methods Bicubic_Patch_Methods;

#endif
