#ifndef __POV_AST_SCENE_BUILDER_H__
#define __POV_AST_SCENE_BUILDER_H__

#include "io/pov/ParserConstants.h"

class AstScene;
class AstNode;
class AstSphereNode;
class AstLightSourceNode;
class AstCsgNode;
class AstObjectNode;
class AstCompositeNode;
class AstTransform;
struct AstVector3;
class Vector3Dd;
class Geometry;
class SimpleBody;
class Sphere;
class Light;
class CSG;
class Composite;
class ParserContext;
class RenderFrame;
struct Constant;

class AstSceneBuilder {
  public:
    static void build(const AstScene &scene, RenderFrame *framePtr, ParserContext &ctx);

  private:
    struct AstDeclTable {
        int count;
        int ids[ParserConstants::MAX_CONSTANTS];
        const AstNode *nodes[ParserConstants::MAX_CONSTANTS];
    };

    static Vector3Dd asVector(const AstVector3 &v);
    static void applyTransforms(
        Geometry *shape, const AstTransform *transforms, int count);
    static const AstNode *findDecl(
        const AstDeclTable &decls, int identifierNumber);
    static const Constant *findLegacyDecl(
        ParserContext &ctx, int identifierNumber);
    static Geometry *buildGeometryNode(
        const AstNode &node, ParserContext &ctx, const AstDeclTable &decls);
    static SimpleBody *buildSimpleBodyNode(
        const AstNode &node, ParserContext &ctx, const AstDeclTable &decls);
    static Sphere *buildSphere(const AstSphereNode &node, ParserContext &ctx,
        const AstDeclTable &decls);
    static Light *buildLight(const AstLightSourceNode &node, ParserContext &ctx,
        const AstDeclTable &decls);
    static CSG *buildCsg(const AstCsgNode &node, ParserContext &ctx,
        const AstDeclTable &decls);
    static SimpleBody *buildObject(
        const AstObjectNode &node, ParserContext &ctx, const AstDeclTable &decls);
    static Composite *buildComposite(const AstCompositeNode &node,
        ParserContext &ctx, const AstDeclTable &decls);
};

#endif
