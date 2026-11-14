#ifndef __BLOB_PARSER__
#define __BLOB_PARSER__

#include "io/pov/context/ParserContext.h"
#include "vsdk/toolkit/environment/geometry/volume/BlobSolid.h"

class BlobParser {
  public:
    static SimpleBodyBuilder *parseBlob();
    static SimpleBodyBuilder *parseBlob(ParserContext &ctx);

  private:
    static SimpleBodyBuilder *rebuildBodyWithGeometry(SimpleBodyBuilder *body, Geometry *geometry);
    // A copy of the blob that solves its rays with Sturm sequences.
    static BlobSolid *copyWithSturmSequences(const BlobSolid &solid);
};

#endif
