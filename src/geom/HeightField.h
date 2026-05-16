#ifndef __HEIGHT_FIELD_H__
#define __HEIGHT_FIELD_H__

#include "common/Frame.h"
#include "geom/Boxes.h"
#include "geom/HeightFieldBlock.h"

class HeightField : public Geometry {
  public:
    Transformation *transformation;
    Box *bounding_box;
    DBL Block_Size;
    DBL Inv_Blk_Size;
    HeightFieldBlock **Block;
    float **Map;
};

#endif
