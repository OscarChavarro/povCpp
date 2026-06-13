#ifndef __BLOB_PARSER_H__
#define __BLOB_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class BlobParser {
  public:
    static Geometry *parseBlob();
    static Geometry *parseBlob(ParserContext &ctx);
};

#endif
