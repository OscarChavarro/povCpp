#include <cmath>

#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/VSDKFatalException.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/media/Image.h"
#include "vsdk/toolkit/media/RGBPixel.h"
#include "vsdk/toolkit/environment/geometry/element/Vertex2D.h"
#include "vsdk/toolkit/environment/geometry/surface/polygon/_Polygon2DContour.h"
#include "vsdk/toolkit/render/raster/Rasterizer2D.h"
static void quicksortDoubles(double* array, int left0, int right0)
{
    int left = left0;
    int right = right0 + 1;
    double pivot;
    double temp;

    pivot = array[left0];
    do {
        do {
            left++;
        } while (left <= right0 && array[left] < pivot);
        do {
            right--;
            if (right < 0) {
                Logger::reportMessage("Rasterizer2D", Logger::ERROR, "quicksortDoubles", "quicksort span underflow");
                throw VSDKFatalException("quicksort span underflow");
            }
        } while (array[right] > pivot);
        if (left < right) {
            temp = array[left];
            array[left] = array[right];
            array[right] = temp;
        }
    } while (left <= right);

    temp = array[left0];
    array[left0] = array[right];
    array[right] = temp;

    if (left0 < right) {
        quicksortDoubles(array, left0, right);
    }
    if (left < right0) {
        quicksortDoubles(array, left, right0);
    }
}

static void sortDoubles(java::ArrayList<double>& arr)
{
    if (arr.size() < 2) {
        return;
    }
    quicksortDoubles(arr.data(), 0, static_cast<int>(arr.size()) - 1);
}

void Rasterizer2D::sortFillEdges(java::ArrayList<FillEdge>& edges)
{
    for (size_t i = 1; i < edges.size(); ++i) {
        FillEdge key = edges[i];
        size_t j = i;
        while (j > 0) {
            const FillEdge& prev = edges[j - 1];
            bool shouldMove =
                prev.xAtCurrentY > key.xAtCurrentY ||
                (prev.xAtCurrentY == key.xAtCurrentY &&
                    (prev.inverseSlope > key.inverseSlope ||
                        (prev.inverseSlope == key.inverseSlope &&
                            prev.sortOrder > key.sortOrder)));
            if (!shouldMove) {
                break;
            }
            edges[j] = prev;
            --j;
        }
        edges[j] = key;
    }
}

void Rasterizer2D::drawLine(Image* img, int x0, int y0, int x1, int y1, const RGBPixel& p)
{
    if (img == 0) return;
    double dx = (double)(x1 - x0);
    double dy = (double)(y1 - y0);
    int x, y;
    double xx, yy;

    if (std::abs(dx) > VSDK::EPSILON && std::abs(dy / dx) <= 1.0 && x1 > x0) {
        double dydx = dy / dx;
        for (x = x0, yy = (double)y0; x <= x1; x++) {
            y = (int)yy;
            if (x >= 0 && x < img->getXSize() && y >= 0 && y < img->getYSize()) {
                RGBPixel c(p);
                img->putPixelRgb(x, y, &c);
            }
            yy += dydx;
        }
    }
    else if (std::abs(dx) > VSDK::EPSILON && std::abs(dy / dx) <= 1.0 && x1 < x0) {
        double dydx = dy / dx;
        for (x = x1, yy = (double)y1; x <= x0; x++) {
            y = (int)yy;
            if (x >= 0 && x < img->getXSize() && y >= 0 && y < img->getYSize()) {
                RGBPixel c(p);
                img->putPixelRgb(x, y, &c);
            }
            yy += dydx;
        }
    }
    else if (std::abs(dy) > VSDK::EPSILON && y1 > y0) {
        double dxdy = dx / dy;
        for (y = y0, xx = (double)x0; y <= y1; y++) {
            x = (int)xx;
            if (x >= 0 && x < img->getXSize() && y >= 0 && y < img->getYSize()) {
                RGBPixel c(p);
                img->putPixelRgb(x, y, &c);
            }
            xx += dxdy;
        }
    }
    else if (std::abs(dy) > VSDK::EPSILON && y1 < y0) {
        double dxdy = dx / dy;
        for (y = y1, xx = (double)x1; y <= y0; y++) {
            x = (int)xx;
            if (x >= 0 && x < img->getXSize() && y >= 0 && y < img->getYSize()) {
                RGBPixel c(p);
                img->putPixelRgb(x, y, &c);
            }
            xx += dxdy;
        }
    }
}

