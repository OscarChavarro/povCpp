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
    ANTLR_IR_PLANE_NODE = 7,
    ANTLR_IR_BOX_NODE = 8,
    ANTLR_IR_TRIANGLE_NODE = 9,
    ANTLR_IR_SMOOTH_TRIANGLE_NODE = 10,
    ANTLR_IR_QUADRIC_NODE = 11,
    ANTLR_IR_QUARTIC_NODE = 12,
    ANTLR_IR_BLOB_NODE = 13,
    ANTLR_IR_OBJECT_NODE = 14,
    ANTLR_IR_COMPOSITE_NODE = 15,
    ANTLR_IR_LIGHT_NODE = 16,
    ANTLR_IR_CSG_NODE = 17
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
    bool hasReference;
    std::string referenceIdentifier;
    AntlrIrCameraOp ops[MAX_CAMERA_OPS];
    int opCount;
};

class AntlrIrTextureChain {
  public:
    std::vector<std::string> rawElements;
    std::vector<std::string> simpleReferenceIdentifiers;
};

class AntlrIrSphereNode;
class AntlrIrPlaneNode;
class AntlrIrBoxNode;
class AntlrIrTriangleNode;
class AntlrIrSmoothTriangleNode;
class AntlrIrQuadricNode;
class AntlrIrQuarticNode;
class AntlrIrBlobNode;
class AntlrIrObjectNode;
class AntlrIrCompositeNode;
class AntlrIrLightNode;
class AntlrIrCsgNode;

class AntlrIrDeclareNode : public AntlrSceneIrNode {
  public:
    enum ValueKind {
        DECLARE_UNKNOWN = 0,
        DECLARE_TEXTURE_CHAIN = 1,
        DECLARE_SPHERE = 2,
        DECLARE_PLANE = 3,
        DECLARE_BOX = 4,
        DECLARE_TRIANGLE = 5,
        DECLARE_SMOOTH_TRIANGLE = 6,
        DECLARE_QUADRIC = 7,
        DECLARE_QUARTIC = 8,
        DECLARE_BLOB = 9,
        DECLARE_OBJECT = 10,
        DECLARE_COMPOSITE = 11,
        DECLARE_LIGHT = 12,
        DECLARE_CSG = 13,
        DECLARE_CAMERA = 14
    };

    AntlrIrDeclareNode();
    std::string identifier;
    ValueKind valueKind;
    bool hasTextureChainValue;
    AntlrIrTextureChain textureChainValue;
    bool hasSphereValue;
    AntlrIrSphereNode *sphereValue;
    bool hasPlaneValue;
    AntlrIrPlaneNode *planeValue;
    bool hasBoxValue;
    AntlrIrBoxNode *boxValue;
    bool hasTriangleValue;
    AntlrIrTriangleNode *triangleValue;
    bool hasSmoothTriangleValue;
    AntlrIrSmoothTriangleNode *smoothTriangleValue;
    bool hasQuadricValue;
    AntlrIrQuadricNode *quadricValue;
    bool hasQuarticValue;
    AntlrIrQuarticNode *quarticValue;
    bool hasBlobValue;
    AntlrIrBlobNode *blobValue;
    bool hasObjectValue;
    AntlrIrObjectNode *objectValue;
    bool hasCompositeValue;
    AntlrIrCompositeNode *compositeValue;
    bool hasLightValue;
    AntlrIrLightNode *lightValue;
    bool hasCsgValue;
    AntlrIrCsgNode *csgValue;
    bool hasCameraValue;
    AntlrIrCameraNode *cameraValue;
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

class AntlrIrPlaneNode : public AntlrSceneIrNode {
  public:
    static constexpr int MAX_TRANSFORMS = 64;

    AntlrIrPlaneNode();
    bool hasReferenceBase;
    std::string referenceIdentifier;
    bool hasInlineBase;
    AntlrIrVector3 normal;
    double distance;
    bool hasColour;
    AntlrIrColor colour;
    bool hasTextureChain;
    AntlrIrTextureChain textureChain;
    bool inverted;
    AntlrIrTransform transforms[MAX_TRANSFORMS];
    int transformCount;
};

class AntlrIrBoxNode : public AntlrSceneIrNode {
  public:
    static constexpr int MAX_TRANSFORMS = 64;

    AntlrIrBoxNode();
    bool hasReferenceBase;
    std::string referenceIdentifier;
    bool hasInlineBase;
    AntlrIrVector3 minBounds;
    AntlrIrVector3 maxBounds;
    bool hasColour;
    AntlrIrColor colour;
    bool hasTextureChain;
    AntlrIrTextureChain textureChain;
    bool inverted;
    AntlrIrTransform transforms[MAX_TRANSFORMS];
    int transformCount;
};

class AntlrIrTriangleNode : public AntlrSceneIrNode {
  public:
    static constexpr int MAX_TRANSFORMS = 64;

