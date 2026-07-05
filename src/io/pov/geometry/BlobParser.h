#ifndef __BLOB_PARSER__
#define __BLOB_PARSER__

#include "io/pov/context/ParserContext.h"

class BlobParser {
  public:
    static SimpleBodyBuilder *parseBlob();
    static SimpleBodyBuilder *parseBlob(ParserContext &ctx);

  private:
    static SimpleBodyBuilder *rebuildBodyWithGeometry(SimpleBodyBuilder *body, Geometry *geometry);
};

#endif
