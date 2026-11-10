#include <gtest/gtest.h>
#include "vsdk/toolkit/media/NormalMap.h"
#include "vsdk/toolkit/media/NormalMapTexture.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"

TEST(NormalMapTextureTest, PreservesFlatNormalAndBottomUpOrderAndClearsWithMap)
{
    NormalMap* map = new NormalMap();
    ASSERT_TRUE(map->init(1, 2));
    map->putNormal(0, 0, Vector3Dd(0, 0, 1));
    map->putNormal(0, 1, Vector3Dd(1, 0, 0));
    SimpleBody body;
    body.setNormalMap(map);
    NormalMapTexture* texture = dynamic_cast<NormalMapTexture*>(body.getNormalMapRgb());
    ASSERT_NE(texture, nullptr);
    const float expected[] = {1, .5f, .5f, .5f, .5f, 1};
    for (int i = 0; i < 6; ++i) EXPECT_FLOAT_EQ(texture->getFloatSamples()[i], expected[i]);
    body.setNormalMap(nullptr);
    EXPECT_EQ(body.getNormalMapRgb(), nullptr);
}
