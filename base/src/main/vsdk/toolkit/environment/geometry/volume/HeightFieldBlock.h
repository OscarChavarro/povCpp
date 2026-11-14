#ifndef VSDK_HEIGHT_FIELD_BLOCK_H
#define VSDK_HEIGHT_FIELD_BLOCK_H

// Height range of one square block of height field cells, used to skip
// blocks a ray passes above or below.
class HeightFieldBlock {
private:
    double minimumHeight;
    double maximumHeight;
public:
    HeightFieldBlock() : minimumHeight(65536.0), maximumHeight(0.0) {}
    double getMinimumHeight() const { return minimumHeight; }
    double getMaximumHeight() const { return maximumHeight; }
    void include(double height)
    {
        if (height < minimumHeight) minimumHeight = height;
        if (height > maximumHeight) maximumHeight = height;
    }
};
#endif
