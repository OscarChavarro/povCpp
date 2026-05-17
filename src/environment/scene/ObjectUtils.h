#ifndef __OBJECT_UTILS_H__
#define __OBJECT_UTILS_H__

class SimpleBody;

class ObjectUtils {
  public:
    static void link(SimpleBody *newObject, SimpleBody **field, SimpleBody **oldObjectList);
    static SimpleBody *getObject(void);
};

#endif
