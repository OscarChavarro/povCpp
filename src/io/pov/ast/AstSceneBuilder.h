#ifndef __POV_AST_SCENE_BUILDER_H__
#define __POV_AST_SCENE_BUILDER_H__

#include "io/pov/ParserConstants.h"

class AstScene;
class AstNode;
class AstSphereNode;
class AstLightSourceNode;
class AstPlaneNode;
class AstBoxNode;
class AstQuadricNode;
class AstBlobNode;
class AstTriangleNode;
class AstSmoothTriangleNode;
class AstPolyNode;
class AstLegacyGeometryNode;
class AstCsgNode;
class AstObjectNode;
class AstCompositeNode;
class AstFogNode;
class AstCameraNode;
class AstMaxTraceLevelNode;
class AstDefaultTextureNode;
class AstTransform;
struct AstVector3;
class Vector3Dd;
class Geometry;
class SimpleBody;
class Sphere;
class Light;
class InfinitePlane;
class Box;
class Quadric;
class Blob;
class Triangle;
class SmoothTriangle;
class PolynomialShape;
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
    static InfinitePlane *buildPlane(
        const AstPlaneNode &node, ParserContext &ctx, const AstDeclTable &decls);
    static Box *buildBox(const AstBoxNode &node, ParserContext &ctx, const AstDeclTable &decls);
    static Quadric *buildQuadric(
        const AstQuadricNode &node, ParserContext &ctx, const AstDeclTable &decls);
    static Blob *buildBlob(const AstBlobNode &node, ParserContext &ctx, const AstDeclTable &decls);
    static Triangle *buildTriangle(
        const AstTriangleNode &node, ParserContext &ctx, const AstDeclTable &decls);
    static SmoothTriangle *buildSmoothTriangle(
        const AstSmoothTriangleNode &node, ParserContext &ctx, const AstDeclTable &decls);
    static PolynomialShape *buildPoly(
        const AstPolyNode &node, ParserContext &ctx, const AstDeclTable &decls);
    static CSG *buildCsg(const AstCsgNode &node, ParserContext &ctx,
        const AstDeclTable &decls);
    static SimpleBody *buildObject(
        const AstObjectNode &node, ParserContext &ctx, const AstDeclTable &decls);
    static Composite *buildComposite(const AstCompositeNode &node,
        ParserContext &ctx, const AstDeclTable &decls);
    static void applyFog(const AstFogNode &node, RenderFrame *framePtr);
    static void applyCamera(
        const AstCameraNode &node, RenderFrame *framePtr, ParserContext &ctx, const AstDeclTable &decls);
    static void applyMaxTraceLevel(const AstMaxTraceLevelNode &node);
    static void applyDefaultTexture(const AstDefaultTextureNode &node, ParserContext &ctx);
};

#endif
