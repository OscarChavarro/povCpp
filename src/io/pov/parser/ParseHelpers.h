#ifndef __PARSE_HELPERS__
#define __PARSE_HELPERS__


class ParseHelpers {
  public:
    static void getExpectedToken(int tokenId);
    static void getExpectedToken(int tokenId, ParserContext &ctx);
};

#endif
