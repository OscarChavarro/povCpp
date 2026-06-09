#ifndef __RGBA_COLOR_H__
#define __RGBA_COLOR_H__

class ColorRgba {
  private:
    double _r;
    double _g;
    double _b;
    double _a;

  public:
    double getR() const { return _r; }
    double getG() const { return _g; }
    double getB() const { return _b; }
    double getA() const { return _a; }

    void setR(double v) { _r = v; }
    void setG(double v) { _g = v; }
    void setB(double v) { _b = v; }
    void setA(double v) { _a = v; }
};

#endif