void Rasterizer2D::drawPolygon(Image* img, Polygon2D& p, const RGBPixel& color)
{
    if (img == 0) return;
    for (long int i = 0; i < p.loops.size(); i++) {
        _Polygon2DContour* contour = p.loops[i];
        if (contour->vertices.size() < 2) continue;
        Vertex2D va(0, 0), vb(0, 0);
        for (long int j = 0; j + 1 < contour->vertices.size(); j++) {
            va = contour->vertices[j];
            vb = contour->vertices[j + 1];
            drawLine(img, (int)va.x, (int)va.y, (int)vb.x, (int)vb.y, color);
        }
        va = vb;
        vb = contour->vertices[0];
        drawLine(img, (int)va.x, (int)va.y, (int)vb.x, (int)vb.y, color);
    }
}

int Rasterizer2D::clamp(int value, int minValue, int maxValue)
{
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

void Rasterizer2D::addFillEdge(java::ArrayList< java::ArrayList<FillEdge> >& buckets,
    const Vertex2D& a, const Vertex2D& b, int imageHeight, int yRange[2],
    int sortOrder)
{
    double dx = b.x - a.x;
    double dy = b.y - a.y;

    if (std::abs(dx) < VSDK::EPSILON && std::abs(dy) < VSDK::EPSILON) {
        return;
    }
    if (std::abs(dy) < VSDK::EPSILON) {
        return;
    }

    const Vertex2D* top = &a;
    const Vertex2D* bottom = &b;
    if (a.y > b.y) {
        top = &b;
        bottom = &a;
        dx = -dx;
        dy = -dy;
    }

    double inverseSlope = dx / dy;
    int yMin = (int)std::ceil(top->y);
    int yMaxExclusive = (int)std::ceil(bottom->y);

    if (yMin >= yMaxExclusive) {
        return;
    }

    int clippedYMin = clamp(yMin, 0, imageHeight);
    int clippedYMaxExclusive = clamp(yMaxExclusive, 0, imageHeight);

    if (clippedYMin >= clippedYMaxExclusive) {
        return;
    }

    FillEdge edge;
    edge.yMin = clippedYMin;
    edge.yMaxExclusive = clippedYMaxExclusive;
    edge.inverseSlope = inverseSlope;
    edge.xAtCurrentY = top->x + (((double)clippedYMin) - top->y) * inverseSlope;
    edge.sortOrder = sortOrder;

    buckets[clippedYMin].add(edge);
    if (clippedYMin < yRange[0]) {
        yRange[0] = clippedYMin;
    }
    if (clippedYMaxExclusive > yRange[1]) {
        yRange[1] = clippedYMaxExclusive;
    }
}

void Rasterizer2D::rasterizePolygonSpans(Image* img, Polygon2D& polygon,
    SpanShader& shader)
{
    int imageWidth = img->getXSize();
    int imageHeight = img->getYSize();

    if (imageWidth <= 0 || imageHeight <= 0) {
        return;
    }

    java::ArrayList< java::ArrayList<FillEdge> > buckets(imageHeight);
    for (int i = 0; i < imageHeight; i++) {
        buckets.add(java::ArrayList<FillEdge>());
    }
    int yRange[2] = {imageHeight, 0};
    int sortOrder = 0;

    for (long int i = 0; i < polygon.loops.size(); i++) {
        _Polygon2DContour* contour = polygon.loops[i];
        long int vertexCount = contour->vertices.size();
        if (vertexCount < 2) {
            continue;
        }

        for (long int j = 0; j < vertexCount; j++) {
            const Vertex2D& a = contour->vertices[j];
            const Vertex2D& b = contour->vertices[(j + 1) % vertexCount];
            addFillEdge(buckets, a, b, imageHeight, yRange, sortOrder);
            sortOrder++;
        }
    }

    if (yRange[0] >= yRange[1]) {
        return;
    }

    java::ArrayList<FillEdge> activeEdges;

    for (int y = yRange[0]; y < yRange[1]; y++) {
        java::ArrayList<FillEdge>& bucket = buckets[y];
        for (long int i = 0; i < bucket.size(); i++) {
            activeEdges.add(bucket[i]);
        }

        for (int i = (int)activeEdges.size() - 1; i >= 0; i--) {
            if (y >= activeEdges[(size_t)i].yMaxExclusive) {
                activeEdges.remove(i);
            }
        }

        if (activeEdges.size() < 2) {
            for (size_t i = 0; i < activeEdges.size(); i++) {
                activeEdges[i].xAtCurrentY += activeEdges[i].inverseSlope;
            }
            continue;
        }

        sortFillEdges(activeEdges);

        for (size_t i = 0; i + 1 < activeEdges.size(); i += 2) {
            double xLeft = activeEdges[i].xAtCurrentY;
            double xRight = activeEdges[i + 1].xAtCurrentY;

            if (xLeft > xRight) {
                double tmp = xLeft;
                xLeft = xRight;
                xRight = tmp;
            }

            int xStart = (int)std::ceil(xLeft);
            int xEndExclusive = (int)std::ceil(xRight);

            if (xEndExclusive <= 0 || xStart >= imageWidth) {
                continue;
            }

            xStart = clamp(xStart, 0, imageWidth);
            xEndExclusive = clamp(xEndExclusive, 0, imageWidth);

            if (xStart < xEndExclusive) {
                shader.shade(img, polygon, y, xStart, xEndExclusive);
            }
        }

        for (size_t i = 0; i < activeEdges.size(); i++) {
            activeEdges[i].xAtCurrentY += activeEdges[i].inverseSlope;
        }
    }
}

void Rasterizer2D::fillPolygon(Image* img, Polygon2D& p, const RGBPixel& color)
{
    if (img == 0) return;
    class SolidSpanShader : public SpanShader {
    public:
        explicit SolidSpanShader(const RGBPixel& color) : color(color) {}

        virtual void shade(Image* img, Polygon2D& polygon, int y, int xStart,
            int xEndExclusive)
        {
            (void)polygon;
            for (int x = xStart; x < xEndExclusive; x++) {
                RGBPixel c(color);
                img->putPixelRgb(x, y, &c);
            }
        }

    private:
        RGBPixel color;
    };

    SolidSpanShader shader(color);
    rasterizePolygonSpans(img, p, shader);
}

void Rasterizer2D::fillSmoothPolygonCalculateColor(
    Polygon2D& p, double x, double y, RGBPixel& outPixel)
{
    double outR = 0.0, outG = 0.0, outB = 0.0;
    double totaldistance = 0.0;

    for (long int i = 0; i < p.loops.size(); i++) {
        for (long int j = 0; j < p.loops[i]->vertices.size(); j++) {
            const Vertex2D& va = p.loops[i]->vertices[j];
            double distance = 1.0 / (1.0 + std::sqrt((va.x - x) * (va.x - x) + (va.y - y) * (va.y - y)));
            totaldistance += distance;
            outR += va.color.r() * distance;
            outG += va.color.g() * distance;
            outB += va.color.b() * distance;
        }
    }
    if (totaldistance <= VSDK::EPSILON) {
        outPixel.r = 0; outPixel.g = 0; outPixel.b = 0;
        return;
    }
    double normalizedR = outR / totaldistance;
    double normalizedG = outG / totaldistance;
    double normalizedB = outB / totaldistance;
    double clippedR = java::Math::max(0.0, java::Math::min(1.0, normalizedR));
    double clippedG = java::Math::max(0.0, java::Math::min(1.0, normalizedG));
    double clippedB = java::Math::max(0.0, java::Math::min(1.0, normalizedB));
    int rr = (int)(clippedR * 255.0);
    int gg = (int)(clippedG * 255.0);
    int bb = (int)(clippedB * 255.0);
    outPixel.r = (char)(rr & 0xFF);
    outPixel.g = (char)(gg & 0xFF);
    outPixel.b = (char)(bb & 0xFF);
}

void Rasterizer2D::fillSmoothPolygon(Image* img, Polygon2D& p)
{
    if (img == 0) return;
    class SmoothSpanShader : public SpanShader {
    public:
        virtual void shade(Image* img, Polygon2D& polygon, int y, int xStart,
            int xEndExclusive)
        {
            RGBPixel color;
            for (int x = xStart; x < xEndExclusive; x++) {
                Rasterizer2D::fillSmoothPolygonCalculateColor(
                    polygon, x, y, color);
                img->putPixelRgb(x, y, &color);
            }
        }
    };

    SmoothSpanShader shader;
    rasterizePolygonSpans(img, p, shader);
}
