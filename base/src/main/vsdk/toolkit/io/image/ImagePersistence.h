#ifndef __IMAGE_PERSISTENCE__
#define __IMAGE_PERSISTENCE__

#include "java/io/File.h"
#include "java/lang/String.h"
class Image;
class RGBImageUncompressed;
class RGBAImageUncompressed;
class RGBAImageCompressed;
class IndexedColorImageUncompressed;

/**
This class is a front end through which images of various formats can be
exported and/or imported to/from files.

This class follows a Singleton design pattern.
*/
class ImagePersistence {

public:
    /**
    Given the filename of an input data file which contains an image, this
    method tries to recognize the file format and load the contents of it
    to the image.

    @param inImageFd - The file containing the image
    @return An RGBImageUncompressed entity that contains the image loaded in memory.
    */
    static RGBImageUncompressed* importRGB(const java::File& inImageFd);

    static IndexedColorImageUncompressed* importIndexedColor(const java::File& inImageFd);

    /**
    Given the filename of an input data file which contains an image, this
    method tries to recognize the file format and load the contents of it
    to the image.

    @param inImageFd - The file containing the image
    @return An RGBAImageUncompressed entity that contains the image loaded in memory.
    */
    static RGBAImageUncompressed* importRGBA(const java::File& inImageFd);

    /**
    Generic image import that may return compressed or uncompressed images
    depending on format support.
    */
    static Image* importImage(const java::File& inImageFd);

    /**
    This method writes the contents of the specified image to a file in
    binary RGB PPM format (i.e. P6 PPM sub-format). Returns true if everything
    works fine, false if something fails.
    @param fd
    @param img
    @return true if export was successful, false otherwise
    */
    static bool exportPPM(const java::File& fd, Image* img);

    /**
    This method writes the contents of the specified image to a file in
    JPEG format. Returns true if everything works fine, false if something fails.
    JPEG support must be compiled with -DWITH_JPEG=ON.
    @param fd
    @param img
    @param quality JPEG quality (0-100)
    @return true if export was successful, false otherwise
    */
    static bool exportJPEG(const java::File& fd, Image* img, int quality = 85);

    /**
    This method writes the contents of the specified image to a file in
    PNG format. Returns true if everything works fine, false if something fails.
    PNG support must be compiled with -DWITH_PNG=ON.
    @param fd
    @param img
    @return true if export was successful, false otherwise
    */
    static bool exportPNG(const java::File& fd, Image* img);

private:
    static bool isTextComment(const java::String& line);
    static java::String* extractExtensionFromFile(const java::File& fd);
};

#endif
