#ifndef __BLOB_PARSER__
#define __BLOB_PARSER__

#include "environment/geometry/TransformedGeometry.h"
#include "environment/scene/SimpleBody.h"
#include "io/pov/context/ParserContext.h"

class BlobParser {
  public:
    static SimpleBody *parseBlob();
    static SimpleBody *parseBlob(ParserContext &ctx);

  private:
    static SimpleBody *rebuildBodyWithGeometry(SimpleBody *body, TransformedGeometry *geometry);
};

#endif