    AntlrIrTriangleNode();
    bool hasReferenceBase;
    std::string referenceIdentifier;
    bool hasInlineBase;
    AntlrIrVector3 p1;
    AntlrIrVector3 p2;
    AntlrIrVector3 p3;
    bool hasColour;
    AntlrIrColor colour;
    bool hasTextureChain;
    AntlrIrTextureChain textureChain;
    bool inverted;
    AntlrIrTransform transforms[MAX_TRANSFORMS];
    int transformCount;
};

class AntlrIrSmoothTriangleNode : public AntlrSceneIrNode {
  public:
    static constexpr int MAX_TRANSFORMS = 64;

    AntlrIrSmoothTriangleNode();
    bool hasReferenceBase;
    std::string referenceIdentifier;
    bool hasInlineBase;
    AntlrIrVector3 p1;
    AntlrIrVector3 n1;
    AntlrIrVector3 p2;
    AntlrIrVector3 n2;
    AntlrIrVector3 p3;
    AntlrIrVector3 n3;
    bool hasColour;
    AntlrIrColor colour;
    bool hasTextureChain;
    AntlrIrTextureChain textureChain;
    bool inverted;
    AntlrIrTransform transforms[MAX_TRANSFORMS];
    int transformCount;
};

class AntlrIrQuadricNode : public AntlrSceneIrNode {
  public:
    static constexpr int MAX_TRANSFORMS = 64;

    AntlrIrQuadricNode();
    bool hasReferenceBase;
    std::string referenceIdentifier;
    bool hasInlineBase;
    AntlrIrVector3 object2Terms;
    AntlrIrVector3 objectMixedTerms;
    AntlrIrVector3 objectTerms;
    double objectConstant;
    bool hasColour;
    AntlrIrColor colour;
    bool hasTextureChain;
    AntlrIrTextureChain textureChain;
    bool inverted;
    AntlrIrTransform transforms[MAX_TRANSFORMS];
    int transformCount;
};

class AntlrIrQuarticNode : public AntlrSceneIrNode {
  public:
    static constexpr int MAX_TRANSFORMS = 64;
    static constexpr int MAX_COEFFICIENTS = 64;

    AntlrIrQuarticNode();
    bool hasReferenceBase;
    std::string referenceIdentifier;
    bool hasInlineBase;
    int coefficientCount;
    double coefficients[MAX_COEFFICIENTS];
    bool sturm;
    bool hasColour;
    AntlrIrColor colour;
    bool hasTextureChain;
    AntlrIrTextureChain textureChain;
    bool inverted;
    AntlrIrTransform transforms[MAX_TRANSFORMS];
    int transformCount;
};

struct AntlrIrBlobComponent {
    double coeff;
    double radius;
    AntlrIrVector3 position;
};

class AntlrIrBlobNode : public AntlrSceneIrNode {
  public:
    static constexpr int MAX_TRANSFORMS = 64;
    static constexpr int MAX_COMPONENTS = 512;

    AntlrIrBlobNode();
    bool hasReferenceBase;
    std::string referenceIdentifier;
    bool hasInlineBase;
    bool hasThreshold;
    double threshold;
    int componentCount;
    AntlrIrBlobComponent components[MAX_COMPONENTS];
    bool sturm;
    bool hasColour;
    AntlrIrColor colour;
    bool hasTextureChain;
    AntlrIrTextureChain textureChain;
    bool inverted;
    AntlrIrTransform transforms[MAX_TRANSFORMS];
    int transformCount;
};

class AntlrIrObjectNode : public AntlrSceneIrNode {
  public:
    static constexpr int MAX_TRANSFORMS = 64;
    static constexpr int MAX_CHILD_SPHERES = 256;
    static constexpr int MAX_CHILD_PLANES = 128;
    static constexpr int MAX_CHILD_BOXES = 128;
    static constexpr int MAX_CHILD_TRIANGLES = 128;
    static constexpr int MAX_CHILD_SMOOTH_TRIANGLES = 128;
    static constexpr int MAX_CHILD_QUADRICS = 128;
    static constexpr int MAX_CHILD_QUARTICS = 128;
    static constexpr int MAX_CHILD_BLOBS = 128;
    static constexpr int MAX_CHILD_LIGHTS = 64;
    static constexpr int MAX_CHILD_OBJECTS = 128;
    static constexpr int MAX_CHILD_COMPOSITES = 128;
    static constexpr int MAX_CHILD_CSGS = 128;
    static constexpr int MAX_CHILD_REFERENCES = 256;
    static constexpr int MAX_BOUNDED_SPHERES = 128;
    static constexpr int MAX_BOUNDED_CSGS = 128;
    static constexpr int MAX_BOUNDED_REFERENCES = 128;
    static constexpr int MAX_CLIPPED_SPHERES = 128;
    static constexpr int MAX_CLIPPED_CSGS = 128;
    static constexpr int MAX_CLIPPED_REFERENCES = 128;

