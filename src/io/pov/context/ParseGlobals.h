#ifndef __PARSE_GLOBALS_H__
#define __PARSE_GLOBALS_H__

// Types of constants allowed in DECLARE statement
class ParseGlobals {
  public:
    static constexpr int OBJECT_CONSTANT = 0;
    static constexpr int VIEW_POINT_CONSTANT = 1;
    static constexpr int VECTOR_CONSTANT = 2;
    static constexpr int FLOAT_CONSTANT = 3;
    static constexpr int COLOUR_CONSTANT = 4;
    static constexpr int QUADRIC_CONSTANT = 5;
    static constexpr int POLY_CONSTANT = 6;
    static constexpr int BICUBIC_PATCH_CONSTANT = 7;
    static constexpr int SPHERE_CONSTANT = 8;
    static constexpr int PLANE_CONSTANT = 9;
    static constexpr int TRIANGLE_CONSTANT = 10;
    static constexpr int SMOOTH_TRIANGLE_CONSTANT = 11;
    static constexpr int CSG_INTERSECTION_CONSTANT = 12;
    static constexpr int CSG_UNION_CONSTANT = 13;
    static constexpr int CSG_DIFFERENCE_CONSTANT = 14;
    static constexpr int COMPOSITE_CONSTANT = 15;
    static constexpr int TEXTURE_CONSTANT = 16;
    static constexpr int HEIGHT_FIELD_CONSTANT = 17;
    static constexpr int BOX_CONSTANT = 18;
    static constexpr int BLOB_CONSTANT = 19;
    static constexpr int LIGHT_SOURCE_CONSTANT = 20;
};

class Constant {
  public:
    int getIdentifierNumber() const;
    void setIdentifierNumber(int value);
    int getConstantType() const;
    void setConstantType(int value);
    void *getConstantData() const;
    void setConstantData(void *value);

  private:
    int identifierNumber;
    int constantType;
    void *constantData;
};

inline int
Constant::getIdentifierNumber() const
{
    return identifierNumber;
}

inline void
Constant::setIdentifierNumber(int value)
{
    identifierNumber = value;
}

inline int
Constant::getConstantType() const
{
    return constantType;
}

inline void
Constant::setConstantType(int value)
{
    constantType = value;
}

inline void *
Constant::getConstantData() const
{
    return constantData;
}

inline void
Constant::setConstantData(void *value)
{
    constantData = value;
}

#endif
