#include "io/pov/ast/AstSceneParser.h"

#include "io/Tokenizer.h"
#include "io/pov/ParseErrorReporter.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/ParserContext.h"
#include "io/pov/ast/AstNodes.h"
#include "io/pov/ast/AstObjectParser.h"

typedef AstNode *(*RootNodeParseFn)(ParserContext &ctx);

static AstNode *parseSphereNode(ParserContext &ctx) { return AstObjectParser::parseSphere(ctx); }
static AstNode *parseLightNode(ParserContext &ctx) { return AstObjectParser::parseLightSource(ctx); }
static AstNode *parseUnionNode(ParserContext &ctx) { return AstObjectParser::parseCsg(ctx, AST_CSG_UNION); }
static AstNode *parseIntersectionNode(ParserContext &ctx) { return AstObjectParser::parseCsg(ctx, AST_CSG_INTERSECTION); }
static AstNode *parseDifferenceNode(ParserContext &ctx) { return AstObjectParser::parseCsg(ctx, AST_CSG_DIFFERENCE); }
static AstNode *parseObjectNode(ParserContext &ctx) { return AstObjectParser::parseObject(ctx); }
static AstNode *parseCompositeNode(ParserContext &ctx) { return AstObjectParser::parseComposite(ctx); }

static RootNodeParseFn
rootNodeParserForToken(int tokenId)
{
    switch (tokenId) {
    case SPHERE_TOKEN:
        return parseSphereNode;
    case LIGHT_SOURCE_TOKEN:
        return parseLightNode;
    case UNION_TOKEN:
        return parseUnionNode;
    case INTERSECTION_TOKEN:
        return parseIntersectionNode;
    case DIFFERENCE_TOKEN:
        return parseDifferenceNode;
    case OBJECT_TOKEN:
        return parseObjectNode;
    case COMPOSITE_TOKEN:
        return parseCompositeNode;
    default:
        return nullptr;
    }
}

static AstDeclareNode *
parseDeclareNode(ParserContext &ctx)
{
    AstDeclareNode *node = new AstDeclareNode();
    node->sourceLine = ctx.token().tokenLineNo + 1;
    node->sourceFile = ctx.token().Filename;

    ParseHelpers::getExpectedToken(IDENTIFIER_TOKEN, ctx);
    node->identifierNumber = ctx.token().identifierNumber;
    ParseHelpers::getExpectedToken(EQUALS_TOKEN, ctx);
    ctx.tokenStream().getToken();
    RootNodeParseFn parseFn = rootNodeParserForToken(ctx.token().tokenId);
    if (parseFn == nullptr) {
        ParseErrorReporter::parseError(SPHERE_TOKEN, ctx);
    }
    node->value = parseFn(ctx);
    return node;
}

AstScene *
AstSceneParser::parseScene(ParserContext &ctx)
{
    AstScene *scene = new AstScene();

    int done = FALSE;
    while (!done) {
        ctx.tokenStream().getToken();
        switch (ctx.token().tokenId) {
        case DECLARE_TOKEN: {
            AstDeclareNode *decl = parseDeclareNode(ctx);
            if (!AstNodes::appendNode(scene->nodes, scene->nodeCount,
                    MAX_AST_SCENE_NODES, (AstNode *)decl)) {
                ParseErrorReporter::Error("Too many AST scene nodes", ctx);
            }
            break;
        }
        case SPHERE_TOKEN:
        case LIGHT_SOURCE_TOKEN:
        case UNION_TOKEN:
        case INTERSECTION_TOKEN:
        case DIFFERENCE_TOKEN:
        case OBJECT_TOKEN:
        case COMPOSITE_TOKEN: {
            RootNodeParseFn parseFn = rootNodeParserForToken(ctx.token().tokenId);
            if (parseFn == nullptr) {
                ParseErrorReporter::parseError(SPHERE_TOKEN, ctx);
            }
            AstNode *n = parseFn(ctx);
            if (!AstNodes::appendNode(
                    scene->nodes, scene->nodeCount, MAX_AST_SCENE_NODES, n)) {
                ParseErrorReporter::Error("Too many AST scene nodes", ctx);
            }
            break;
        }
        case END_OF_FILE_TOKEN:
            done = TRUE;
            break;
        default:
            ParseErrorReporter::parseError(SPHERE_TOKEN, ctx);
            break;
        }
    }

    return scene;
}
