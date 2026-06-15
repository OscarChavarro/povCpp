#ifndef __SIMPLE_BODY_FACTORY_H__
#define __SIMPLE_BODY_FACTORY_H__

#include "environment/geometry/SimpleBody.h"

class SimpleBodyFactory {
  public:
    static SimpleBody *getObject(void);
};

#endif
