#ifndef __RASTERIZER_2_D__
#define __RASTERIZER_2_D__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/surface/polygon/Polygon2D.h"
#include "vsdk/toolkit/render/RenderingElement.h"
class Image;
class RGBPixel;
class Vertex2D;

class Rasterizer2D : public RenderingElement {
public:
    static void drawLine(Image* img, int x0, int y0, int x1, int y1, const RGBPixel& p);
    static void drawPolygon(Image* img, Polygon2D& p, const RGBPixel& color);
    static void fillPolygon(Image* img, Polygon2D& p, const RGBPixel& color);
    static void fillSmoothPolygon(Image* img, Polygon2D& p);

private:
    struct FillEdge {
        int yMin;
        int yMaxExclusive;
        double xAtCurrentY;
        double inverseSlope;
        int sortOrder;
    };

    class SpanShader {
    public:
        virtual ~SpanShader() {}
        virtual void shade(Image* img, Polygon2D& polygon, int y, int xStart,
            int xEndExclusive) = 0;
    };

    static int clamp(int value, int minValue, int maxValue);
    static void sortFillEdges(java::ArrayList<FillEdge>& edges);
    static void addFillEdge(java::ArrayList< java::ArrayList<FillEdge> >& buckets,
        const Vertex2D& a, const Vertex2D& b, int imageHeight, int yRange[2],
        int sortOrder);
    static void rasterizePolygonSpans(Image* img, Polygon2D& polygon,
        SpanShader& shader);
    static void fillSmoothPolygonCalculateColor(
        Polygon2D& p,
        double x,
        double y,
        RGBPixel& outPixel);
};

#endif
