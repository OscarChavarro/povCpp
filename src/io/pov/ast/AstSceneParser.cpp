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
    case Tokenizer::SPHERE_TOKEN:
        return parseSphereNode;
    case Tokenizer::LIGHT_SOURCE_TOKEN:
        return parseLightNode;
    case Tokenizer::UNION_TOKEN:
        return parseUnionNode;
    case Tokenizer::INTERSECTION_TOKEN:
        return parseIntersectionNode;
    case Tokenizer::DIFFERENCE_TOKEN:
        return parseDifferenceNode;
    case Tokenizer::OBJECT_TOKEN:
        return parseObjectNode;
    case Tokenizer::COMPOSITE_TOKEN:
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

    ParseHelpers::getExpectedToken(Tokenizer::IDENTIFIER_TOKEN, ctx);
    node->identifierNumber = ctx.token().identifierNumber;
    ParseHelpers::getExpectedToken(Tokenizer::EQUALS_TOKEN, ctx);
    ctx.tokenStream().getToken();
    RootNodeParseFn parseFn = rootNodeParserForToken(ctx.token().tokenId);
    if (parseFn == nullptr) {
        ParseErrorReporter::parseError(Tokenizer::SPHERE_TOKEN, ctx);
    }
    node->value = parseFn(ctx);
    return node;
}

AstScene *
AstSceneParser::parseScene(ParserContext &ctx)
{
    AstScene *scene = new AstScene();

    int done = LegacyBoolean::FALSE_VALUE;
    while (!done) {
        ctx.tokenStream().getToken();
        switch (ctx.token().tokenId) {
        case Tokenizer::DECLARE_TOKEN: {
            AstDeclareNode *decl = parseDeclareNode(ctx);
            if (!AstNodes::appendNode(scene->nodes, scene->nodeCount,
                    MAX_AST_SCENE_NODES, (AstNode *)decl)) {
                ParseErrorReporter::Error("Too many AST scene nodes", ctx);
            }
            break;
        }
        case Tokenizer::SPHERE_TOKEN:
        case Tokenizer::LIGHT_SOURCE_TOKEN:
        case Tokenizer::UNION_TOKEN:
        case Tokenizer::INTERSECTION_TOKEN:
        case Tokenizer::DIFFERENCE_TOKEN:
        case Tokenizer::OBJECT_TOKEN:
        case Tokenizer::COMPOSITE_TOKEN: {
            RootNodeParseFn parseFn = rootNodeParserForToken(ctx.token().tokenId);
            if (parseFn == nullptr) {
                ParseErrorReporter::parseError(Tokenizer::SPHERE_TOKEN, ctx);
            }
            AstNode *n = parseFn(ctx);
            if (!AstNodes::appendNode(
                    scene->nodes, scene->nodeCount, MAX_AST_SCENE_NODES, n)) {
                ParseErrorReporter::Error("Too many AST scene nodes", ctx);
            }
            break;
        }
        case Tokenizer::END_OF_FILE_TOKEN:
            done = LegacyBoolean::TRUE_VALUE;
            break;
        default:
            ParseErrorReporter::parseError(Tokenizer::SPHERE_TOKEN, ctx);
            break;
        }
    }

    return scene;
}
