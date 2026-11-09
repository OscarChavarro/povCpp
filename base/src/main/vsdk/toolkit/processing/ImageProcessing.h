#ifndef __IMAGE_PROCESSING__
#define __IMAGE_PROCESSING__

#include "vsdk/toolkit/processing/ProcessingElement.h"

class Image;
class IndexedColorImageUncompressed;
class RGBImageUncompressed;

/**
\todo  Current implementation is not well designed. This class' design should
be checked to inforce:
  - Interoperability with existing image processing toolkits/frameworks like
    JAI, ITK, Khoros, OpenCV, Matlab+ImageToolbox, ImageMagick+JMagick, GIMP,
    etc.
  - Programmability of image processing operations using GPUs
  - Filter graph approach
*/
class ImageProcessing : public ProcessingElement {
private:
    static int gammaCorrection8bits(int in, double gamma);

public:
    static void gammaCorrection(IndexedColorImageUncompressed* img,
                                double gamma);
    static void gammaCorrection(RGBImageUncompressed* img, double gamma);

    /**
    Given the `input` and `output` previously created images, fills in
    `output`'s space the `this` image using bilinear interpolation.
    \todo  worked well only in the growing case. Must add the shrinking
    case for area averaging.
    */
    static void resize(Image* input, Image* output);

    /**
    Copies the contents from the `input` image to the `output` image. Note
    that this method can serve also as a format conversion between different
    Image formats (i.e. convert an RGBImageUncompressed to an
    IndexedColorImageUncompressed).
    */
    static void copy(Image* input, Image* output);

    /**
    Takes the greater dimension of the input image, and uses it to create an
    output square image of such dimension. Then copies the input image
    centered in the output image.
    */
    static void squareFill(Image* input, Image* output);

    /**
    Grows the input image copying the input in the center of a border of
    `border` size.
    */
    static void frame(Image* input, Image* output, int border);

    /**
    This method extracts a region of interest from source image rectangle
    including points from <x0Roi, y0Roi> to <x1Roi, y1Roi>.
    */
    static void extractRoi(Image* source, Image* roi,
        int x0Roi, int y0Roi, int x1Roi, int y1Roi);

    /**
    A distance field is a scalar map where each pixel value correspond to the
    nearest distance to an "inside" pixel.
    Every pixel in the input image with a value greater or equal to
    `threshold` will be noted as "inside", otherwise will be "outside".
    This implements the naive, real, full, simple (direct) and non-optimized
    version of the algorithm, which doesn't have extra memory requirements
    and has the following complexity:
       - Time: O(N^4)
       - Space: O(2*N^2)
    Where N is the size in pixels of a squared input image for the square
    image case.
    This version of the algorithm is provided for reference. Use
    processDistanceFieldWithArray instead.
    @return false if images are null or of different sizes
    */
    static bool processDistanceField(Image* inInput,
        IndexedColorImageUncompressed* outOutput, int threshold);

    /**
    Optimized version of `processDistanceField` using a dynamic programming
    technique which requires an extra preprocessing step and an array, which
    is of N^2 positions in the worst case.
    Algorithm with optimization is bounded by
       - Time: O(N^4)
       - Space: O(3*N^2)
    but falls to
       - Time: O((2+K)*N^2)
       - Space: O(2*N^2+K)
    where K is usually N*0.06 in contour type images.
    @return false if images are null or of different sizes
    */
    static bool processDistanceFieldWithArray(Image* inInput,
        IndexedColorImageUncompressed* outOutput, int threshold);
};

#endif
