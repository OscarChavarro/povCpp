#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "environment/material/RenderRuntimeState.h"
#include "io/pov/antlr/AntlrParsedSceneProgram.h"
#include "io/pov/antlr/AntlrParseTreeToIrMapper.h"
#include "io/pov/antlr/AntlrSceneIr.h"
#include "io/pov/antlr/AntlrSceneLowering.h"
#include "render/RenderFrame.h"

#ifdef POV_WITH_ANTLR_RUNTIME
#include "POVLexer.h"
#include "POVParser.h"
#include "antlr4-runtime.h"
#endif

int main()
{
#ifndef POV_WITH_ANTLR_RUNTIME
    std::cerr << "ANTLR runtime not enabled.\n";
    return 2;
#else
    const std::string input =
        "#declare TexA = texture { wood }\n"
        "#declare SRef = sphere { <2,2,2> 1 }\n"
        "#declare PRef = plane { <0,1,0> -1 }\n"
        "#declare BRef = box { <-1,-1,-1> <1,1,1> }\n"
        "#declare TRef = triangle { <0,0,0> <1,0,0> <0,1,0> }\n"
        "#declare STRef = smooth_triangle { <0,0,0> <0,0,1> <1,0,0> <0,0,1> <0,1,0> <0,0,1> }\n"
        "#declare QRef = quadric { <1,1,1> <0,0,0> <0,0,0> -1 }\n"
        "#declare BlRef = blob { threshold 1 component 1 1 <0,0,0> }\n"
        "#declare ORef = object { SRef }\n"
        "#declare CRef = composite { sphere { <1,1,1> 1 } }\n"
        "#declare LRef = light_source { <0,5,-5> colour <1,1,1> }\n"
        "#declare URef = union { sphere { <0,0,0> 1 } sphere { <1,0,0> 1 } }\n"
        "#declare CamRef = camera { location <0,0,-5> look_at <0,0,0> }\n"
        "default { texture { TexA } }\n"
        "plane { PRef texture { TexA } }\n"
        "box { BRef texture { TexA } }\n"
        "triangle { TRef texture { TexA } }\n"
        "smooth_triangle { STRef texture { TexA } }\n"
        "quadric { QRef texture { TexA } }\n"
        "blob { BlRef texture { TexA } }\n"
        "sphere { SRef texture { TexA } }\n"
        "object { ORef }\n"
        "composite { CRef }\n"
        "composite { ORef CRef URef }\n"
        "light_source { LRef spotlight point_at <0,0,0> }\n"
        "difference { URef sphere { <0.5,0,0> 0.5 } }\n"
        "sphere { <1,2,3> 4 colour <0.5,0.6,0.7> texture { TexA } translate <1,0,0> }\n"
        "object { sphere { <0,0,0> 1 } no_shadow translate <0,1,0> texture { TexA } }\n"
        "object { object { sphere { <0,0,0> 1 } } }\n"
        "object { composite { sphere { <0,0,0> 1 } } }\n"
        "object { sphere { <0,0,1> 1 } inverse }\n"
        "object { sphere { <0,0,0> 1 } bounded_by { sphere { <0,0,0> 2 } } clipped_by { sphere { <0,0,0> 1.5 } } }\n"
        "composite { sphere { <0,0,0> 1 } bounded_by { sphere { <0,0,0> 3 } } clipped_by { sphere { <0,0,0> 2.5 } } }\n"
        "composite { sphere { <0,0,0> 2 } rotate <0,45,0> }\n"
        "max_trace_level 7\n"
        "fog { colour <0.1,0.2,0.3> 12 }\n"
        "camera { CamRef }\n";

    antlr4::ANTLRInputStream stream(input);
    POVLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    POVParser parser(&tokens);
    POVParser::SceneContext *sceneCtx = parser.scene();
    if (sceneCtx == nullptr) {
        std::cerr << "ANTLR scene parse returned null context.\n";
        return 1;
    }

    std::unique_ptr<AntlrParsedSceneProgram> parsed(
        AntlrParseTreeToIrMapper::mapScene(sceneCtx));
    if (!parsed || parsed->program == nullptr) {
        std::cerr << "Mapper returned null IR program.\n";
        return 1;
    }

    bool foundMaxTrace = false;
    bool foundFog = false;
    bool foundCamera = false;
    bool foundDeclareTexture = false;
    bool foundDefaultTexture = false;
    bool foundSphere = false;
    bool foundPlane = false;
    bool foundBox = false;
    bool foundTriangle = false;
    bool foundSmoothTriangle = false;
    bool foundQuadric = false;
    bool foundBlob = false;
    bool foundObject = false;
    bool foundComposite = false;
    bool foundLight = false;
    bool foundCsg = false;
    for (int i = 0; i < parsed->program->nodeCount; ++i) {
        AntlrSceneIrNode *n = parsed->program->nodes[i];
        if (n == nullptr) {
            continue;
        }
        if (n->kind == ANTLR_IR_MAX_TRACE_LEVEL_NODE) {
            foundMaxTrace = true;
        } else if (n->kind == ANTLR_IR_FOG_NODE) {
            foundFog = true;
        } else if (n->kind == ANTLR_IR_CAMERA_NODE) {
            foundCamera = true;
        } else if (n->kind == ANTLR_IR_DECLARE_NODE) {
            AntlrIrDeclareNode *d = (AntlrIrDeclareNode *)n;
            if (d->identifier == "TexA" && d->hasTextureChainValue &&
                !d->textureChainValue.rawElements.empty()) {
                foundDeclareTexture = true;
            }
        } else if (n->kind == ANTLR_IR_DEFAULT_TEXTURE_NODE) {
            AntlrIrDefaultTextureNode *d = (AntlrIrDefaultTextureNode *)n;
            if (d->hasTextureChain && !d->textureChain.rawElements.empty()) {
                foundDefaultTexture = true;
            }
        } else if (n->kind == ANTLR_IR_SPHERE_NODE) {
            AntlrIrSphereNode *s = (AntlrIrSphereNode *)n;
            if (s->hasInlineBase && s->radius == 4.0 &&
                s->hasColour && s->hasTextureChain && s->transformCount > 0) {
                foundSphere = true;
            }
        } else if (n->kind == ANTLR_IR_PLANE_NODE) {
            AntlrIrPlaneNode *p = (AntlrIrPlaneNode *)n;
            if ((p->hasInlineBase || p->hasReferenceBase) && p->hasTextureChain) {
                foundPlane = true;
            }
        } else if (n->kind == ANTLR_IR_BOX_NODE) {
            AntlrIrBoxNode *b = (AntlrIrBoxNode *)n;
            if ((b->hasInlineBase || b->hasReferenceBase) && b->hasTextureChain) {
                foundBox = true;
            }
        } else if (n->kind == ANTLR_IR_TRIANGLE_NODE) {
            AntlrIrTriangleNode *t = (AntlrIrTriangleNode *)n;
            if ((t->hasInlineBase || t->hasReferenceBase) && t->hasTextureChain) {
                foundTriangle = true;
            }
        } else if (n->kind == ANTLR_IR_SMOOTH_TRIANGLE_NODE) {
            AntlrIrSmoothTriangleNode *t = (AntlrIrSmoothTriangleNode *)n;
            if ((t->hasInlineBase || t->hasReferenceBase) && t->hasTextureChain) {
                foundSmoothTriangle = true;
            }
        } else if (n->kind == ANTLR_IR_QUADRIC_NODE) {
            AntlrIrQuadricNode *q = (AntlrIrQuadricNode *)n;
            if ((q->hasInlineBase || q->hasReferenceBase) && q->hasTextureChain) {
                foundQuadric = true;
            }
        } else if (n->kind == ANTLR_IR_BLOB_NODE) {
            AntlrIrBlobNode *b = (AntlrIrBlobNode *)n;
            if ((b->hasInlineBase || b->hasReferenceBase) && b->hasTextureChain) {
                foundBlob = true;
            }
        } else if (n->kind == ANTLR_IR_OBJECT_NODE) {
            AntlrIrObjectNode *o = (AntlrIrObjectNode *)n;
            if (o->childSphereCount > 0 && o->noShadow &&
                o->transformCount > 0 && o->hasTextureChain) {
                foundObject = true;
            } else if (o->inverted) {
                foundObject = true;
            }
        } else if (n->kind == ANTLR_IR_COMPOSITE_NODE) {
            AntlrIrCompositeNode *c = (AntlrIrCompositeNode *)n;
            if (c->childSphereCount > 0 && c->transformCount > 0) {
                foundComposite = true;
            } else if (c->childReferenceCount >= 3) {
                foundComposite = true;
            }
        } else if (n->kind == ANTLR_IR_LIGHT_NODE) {
            AntlrIrLightNode *l = (AntlrIrLightNode *)n;
            if ((l->hasCenter || l->hasReference) && (l->spotlight || l->hasPointAt)) {
                foundLight = true;
            }
        } else if (n->kind == ANTLR_IR_CSG_NODE) {
            AntlrIrCsgNode *csg = (AntlrIrCsgNode *)n;
            if (csg->childSphereCount > 0 || csg->hasReference) {
                foundCsg = true;
            }
        }
    }

    if (!foundMaxTrace || !foundFog || !foundCamera ||
        !foundDeclareTexture || !foundDefaultTexture || !foundSphere ||
        !foundPlane || !foundBox || !foundTriangle || !foundSmoothTriangle ||
        !foundQuadric || !foundBlob || !foundObject || !foundComposite || !foundLight || !foundCsg) {
        std::cerr << "IR smoke check failed."
                  << " max=" << foundMaxTrace
                  << " fog=" << foundFog
                  << " camera=" << foundCamera
                  << " declare=" << foundDeclareTexture
                  << " default=" << foundDefaultTexture
                  << " sphere=" << foundSphere
                  << " plane=" << foundPlane
                  << " box=" << foundBox
                  << " triangle=" << foundTriangle
                  << " smooth_triangle=" << foundSmoothTriangle
                  << " quadric=" << foundQuadric
                  << " blob=" << foundBlob
                  << " object=" << foundObject
                  << " composite=" << foundComposite
                  << " light=" << foundLight
                  << " csg=" << foundCsg << "\n";
        return 1;
    }

    RenderFrame frame;
    frame.viewPoint.initializeDefaults();
    frame.fogDistance = 0.0;
    RenderRuntimeState::maxTraceLevel() = 0.0;
    AntlrSceneLowering::applyProgram(*parsed->program, &frame);
    if (RenderRuntimeState::maxTraceLevel() != 7.0 ||
        frame.fogDistance != 12.0 ||
        frame.viewPoint.Location.x != 0.0 || frame.viewPoint.Location.y != 0.0 ||
        frame.viewPoint.Location.z != -5.0) {
        std::cerr << "ANTLR lowering smoke check failed.\n";
        return 1;
    }

    AntlrSceneIrNodes::destroyProgram(parsed->program);
    parsed->program = nullptr;

    std::cout << "ANTLR smoke test passed.\n";
    return 0;
#endif
}
