#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include "environment/TransformableElement.h"
#include "environment/geometry/GeometryConstants.h"

// Pure geometric element: intersection math only. Material, colour and the
// transform/material bookkeeping now live on TranslatedBody (scene layer).
class Geometry : public TransformableElement {
};

#endif
