#ifndef __SIMPLE_BODY_FACTORY_H__
#define __SIMPLE_BODY_FACTORY_H__

class SimpleBody;

class SimpleBodyFactory {
  public:
    static void link(
        SimpleBody *newObject, SimpleBody **field, SimpleBody **oldObjectList);
    static SimpleBody *getObject(void);
};

#endif
