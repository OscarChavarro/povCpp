#ifndef __VSDK_TOOLKIT_COMMON_COLOR_COLORRGB_H__
#define __VSDK_TOOLKIT_COMMON_COLOR_COLORRGB_H__

class ColorRgb {
private:
    double r_;
    double g_;
    double b_;

public:
    ColorRgb();
    ColorRgb(const ColorRgb& c);
    ColorRgb(double r, double g, double b);
    ~ColorRgb();

    double r() const;
    double g() const;
    double b() const;

    double getR() const;
    double getG() const;
    double getB() const;

    double distance(const ColorRgb& other) const;
    ColorRgb add(const ColorRgb& other) const;
    ColorRgb multiply(double scalar) const;
};

#endif
