#ifndef __BLOB_PARSER_H__
#define __BLOB_PARSER_H__

class Geometry;
class ParserContext;

class BlobParser {
  public:
    static Geometry *parseBlob();
    static Geometry *parseBlob(ParserContext &ctx);
};

#endif
