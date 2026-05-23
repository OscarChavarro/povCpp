#ifndef __GEOMETRY_UTILS_H__
#define __GEOMETRY_UTILS_H__

class SimpleBody;

class GeometryUtils {
  public:
    static void link(
        SimpleBody *newObject, SimpleBody **field, SimpleBody **oldObjectList);
    static SimpleBody *getObject(void);
};

#endif
