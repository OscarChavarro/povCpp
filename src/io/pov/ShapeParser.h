#ifndef __SHAPE_PARSER_H__
#define __SHAPE_PARSER_H__

class Geometry;

class ShapeParser {
  public:
    static Geometry *parseSphere();
    static Geometry *parseLightSource();
    static Geometry *parsePlane();
    static Geometry *parseTriangle();
    static Geometry *parseSmoothTriangle();
    static Geometry *parseQuadric();
    static Geometry *parsePoly(int order);
    static Geometry *parseBicubicPatch();
    static Geometry *parseBox();
    static Geometry *parseBlob();
    static Geometry *parseHeightField();
};

#endif
