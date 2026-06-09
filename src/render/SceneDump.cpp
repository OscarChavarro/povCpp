#include "render/SceneDump.h"
#include "render/RenderEngine.h"
#include "environment/geometry/SimpleBody.h"
#include "media/solidTexture/Texture.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "common/color/RGBAColorPaletteSpan.h"

void dumpSceneStructure(FILE *f)
{
    if (f == nullptr) return;

    int idx = 0;
    for (SimpleBody *obj = RenderEngine::renderFrame().Objects; obj; obj = obj->nextObject) {
        fprintf(f, "OBJ %03d type=%d\n", idx, obj->Type);

        if (obj->objectTexture) {
            Texture *tex = obj->objectTexture;

            fprintf(f, "  tex.num=%d amb=%.6f dif=%.6f phong=%.6f psize=%.6f bril=%.6f\n",
                    tex->textureNumber,
                    tex->objectAmbient,
                    tex->objectDiffuse,
                    tex->objectPhong,
                    tex->objectPhongSize,
                    tex->objectBrilliance);

            fprintf(f, "  tex.spec=%.6f rough=%.6f refl=%.6f refr=%.6f ior=%.6f transmit=%.6f metal=%d\n",
                    tex->objectSpecular,
                    tex->objectRoughness,
                    tex->objectReflection,
                    tex->objectRefraction,
                    tex->objectIndexOfRefraction,
                    tex->objectTransmit,
                    tex->metallicFlag);

            fprintf(f, "  tex.turb=%.6f oct=%d freq=%.6f phase=%.6f mortar=%.6f bump=%d/%.6f\n",
                    tex->turbulence,
                    tex->octaves,
                    tex->frequency,
                    tex->phase,
                    tex->mortar,
                    tex->bumpNumber,
                    tex->bumpAmount);

            fprintf(f, "  tex.grad=%.6f,%.6f,%.6f\n",
                    tex->textureGradient.x(),
                    tex->textureGradient.y(),
                    tex->textureGradient.z());

            if (tex->color1) {
                fprintf(f, "  tex.c1=%.6f,%.6f,%.6f,%.6f",
                        tex->color1->Red,
                        tex->color1->Green,
                        tex->color1->Blue,
                        tex->color1->Alpha);
            } else {
                fprintf(f, "  tex.c1=none");
            }

            if (tex->color2) {
                fprintf(f, " c2=%.6f,%.6f,%.6f,%.6f",
                        tex->color2->Red,
                        tex->color2->Green,
                        tex->color2->Blue,
                        tex->color2->Alpha);
            } else {
                fprintf(f, " c2=none");
            }
            fprintf(f, "\n");

            if (tex->colorMap) {
                fprintf(f, "  tex.cmap n=%d", tex->colorMap->numberOfEntries);
                for (int i = 0; i < tex->colorMap->numberOfEntries; i++) {
                    RGBAColorPaletteSpan &entry = tex->colorMap->colorMapEntries[i];
                    fprintf(f, " [%.6f %.6f %.3f,%.3f,%.3f,%.3f->%.3f,%.3f,%.3f,%.3f]",
                            entry.start,
                            entry.end,
                            entry.startColor.Red,
                            entry.startColor.Green,
                            entry.startColor.Blue,
                            entry.startColor.Alpha,
                            entry.endColor.Red,
                            entry.endColor.Green,
                            entry.endColor.Blue,
                            entry.endColor.Alpha);
                }
                fprintf(f, "\n");
            }

            if (tex->image) {
                fprintf(f, "  tex.img grad=(%.6f,%.6f,%.6f) map=%d interp=%d once=%d w=%.0f h=%.0f\n",
                        tex->image->getImageGradient().x(),
                        tex->image->getImageGradient().y(),
                        tex->image->getImageGradient().z(),
                        tex->image->getMapType(),
                        tex->image->getInterpolationType(),
                        tex->image->getOnceFlag(),
                        (double)tex->image->getXSize(),
                        (double)tex->image->getYSize());
            }

            if (tex->textureTransformation) {
                Matrix4x4d *xform = tex->textureTransformation;
                fprintf(f, "  tex.xform=");
                for (int i = 0; i < 4; i++) {
                    for (int j = 0; j < 4; j++) {
                        fprintf(f, "%.6f", xform->get(i, j));
                        if (i < 3 || j < 3) fprintf(f, ",");
                    }
                }
                fprintf(f, "\n");
            }
        }
        idx++;
    }
}
