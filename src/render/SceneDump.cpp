#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"

#include "environment/material/PovrayMaterial.h"

#include "environment/geometry/BoundedGeometry.h"

#include "render/RenderEngine.h"
#include "render/SceneDump.h"

void SceneDumper::dumpSceneStructure(FILE *f)
{
    if (f == nullptr) return;

    int idx = 0;
    java::ArrayList<BoundedGeometry*> &sceneObjects = RenderEngine::renderFrame().Objects;
    for (long int i = sceneObjects.size() - 1; i >= 0; i--) {
        const BoundedGeometry *obj = sceneObjects[i];
        fprintf(f, "OBJ %03d\n", idx);

        if (obj->getObjectTexture()) {
            const PovrayMaterial * const tex =
                static_cast<const PovrayMaterial *>(obj->getObjectTexture());

            fprintf(f, "  tex.num=%d amb=%.6f dif=%.6f phong=%.6f psize=%.6f bril=%.6f\n",
                    tex->getTextureNumber(),
                    tex->getObjectAmbient(),
                    tex->getObjectDiffuse(),
                    tex->getObjectPhong(),
                    tex->getObjectPhongSize(),
                    tex->getObjectBrilliance());

            fprintf(f, "  tex.spec=%.6f rough=%.6f refl=%.6f refr=%.6f ior=%.6f transmit=%.6f metal=%d\n",
                    tex->getObjectSpecular(),
                    tex->getObjectRoughness(),
                    tex->getObjectReflection(),
                    tex->getObjectRefraction(),
                    tex->getObjectIndexOfRefraction(),
                    tex->getObjectTransmit(),
                    tex->isMetallic());

            fprintf(f, "  tex.turb=%.6f oct=%d freq=%.6f phase=%.6f mortar=%.6f bump=%d/%.6f\n",
                    tex->getTurbulence(),
                    tex->getOctaves(),
                    tex->getFrequency(),
                    tex->getPhase(),
                    tex->getMortar(),
                    tex->getBumpNumber(),
                    tex->getBumpAmount());

            fprintf(f, "  tex.grad=%.6f,%.6f,%.6f\n",
                    tex->getTextureGradient().x(),
                    tex->getTextureGradient().y(),
                    tex->getTextureGradient().z());

            if (tex->getColor1()) {
                fprintf(f, "  tex.c1=%.6f,%.6f,%.6f,%.6f",
                        tex->getColor1()->getR(),
                        tex->getColor1()->getG(),
                        tex->getColor1()->getB(),
                        tex->getColor1()->getA());
            } else {
                fprintf(f, "  tex.c1=none");
            }

            if (tex->getColor2()) {
                fprintf(f, " c2=%.6f,%.6f,%.6f,%.6f",
                        tex->getColor2()->getR(),
                        tex->getColor2()->getG(),
                        tex->getColor2()->getB(),
                        tex->getColor2()->getA());
            } else {
                fprintf(f, " c2=none");
            }
            fprintf(f, "\n");

            if (tex->getColorMap()) {
                fprintf(f, "  tex.cmap n=%d", tex->getColorMap()->size());
                for (int i = 0; i < tex->getColorMap()->size(); i++) {
                    const ColorRgba *c = tex->getColorMap()->getColorAt(i);
                    if (tex->getColorMap()->hasPositions()) {
                        fprintf(f, " [%.6f %.3f,%.3f,%.3f,%.3f]",
                                tex->getColorMap()->getPositionAt(i),
                                c->getR(), c->getG(), c->getB(), c->getA());
                    } else {
                        fprintf(f, " [%.3f,%.3f,%.3f,%.3f]",
                                c->getR(), c->getG(), c->getB(), c->getA());
                    }
                    delete c;
                }
                fprintf(f, "\n");
            }

            if (tex->getImage()) {
                fprintf(f, "  tex.img grad=(%.6f,%.6f,%.6f) map=%d interp=%d once=%d w=%.0f h=%.0f\n",
                        tex->getImage()->getImageGradient().x(),
                        tex->getImage()->getImageGradient().y(),
                        tex->getImage()->getImageGradient().z(),
                        tex->getImage()->getMapType(),
                        tex->getImage()->getInterpolationType(),
                        tex->getImage()->getOnceFlag(),
                        (double)tex->getImage()->getXSize(),
                        (double)tex->getImage()->getYSize());
            }

            if (tex->getTextureTransformation()) {
                const Matrix4x4d *xform = tex->getTextureTransformation();
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
