#ifndef __GEOMETRY_OBJECT_UTILS_H__
#define __GEOMETRY_OBJECT_UTILS_H__

class SimpleBody;

class ObjectUtils {
  public:
    static void link(
        SimpleBody *newObject, SimpleBody **field, SimpleBody **oldObjectList);
    static SimpleBody *getObject(void);
};

#endif
