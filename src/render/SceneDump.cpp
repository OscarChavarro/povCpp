#include "render/SceneDump.h"
#include "render/RenderEngine.h"
#include "environment/geometry/SimpleBody.h"
#include "media/Texture.h"
#include "common/linealAlgebra/Transformation.h"
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
                    tex->Turbulence,
                    tex->Octaves,
                    tex->Frequency,
                    tex->Phase,
                    tex->Mortar,
                    tex->bumpNumber,
                    tex->bumpAmount);

            fprintf(f, "  tex.grad=%.6f,%.6f,%.6f\n",
                    tex->textureGradient.x,
                    tex->textureGradient.y,
                    tex->textureGradient.z);

            if (tex->Colour1) {
                fprintf(f, "  tex.c1=%.6f,%.6f,%.6f,%.6f",
                        tex->Colour1->Red,
                        tex->Colour1->Green,
                        tex->Colour1->Blue,
                        tex->Colour1->Alpha);
            } else {
                fprintf(f, "  tex.c1=none");
            }

            if (tex->Colour2) {
                fprintf(f, " c2=%.6f,%.6f,%.6f,%.6f",
                        tex->Colour2->Red,
                        tex->Colour2->Green,
                        tex->Colour2->Blue,
                        tex->Colour2->Alpha);
            } else {
                fprintf(f, " c2=none");
            }
            fprintf(f, "\n");

            if (tex->Colour_Map) {
                fprintf(f, "  tex.cmap n=%d", tex->Colour_Map->numberOfEntries);
                for (int i = 0; i < tex->Colour_Map->numberOfEntries; i++) {
                    RGBAColorPaletteSpan &entry = tex->Colour_Map->Colour_Map_Entries[i];
                    fprintf(f, " [%.6f %.6f %.3f,%.3f,%.3f,%.3f->%.3f,%.3f,%.3f,%.3f]",
                            entry.start,
                            entry.end,
                            entry.startColour.Red,
                            entry.startColour.Green,
                            entry.startColour.Blue,
                            entry.startColour.Alpha,
                            entry.endColour.Red,
                            entry.endColour.Green,
                            entry.endColour.Blue,
                            entry.endColour.Alpha);
                }
                fprintf(f, "\n");
            }

            if (tex->Image) {
                fprintf(f, "  tex.img grad=(%.6f,%.6f,%.6f) map=%d interp=%d once=%d w=%.0f h=%.0f\n",
                        tex->Image->imageGradient.x,
                        tex->Image->imageGradient.y,
                        tex->Image->imageGradient.z,
                        tex->Image->mapType,
                        tex->Image->interpolationType,
                        tex->Image->onceFlag,
                        tex->Image->width,
                        tex->Image->height);
            }

            if (tex->Texture_Transformation) {
                Transformation *xform = tex->Texture_Transformation;
                fprintf(f, "  tex.xform=");
                for (int i = 0; i < 4; i++) {
                    for (int j = 0; j < 4; j++) {
                        fprintf(f, "%.6f", xform->matrix[i][j]);
                        if (i < 3 || j < 3) fprintf(f, ",");
                    }
                }
                fprintf(f, "\n");
            }
        }
        idx++;
    }
}
