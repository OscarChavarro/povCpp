#ifndef __BLOB_PARSER_H__
#define __BLOB_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class TranslatedBody;

class BlobParser {
  public:
    static TranslatedBody *parseBlob();
    static TranslatedBody *parseBlob(ParserContext &ctx);
};

#endif
