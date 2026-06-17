#ifndef __BLOB_PARSER_H__
#define __BLOB_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class SimpleBody;

class BlobParser {
  public:
    static SimpleBody *parseBlob();
    static SimpleBody *parseBlob(ParserContext &ctx);
};

#endif
