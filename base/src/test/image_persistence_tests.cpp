#include "java/lang/String.h"
#include <gtest/gtest.h>

#include <cstdio>

#include "java/io/File.h"
#include "vsdk/toolkit/io/image/ImagePersistence.h"
#include "vsdk/toolkit/media/RGBAImageUncompressed.h"
#include "vsdk/toolkit/media/RGBAPixel.h"
#include "vsdk/toolkit/media/RGBImageUncompressed.h"

TEST(ImagePersistenceTest, ImportRGBAReadsPngPixelsAsOpaque)
{
    RGBImageUncompressed image;
    image.init(3, 2);
    image.putPixel(0, 0, (char)255, 0, 0);
    image.putPixel(2, 1, 0, 0, (char)200);
    java::String path = java::String(testing::TempDir().c_str()) + "vitral_import_rgba.png";
    java::File file(path.c_str());
    if ( !ImagePersistence::exportPNG(file, &image) ) {
        GTEST_SKIP() << "Built without PNG support";
    }

    RGBAImageUncompressed* imported = ImagePersistence::importRGBA(file);
    ASSERT_NE(nullptr, imported);
    EXPECT_EQ(3, imported->getXSize());
    EXPECT_EQ(2, imported->getYSize());
    RGBAPixel pixel;
    imported->getPixelRgba(0, 0, &pixel);
    EXPECT_EQ(255, (unsigned char)pixel.r);
    EXPECT_EQ(0, (unsigned char)pixel.g);
    EXPECT_EQ(255, (unsigned char)pixel.a);
    imported->getPixelRgba(2, 1, &pixel);
    EXPECT_EQ(200, (unsigned char)pixel.b);
    EXPECT_EQ(255, (unsigned char)pixel.a);
    delete imported;
    std::remove(path.c_str());
}
