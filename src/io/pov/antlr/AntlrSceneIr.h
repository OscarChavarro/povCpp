#ifndef __POV_ANTLR_SCENE_IR_H__
#define __POV_ANTLR_SCENE_IR_H__

#include <cstddef>
#include <string>
#include <vector>

enum AntlrSceneIrNodeKind {
    ANTLR_IR_UNKNOWN_NODE = 0,
    ANTLR_IR_MAX_TRACE_LEVEL_NODE = 1,
    ANTLR_IR_FOG_NODE = 2,
    ANTLR_IR_CAMERA_NODE = 3,
    ANTLR_IR_DECLARE_NODE = 4,
    ANTLR_IR_DEFAULT_TEXTURE_NODE = 5,
    ANTLR_IR_SPHERE_NODE = 6,
    ANTLR_IR_OBJECT_NODE = 7,
    ANTLR_IR_COMPOSITE_NODE = 8
};

struct AntlrIrVector3 {
    double x;
    double y;
    double z;
};

struct AntlrIrColor {
    double r;
    double g;
    double b;
    double a;
};

enum AntlrIrCameraOpKind {
    ANTLR_IR_CAMERA_REF = 1,
    ANTLR_IR_CAMERA_LOCATION = 2,
    ANTLR_IR_CAMERA_DIRECTION = 3,
    ANTLR_IR_CAMERA_UP = 4,
    ANTLR_IR_CAMERA_RIGHT = 5,
    ANTLR_IR_CAMERA_SKY = 6,
    ANTLR_IR_CAMERA_LOOK_AT = 7,
    ANTLR_IR_CAMERA_TRANSLATE = 8,
    ANTLR_IR_CAMERA_ROTATE = 9,
    ANTLR_IR_CAMERA_SCALE = 10
};

struct AntlrIrCameraOp {
    AntlrIrCameraOpKind kind;
    int referenceIdentifierNumber;
    AntlrIrVector3 vectorValue;
};

enum AntlrIrTransformKind {
    ANTLR_IR_TRANSLATE = 1,
    ANTLR_IR_ROTATE = 2,
    ANTLR_IR_SCALE = 3
};

struct AntlrIrTransform {
    AntlrIrTransformKind kind;
    AntlrIrVector3 vectorValue;
};

class AntlrSceneIrNode {
  public:
    virtual ~AntlrSceneIrNode() {}
    AntlrSceneIrNodeKind kind;
    int sourceLine;
    int sourceColumn;
    const char *sourceFile;
};

class AntlrIrMaxTraceLevelNode : public AntlrSceneIrNode {
  public:
    AntlrIrMaxTraceLevelNode();
    double value;
};

class AntlrIrFogNode : public AntlrSceneIrNode {
  public:
    AntlrIrFogNode();
    bool hasColour;
    AntlrIrColor colour;
    bool hasDistance;
    double distance;
};

class AntlrIrCameraNode : public AntlrSceneIrNode {
  public:
    static constexpr int MAX_CAMERA_OPS = 128;

    AntlrIrCameraNode();
    AntlrIrCameraOp ops[MAX_CAMERA_OPS];
    int opCount;
};

class AntlrIrTextureChain {
  public:
    std::vector<std::string> rawElements;
    std::vector<std::string> simpleReferenceIdentifiers;
};

class AntlrIrSphereNode;
class AntlrIrObjectNode;
class AntlrIrCompositeNode;

class AntlrIrDeclareNode : public AntlrSceneIrNode {
  public:
    enum ValueKind {
        DECLARE_UNKNOWN = 0,
        DECLARE_TEXTURE_CHAIN = 1,
        DECLARE_SPHERE = 2,
        DECLARE_OBJECT = 3,
        DECLARE_COMPOSITE = 4
    };

    AntlrIrDeclareNode();
    std::string identifier;
    ValueKind valueKind;
    bool hasTextureChainValue;
    AntlrIrTextureChain textureChainValue;
    bool hasSphereValue;
    AntlrIrSphereNode *sphereValue;
    bool hasObjectValue;
    AntlrIrObjectNode *objectValue;
    bool hasCompositeValue;
    AntlrIrCompositeNode *compositeValue;
};

class AntlrIrDefaultTextureNode : public AntlrSceneIrNode {
  public:
    AntlrIrDefaultTextureNode();
    bool hasTextureChain;
    AntlrIrTextureChain textureChain;
};

class AntlrIrSphereNode : public AntlrSceneIrNode {
  public:
    static constexpr int MAX_TRANSFORMS = 64;

    AntlrIrSphereNode();
    bool hasReferenceBase;
    std::string referenceIdentifier;
    bool hasInlineBase;
    AntlrIrVector3 center;
    double radius;
    bool hasColour;
    AntlrIrColor colour;
    bool hasTextureChain;
    AntlrIrTextureChain textureChain;
    AntlrIrTransform transforms[MAX_TRANSFORMS];
    int transformCount;
};

class AntlrIrObjectNode : public AntlrSceneIrNode {
  public:
    static constexpr int MAX_TRANSFORMS = 64;
    static constexpr int MAX_CHILD_SHAPES = 256;
    static constexpr int MAX_CHILD_REFERENCES = 256;

    AntlrIrObjectNode();
    bool hasReference;
    std::string referenceIdentifier;
    int childShapeCount;
    std::string childShapeTexts[MAX_CHILD_SHAPES];
    int childReferenceCount;
    std::string childReferenceIdentifiers[MAX_CHILD_REFERENCES];
    bool hasColour;
    AntlrIrColor colour;
    bool hasTextureChain;
    AntlrIrTextureChain textureChain;
    bool noShadow;
    AntlrIrTransform transforms[MAX_TRANSFORMS];
    int transformCount;
};

class AntlrIrCompositeNode : public AntlrSceneIrNode {
  public:
    static constexpr int MAX_TRANSFORMS = 64;
    static constexpr int MAX_CHILD_SHAPES = 512;
    static constexpr int MAX_CHILD_REFERENCES = 512;

    AntlrIrCompositeNode();
    bool hasReference;
    std::string referenceIdentifier;
    int childShapeCount;
    std::string childShapeTexts[MAX_CHILD_SHAPES];
    int childReferenceCount;
    std::string childReferenceIdentifiers[MAX_CHILD_REFERENCES];
    AntlrIrTransform transforms[MAX_TRANSFORMS];
    int transformCount;
};

class AntlrSceneIrProgram {
  public:
    static constexpr int MAX_NODES = 2048;

    AntlrSceneIrProgram();
    AntlrSceneIrNode *nodes[MAX_NODES];
    int nodeCount;
};

class AntlrSceneIrNodes {
  public:
    static bool appendNode(AntlrSceneIrProgram &program, AntlrSceneIrNode *node);
    static bool appendCameraOp(AntlrIrCameraNode &camera, const AntlrIrCameraOp &op);
    static bool appendTransform(
        AntlrIrTransform *arr, int &count, AntlrIrTransformKind kind, const AntlrIrVector3 &v);
    static void destroyNode(AntlrSceneIrNode *node);
    static void destroyProgram(AntlrSceneIrProgram *program);
};

#endif
