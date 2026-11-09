#ifndef __VIEWPORT_ELEMENT_SCALER__
#define __VIEWPORT_ELEMENT_SCALER__

/**
A `ViewportElementScaler` calculates how much the elements of a viewport
(text and its related lengths) must be enlarged for the resolution of a
screen.

Sizes in the application are designed for a legacy reference resolution
(1024x768 by default; 640x480 and 800x600 screens are also served with the
designed sizes). On screens with more pixels the same size in pixels looks
tiny, so the scale grows with the resolution: it is the smaller of
the width and height ratios between the screen and the reference resolution,
rounded to steps of a quarter, and never below 1 (sizes are not reduced for
legacy resolutions). For example, a 2560x1600 screen gets a scale of 2, and
a 1920x1080 one gets 1.5.

The class is a plain model object: it does not know how the resolution is
obtained, since that depends on the platform. Whoever knows it informs the
scaler with `setScreenResolution`, and can do it again if the resolution
changes, so the scale follows the screen. Until a valid resolution is given,
the scale is 1.
*/
class ViewportElementScaler {
private:
    int screenWidthInPixels;
    int screenHeightInPixels;
    int referenceWidthInPixels;
    int referenceHeightInPixels;
    double scaleStep;
    double maximumScale;

public:
    ViewportElementScaler();
    virtual ~ViewportElementScaler() {}

    int getScreenWidthInPixels() const;
    int getScreenHeightInPixels() const;

    /**
    Informs the resolution, in physical pixels, of the screen where text is
    presented.
    */
    void setScreenResolution(int screenWidthInPixels,
                             int screenHeightInPixels);
    int getReferenceWidthInPixels() const;
    int getReferenceHeightInPixels() const;

    /**
    Sets the resolution for which the base sizes were designed; invalid
    (not positive) values are ignored.
    */
    void setReferenceResolution(int referenceWidthInPixels,
                                int referenceHeightInPixels);
    double getScaleStep() const;

    /**
    @param scaleStep the scale is rounded to a multiple of this value; not
    positive values are ignored
    */
    void setScaleStep(double scaleStep);
    double getMaximumScale() const;

    /**
    @param maximumScale upper limit of the scale; values under 1 are ignored
    */
    void setMaximumScale(double maximumScale);

    /**
    @return the factor to apply to base sizes for the current screen
    resolution, between 1 and the maximum scale
    */
    double getScale() const;

    /**
    @param baseLength a length (line width, etc.) in pixels, designed for the
    reference resolution, that can have fractional values
    @return the length to use in the current screen
    */
    double scaleLength(double baseLength) const;

    /**
    @param baseSize a size (font size, offset, etc.) in pixels, designed for
    the reference resolution
    @return the size to use in the current screen, at least 1
    */
    int scaleSize(int baseSize) const;
};

#endif
