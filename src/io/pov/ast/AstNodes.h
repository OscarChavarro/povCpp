#ifndef __POV_AST_NODES_H__
#define __POV_AST_NODES_H__

#include "io/pov/ParserConstants.h"

static constexpr int MAX_AST_TRANSFORMS = 64;
static constexpr int MAX_AST_CHILDREN = 256;
static constexpr int MAX_AST_SCENE_NODES = 2048;

enum AstNodeKind {
    AST_DECLARE_NODE = 0,
    AST_SPHERE_NODE = 1,
    AST_LIGHT_SOURCE_NODE = 2,
    AST_CSG_NODE = 3,
    AST_OBJECT_NODE = 4,
    AST_COMPOSITE_NODE = 5
};

enum AstTransformKind {
    AST_TRANSLATE = 1,
    AST_ROTATE = 2,
    AST_SCALE = 3,
    AST_INVERSE = 4
};

enum AstCsgOpKind {
    AST_CSG_UNION = 1,
    AST_CSG_INTERSECTION = 2,
    AST_CSG_DIFFERENCE = 3
};

struct AstVector3 {
    double x;
    double y;
    double z;
};

struct AstColor {
    double r;
    double g;
    double b;
    double a;
};

struct AstTransform {
    AstTransformKind kind;
    AstVector3 vectorValue;
};

class AstNode {
  public:
    virtual ~AstNode() {}
    AstNodeKind kind;
    int sourceLine;
    const char *sourceFile;
};

class AstSphereNode : public AstNode {
  public:
    AstSphereNode();

    bool hasInlineData;
    bool hasReference;
    int referenceConstantId;
    AstVector3 center;
    double radius;
    bool hasColour;
    AstColor colour;
    AstTransform transforms[MAX_AST_TRANSFORMS];
    int transformCount;
};

class AstLightSourceNode : public AstNode {
  public:
    AstLightSourceNode();

    bool hasInlineData;
    bool hasReference;
    int referenceConstantId;
    AstVector3 center;
    AstColor colour;
    bool hasSpotlight;
    AstVector3 pointsAt;
    double tightness;
    double radiusDegrees;
    double falloffDegrees;
    AstTransform transforms[MAX_AST_TRANSFORMS];
    int transformCount;
};

class AstCsgNode : public AstNode {
  public:
    AstCsgNode();

    AstCsgOpKind op;
    bool hasReference;
    int referenceConstantId;
    AstNode *children[MAX_AST_CHILDREN];
    int childCount;
    AstTransform transforms[MAX_AST_TRANSFORMS];
    int transformCount;
};

class AstDeclareNode : public AstNode {
  public:
    AstDeclareNode();

    int identifierNumber;
    AstNode *value;
};

class AstObjectNode : public AstNode {
  public:
    AstObjectNode();

    bool hasReference;
    int referenceConstantId;
    AstNode *shape;
    AstNode *boundedBy[MAX_AST_CHILDREN];
    int boundedByCount;
    AstNode *clippedBy[MAX_AST_CHILDREN];
    int clippedByCount;
    AstTransform transforms[MAX_AST_TRANSFORMS];
    int transformCount;
    bool noShadow;
};

class AstCompositeNode : public AstNode {
  public:
    AstCompositeNode();

    bool hasReference;
    int referenceConstantId;
    AstNode *children[MAX_AST_CHILDREN];
    int childCount;
    AstNode *boundedBy[MAX_AST_CHILDREN];
    int boundedByCount;
    AstNode *clippedBy[MAX_AST_CHILDREN];
    int clippedByCount;
    AstTransform transforms[MAX_AST_TRANSFORMS];
    int transformCount;
};

class AstScene {
  public:
    AstScene();

    AstNode *nodes[MAX_AST_SCENE_NODES];
    int nodeCount;
};

class AstNodes {
  public:
    static bool appendTransform(
        AstTransform *arr, int &count, AstTransformKind kind, const AstVector3 &vec);
    static bool appendNode(AstNode **arr, int &count, int cap, AstNode *node);
    static void destroyNode(AstNode *node);
    static void destroyScene(AstScene *scene);
};

#endif
