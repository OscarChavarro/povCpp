#ifndef __POV_AST_NODES_H__
#define __POV_AST_NODES_H__

#include "io/pov/ParserConstants.h"

enum AstNodeKind {
    AST_DECLARE_NODE = 0,
    AST_SPHERE_NODE = 1,
    AST_LIGHT_SOURCE_NODE = 2,
    AST_CSG_NODE = 3,
    AST_OBJECT_NODE = 4,
    AST_COMPOSITE_NODE = 5,
    AST_PLANE_NODE = 6,
    AST_FOG_NODE = 7,
    AST_CAMERA_NODE = 8,
    AST_MAX_TRACE_LEVEL_NODE = 9,
    AST_DEFAULT_TEXTURE_NODE = 10,
    AST_BOX_NODE = 11,
    AST_QUADRIC_NODE = 12,
    AST_BLOB_NODE = 13
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

enum AstCameraOpKind {
    AST_CAMERA_REF = 1,
    AST_CAMERA_LOCATION = 2,
    AST_CAMERA_DIRECTION = 3,
    AST_CAMERA_UP = 4,
    AST_CAMERA_RIGHT = 5,
    AST_CAMERA_SKY = 6,
    AST_CAMERA_LOOK_AT = 7,
    AST_CAMERA_TRANSLATE = 8,
    AST_CAMERA_ROTATE = 9,
    AST_CAMERA_SCALE = 10
};

struct AstCameraOp {
    AstCameraOpKind kind;
    int referenceConstantId;
    AstVector3 vectorValue;
};

class AstLimits {
  public:
    static constexpr int MAX_AST_TRANSFORMS = 64;
    static constexpr int MAX_AST_CHILDREN = 256;
    static constexpr int MAX_AST_SCENE_NODES = 2048;
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
    AstTransform transforms[AstLimits::MAX_AST_TRANSFORMS];
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
    AstTransform transforms[AstLimits::MAX_AST_TRANSFORMS];
    int transformCount;
};

class AstPlaneNode : public AstNode {
  public:
    AstPlaneNode();

    bool hasInlineData;
    bool hasReference;
    int referenceConstantId;
    AstVector3 normal;
    double distance;
    bool hasColour;
    AstColor colour;
    AstTransform transforms[AstLimits::MAX_AST_TRANSFORMS];
    int transformCount;
};

class AstBoxNode : public AstNode {
  public:
    AstBoxNode();

    bool hasInlineData;
    bool hasReference;
    int referenceConstantId;
    AstVector3 minBound;
    AstVector3 maxBound;
    bool hasColour;
    AstColor colour;
    AstTransform transforms[AstLimits::MAX_AST_TRANSFORMS];
    int transformCount;
};

class AstQuadricNode : public AstNode {
  public:
    AstQuadricNode();

    bool hasInlineData;
    bool hasReference;
    int referenceConstantId;
    AstVector3 object2Terms;
    AstVector3 objectMixedTerms;
    AstVector3 objectTerms;
    double objectConstant;
    bool hasColour;
    AstColor colour;
    AstTransform transforms[AstLimits::MAX_AST_TRANSFORMS];
    int transformCount;
};

struct AstBlobComponent {
    double coeff;
    double radius;
    AstVector3 pos;
};

class AstBlobNode : public AstNode {
  public:
    AstBlobNode();

    static constexpr int MAX_AST_BLOB_COMPONENTS = 512;
    bool hasInlineData;
    bool hasReference;
    int referenceConstantId;
    double threshold;
    AstBlobComponent components[MAX_AST_BLOB_COMPONENTS];
    int componentCount;
    bool sturm;
    bool hasColour;
    AstColor colour;
    AstTransform transforms[AstLimits::MAX_AST_TRANSFORMS];
    int transformCount;
};

class AstCsgNode : public AstNode {
  public:
    AstCsgNode();

    AstCsgOpKind op;
    bool hasReference;
    int referenceConstantId;
    AstNode *children[AstLimits::MAX_AST_CHILDREN];
    int childCount;
    AstTransform transforms[AstLimits::MAX_AST_TRANSFORMS];
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
    AstNode *boundedBy[AstLimits::MAX_AST_CHILDREN];
    int boundedByCount;
    AstNode *clippedBy[AstLimits::MAX_AST_CHILDREN];
    int clippedByCount;
    AstTransform transforms[AstLimits::MAX_AST_TRANSFORMS];
    int transformCount;
    bool noShadow;
};

class AstCompositeNode : public AstNode {
  public:
    AstCompositeNode();

    bool hasReference;
    int referenceConstantId;
    AstNode *children[AstLimits::MAX_AST_CHILDREN];
    int childCount;
    AstNode *boundedBy[AstLimits::MAX_AST_CHILDREN];
    int boundedByCount;
    AstNode *clippedBy[AstLimits::MAX_AST_CHILDREN];
    int clippedByCount;
    AstTransform transforms[AstLimits::MAX_AST_TRANSFORMS];
    int transformCount;
};

class AstFogNode : public AstNode {
  public:
    AstFogNode();

    bool hasColour;
    AstColor colour;
    bool hasDistance;
    double distance;
};

class AstCameraNode : public AstNode {
  public:
    AstCameraNode();

    static constexpr int MAX_AST_CAMERA_OPS = 128;
    AstCameraOp ops[MAX_AST_CAMERA_OPS];
    int opCount;
};

class AstMaxTraceLevelNode : public AstNode {
  public:
    AstMaxTraceLevelNode();

    double value;
};

class Texture;

class AstDefaultTextureNode : public AstNode {
  public:
    AstDefaultTextureNode();

    Texture *texture;
};

class AstScene {
  public:
    AstScene();

    AstNode *nodes[AstLimits::MAX_AST_SCENE_NODES];
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