    AntlrIrObjectNode();
    bool hasReference;
    std::string referenceIdentifier;
    int childSphereCount;
    AntlrIrSphereNode *childSpheres[MAX_CHILD_SPHERES];
    int childPlaneCount;
    AntlrIrPlaneNode *childPlanes[MAX_CHILD_PLANES];
    int childBoxCount;
    AntlrIrBoxNode *childBoxes[MAX_CHILD_BOXES];
    int childTriangleCount;
    AntlrIrTriangleNode *childTriangles[MAX_CHILD_TRIANGLES];
    int childSmoothTriangleCount;
    AntlrIrSmoothTriangleNode *childSmoothTriangles[MAX_CHILD_SMOOTH_TRIANGLES];
    int childQuadricCount;
    AntlrIrQuadricNode *childQuadrics[MAX_CHILD_QUADRICS];
    int childQuarticCount;
    AntlrIrQuarticNode *childQuartics[MAX_CHILD_QUARTICS];
    int childBlobCount;
    AntlrIrBlobNode *childBlobs[MAX_CHILD_BLOBS];
    int childLightCount;
    AntlrIrLightNode *childLights[MAX_CHILD_LIGHTS];
    int childObjectCount;
    AntlrIrObjectNode *childObjects[MAX_CHILD_OBJECTS];
    int childCompositeCount;
    AntlrIrCompositeNode *childComposites[MAX_CHILD_COMPOSITES];
    int childCsgCount;
    AntlrIrCsgNode *childCsgs[MAX_CHILD_CSGS];
    int childReferenceCount;
    std::string childReferenceIdentifiers[MAX_CHILD_REFERENCES];
    int boundedSphereCount;
    AntlrIrSphereNode *boundedSpheres[MAX_BOUNDED_SPHERES];
    int boundedCsgCount;
    AntlrIrCsgNode *boundedCsgs[MAX_BOUNDED_CSGS];
    int boundedReferenceCount;
    std::string boundedReferenceIdentifiers[MAX_BOUNDED_REFERENCES];
    int clippedSphereCount;
    AntlrIrSphereNode *clippedSpheres[MAX_CLIPPED_SPHERES];
    int clippedCsgCount;
    AntlrIrCsgNode *clippedCsgs[MAX_CLIPPED_CSGS];
    int clippedReferenceCount;
    std::string clippedReferenceIdentifiers[MAX_CLIPPED_REFERENCES];
    bool hasColour;
    AntlrIrColor colour;
    bool hasTextureChain;
    AntlrIrTextureChain textureChain;
    bool noShadow;
    bool inverted;
    AntlrIrTransform transforms[MAX_TRANSFORMS];
    int transformCount;
};

class AntlrIrCompositeNode : public AntlrSceneIrNode {
  public:
    static constexpr int MAX_TRANSFORMS = 64;
    static constexpr int MAX_CHILD_SPHERES = 512;
    static constexpr int MAX_CHILD_OBJECTS = 256;
    static constexpr int MAX_CHILD_COMPOSITES = 256;
    static constexpr int MAX_CHILD_REFERENCES = 512;
    static constexpr int MAX_BOUNDED_SPHERES = 256;
    static constexpr int MAX_BOUNDED_CSGS = 256;
    static constexpr int MAX_BOUNDED_REFERENCES = 256;
    static constexpr int MAX_CLIPPED_SPHERES = 256;
    static constexpr int MAX_CLIPPED_CSGS = 256;
    static constexpr int MAX_CLIPPED_REFERENCES = 256;

