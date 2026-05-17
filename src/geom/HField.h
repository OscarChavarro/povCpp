#ifndef __HFIELD_H__
#define __HFIELD_H__

#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Boxes.h"

static constexpr int GIF = 0; /* These are the types of image maps which can be used as a */
static constexpr int POT = 1; /* height field. */
static constexpr int TGA = 2;

#include "geom/HeightField.h"
#include "geom/HeightFieldBlock.h"

extern Methods Height_Field_Methods;
extern HeightField *getHeightFieldShape(void);

#endif
