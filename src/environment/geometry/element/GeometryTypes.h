#ifndef __GEOMETRY_TYPES__
#define __GEOMETRY_TYPES__

enum class GeometryTypes {
  SPHERE_TYPE = 0,
  TRIANGLE_TYPE = 1,
  SMOOTH_TRIANGLE_TYPE = 2,
  PLANE_TYPE = 3,
  QUARTIC_TYPE = 4,
  POLY_TYPE = 5,
  BICUBIC_PATCH_TYPE = 6,
  COMPOSITE_TYPE = 7,
  OBJECT_TYPE = 8,
  CSG_UNION_TYPE = 9,
  CSG_INTERSECTION_TYPE = 10,
  CSG_DIFFERENCE_TYPE = 11,
  VIEWPOINT_TYPE = 12,
  HEIGHT_FIELD_TYPE = 13,
  POINT_LIGHT_TYPE = 14,
  SPOT_LIGHT_TYPE = 15,
  BOX_TYPE = 16,
  BLOB_TYPE = 17
};

constexpr int toInt(GeometryTypes type) {
  return static_cast<int>(type);
}

constexpr GeometryTypes fromInt(int value) {
  return static_cast<GeometryTypes>(value);
}

#endif