    AntlrIrCompositeNode();
    bool hasReference;
    std::string referenceIdentifier;
    int childSphereCount;
    AntlrIrSphereNode *childSpheres[MAX_CHILD_SPHERES];
    int childObjectCount;
    AntlrIrObjectNode *childObjects[MAX_CHILD_OBJECTS];
    int childCompositeCount;
    AntlrIrCompositeNode *childComposites[MAX_CHILD_COMPOSITES];
    int childReferenceCount;
    std::string childReferenceIdentifiers[MAX_CHILD_REFERENCES];
    int boundedSphereCount;
    AntlrIrSphereNode *boundedSpheres[MAX_BOUNDED_SPHERES];
    int boundedCsgCount;
    AntlrIrCsgNode *boundedCsgs[MAX_BOUNDED_CSGS];
    int boundedReferenceCount;
    std::string boundedReferenceIdentifiers[MAX_BOUNDED_REFERENCES];
    int clippedSphereCount;
    AntlrIrSphereNode *clippedSpheres[MAX_CLIPPED_SPHERES];
    int clippedCsgCount;
    AntlrIrCsgNode *clippedCsgs[MAX_CLIPPED_CSGS];
    int clippedReferenceCount;
    std::string clippedReferenceIdentifiers[MAX_CLIPPED_REFERENCES];
    AntlrIrTransform transforms[MAX_TRANSFORMS];
    int transformCount;
};

class AntlrIrLightNode : public AntlrSceneIrNode {
  public:
    static constexpr int MAX_TRANSFORMS = 64;

    AntlrIrLightNode();
    bool hasReference;
    std::string referenceIdentifier;
    bool hasCenter;
    AntlrIrVector3 center;
    bool hasColour;
    AntlrIrColor colour;
    bool hasPointAt;
    AntlrIrVector3 pointAt;
    bool hasTightness;
    double tightness;
    bool hasRadius;
    double radiusDegrees;
    bool hasFalloff;
    double falloffDegrees;
    bool spotlight;
    AntlrIrTransform transforms[MAX_TRANSFORMS];
    int transformCount;
};

enum AntlrIrCsgOpKind {
    ANTLR_IR_CSG_UNION = 1,
    ANTLR_IR_CSG_INTERSECTION = 2,
    ANTLR_IR_CSG_DIFFERENCE = 3
};

class AntlrIrCsgNode : public AntlrSceneIrNode {
  public:
    static constexpr int MAX_TRANSFORMS = 64;
    static constexpr int MAX_CHILD_SPHERES = 512;
    static constexpr int MAX_CHILD_PLANES = 256;
    static constexpr int MAX_CHILD_BOXES = 256;
    static constexpr int MAX_CHILD_TRIANGLES = 256;
    static constexpr int MAX_CHILD_SMOOTH_TRIANGLES = 256;
    static constexpr int MAX_CHILD_QUADRICS = 2048;
    static constexpr int MAX_CHILD_QUARTICS = 256;
    static constexpr int MAX_CHILD_BLOBS = 256;
    static constexpr int MAX_CHILD_LIGHTS = 128;
    static constexpr int MAX_CHILD_OBJECTS = 256;
    static constexpr int MAX_CHILD_COMPOSITES = 256;
    static constexpr int MAX_CHILD_CSGS = 256;
    static constexpr int MAX_CHILD_REFERENCES = 512;

    AntlrIrCsgNode();
    AntlrIrCsgOpKind op;
    bool hasReference;
    std::string referenceIdentifier;
    int childSphereCount;
    AntlrIrSphereNode *childSpheres[MAX_CHILD_SPHERES];
    int childPlaneCount;
    AntlrIrPlaneNode *childPlanes[MAX_CHILD_PLANES];
    int childBoxCount;
    AntlrIrBoxNode *childBoxes[MAX_CHILD_BOXES];
    int childTriangleCount;
    AntlrIrTriangleNode *childTriangles[MAX_CHILD_TRIANGLES];
    int childSmoothTriangleCount;
    AntlrIrSmoothTriangleNode *childSmoothTriangles[MAX_CHILD_SMOOTH_TRIANGLES];
    int childQuadricCount;
    AntlrIrQuadricNode *childQuadrics[MAX_CHILD_QUADRICS];
    int childQuarticCount;
    AntlrIrQuarticNode *childQuartics[MAX_CHILD_QUARTICS];
    int childBlobCount;
    AntlrIrBlobNode *childBlobs[MAX_CHILD_BLOBS];
    int childLightCount;
    AntlrIrLightNode *childLights[MAX_CHILD_LIGHTS];
    int childObjectCount;
    AntlrIrObjectNode *childObjects[MAX_CHILD_OBJECTS];
    int childCompositeCount;
    AntlrIrCompositeNode *childComposites[MAX_CHILD_COMPOSITES];
    int childCsgCount;
    AntlrIrCsgNode *childCsgs[MAX_CHILD_CSGS];
    int childReferenceCount;
    std::string childReferenceIdentifiers[MAX_CHILD_REFERENCES];
    bool inverted;
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
