#ifndef __BLOB_PARSER__
#define __BLOB_PARSER__

#include "environment/geometry/Geometry.h"
#include "io/pov/geometry/SimpleBodyBuilder.h"
#include "io/pov/context/ParserContext.h"

class BlobParser {
  public:
    static SimpleBodyBuilder *parseBlob();
    static SimpleBodyBuilder *parseBlob(ParserContext &ctx);

  private:
    static SimpleBodyBuilder *rebuildBodyWithGeometry(SimpleBodyBuilder *body, Geometry *geometry);
};

#endif
