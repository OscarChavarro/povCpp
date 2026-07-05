
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "java/util/PriorityQueue.txx"



void
ParseHelpers::getExpectedToken(int tokenId)
{
    ParserContext ctx;
    ParseHelpers::getExpectedToken(tokenId, ctx);
}

void
ParseHelpers::getExpectedToken(int tokenId, ParserContext &ctx)
{
    ctx.tokenStream().getToken();
    if (ctx.token().getTokenId() != tokenId) {
        ParseErrorReporter::parseError(tokenId, ctx);
    }
}
