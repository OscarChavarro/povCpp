#ifndef __SIMPLE_BACKGROUND__
#define __SIMPLE_BACKGROUND__

#include "vsdk/toolkit/environment/background/Background.h"
class SimpleBackground : public Background {
private:
    ColorRgb color;

public:
    SimpleBackground();
    virtual ~SimpleBackground() {}

    virtual ColorRgb colorInDireccion(const Vector3Dd& d) override;
    void setColor(double r, double g, double b);
};

#endif
