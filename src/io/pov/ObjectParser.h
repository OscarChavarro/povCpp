#ifndef __OBJECT_PARSER_H__
#define __OBJECT_PARSER_H__

class Geometry;
class SimpleBody;
class CSG;

class ObjectParser {
  public:
    static Geometry *parseShape();
    static SimpleBody *parseObject();
    static SimpleBody *parseComposite();
    static CSG *parseCsg(int type);
};

#endif
