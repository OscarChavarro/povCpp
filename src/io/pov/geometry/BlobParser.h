#ifndef __BLOB_PARSER__
#define __BLOB_PARSER__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class SimpleBody;

class BlobParser {
  public:
    static SimpleBody *parseBlob();
    static SimpleBody *parseBlob(ParserContext &ctx);

  private:
    static SimpleBody *rebuildBodyWithGeometry(SimpleBody *body, Geometry *geometry);
};

#endif
