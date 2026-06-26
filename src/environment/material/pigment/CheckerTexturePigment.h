#ifndef __CHECKER_TEXTURE_PIGMENT__
#define __CHECKER_TEXTURE_PIGMENT__

#include "environment/material/pigment/ICheckerTextureSlot.h"
#include "environment/material/pigment/SolidTexturePigment.h"

class CheckerTexturePigment : public SolidTexturePigment {
  public:
    CheckerTexturePigment(ICheckerTextureSlot *texture1, ICheckerTextureSlot *texture2);
    ~CheckerTexturePigment() override;

    void colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
        const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const override;
    SolidTexturePigment *copy() const override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
    void translate(Vector3Dd *vector) override;

    ICheckerTextureSlot *getTexture1() const;
    ICheckerTextureSlot *getTexture2() const;

  private:
    ICheckerTextureSlot *texture1;
    ICheckerTextureSlot *texture2;
};

#endif
