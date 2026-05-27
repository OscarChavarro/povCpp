#include <iostream>
#include <memory>
#include <string>
#include <stdexcept>

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
#elif defined(POV_ANTLR_MINIMAL_BRIDGE)
    std::cerr << "ANTLR lowering-negative test requires full lowering mode (no minimal bridge).\n";
    return 2;
#else
    auto parseProgram = [](const std::string &text) -> std::unique_ptr<AntlrParsedSceneProgram> {
        antlr4::ANTLRInputStream stream(text);
        POVLexer lexer(&stream);
        antlr4::CommonTokenStream tokens(&lexer);
        POVParser parser(&tokens);
        POVParser::SceneContext *sceneCtx = parser.scene();
        if (sceneCtx == nullptr || parser.getNumberOfSyntaxErrors() > 0) {
            return nullptr;
        }
        return std::unique_ptr<AntlrParsedSceneProgram>(AntlrParseTreeToIrMapper::mapScene(sceneCtx));
    };


    auto expectLoweringFailure =
        [&](const std::string &name, const std::string &text, const std::string &messageNeedle) -> bool {
        std::unique_ptr<AntlrParsedSceneProgram> parsed = parseProgram(text);
        if (!parsed || parsed->program == nullptr) {
            std::cerr << "Expected parse success before lowering failure in " << name << ".\n";
            return false;
        }
        RenderFrame frame;
        frame.viewPoint.initializeDefaults();
        try {
            AntlrSceneLowering::applyProgram(*parsed->program, &frame);
        } catch (const std::runtime_error &e) {
            if (!messageNeedle.empty() &&
                std::string(e.what()).find(messageNeedle) == std::string::npos) {
                AntlrSceneIrNodes::destroyProgram(parsed->program);
                parsed->program = nullptr;
                std::cerr << "Unexpected lowering error in " << name << ": " << e.what() << "\n";
                return false;
            }
            AntlrSceneIrNodes::destroyProgram(parsed->program);
            parsed->program = nullptr;
            return true;
        } catch (...) {
            AntlrSceneIrNodes::destroyProgram(parsed->program);
            parsed->program = nullptr;
            std::cerr << "Unexpected non-runtime exception in " << name << ".\n";
            return false;
        }
        AntlrSceneIrNodes::destroyProgram(parsed->program);
        parsed->program = nullptr;
        std::cerr << "Expected lowering failure in " << name << ".\n";
        return false;
    };

    const std::string invalidLightNoBase =
        "camera { location <0,0,-5> look_at <0,0,0> }\n"
        "light_source { colour <1,1,1> }\n";
    const std::string invalidEmptyCsg = "difference { }\n";
    const std::string invalidUnknownLightReference = "light_source { UnknownLightRef }\n";
    const std::string invalidUnknownObjectReference = "object { UnknownObjectRef }\n";
    const std::string invalidUnknownCompositeReference = "composite { UnknownCompositeRef }\n";
    const std::string invalidUnknownCameraReference =
        "camera { CamUnknownRef }\n";

    if (!expectLoweringFailure(
            "invalid-light-no-base", invalidLightNoBase, "requires inline center or reference")) {
        return 1;
    }
    if (!expectLoweringFailure(
            "invalid-empty-csg", invalidEmptyCsg, "requires at least one child or reference")) {
        return 1;
    }
    if (!expectLoweringFailure(
            "invalid-unknown-light-reference", invalidUnknownLightReference, "Unknown ANTLR light reference")) {
        return 1;
    }
    if (!expectLoweringFailure(
            "invalid-unknown-object-reference", invalidUnknownObjectReference, "Unknown ANTLR object reference")) {
        return 1;
    }
    if (!expectLoweringFailure(
            "invalid-unknown-composite-reference", invalidUnknownCompositeReference,
            "Unknown ANTLR composite reference")) {
        return 1;
    }
    if (!expectLoweringFailure(
            "invalid-unknown-camera-reference", invalidUnknownCameraReference,
            "Unknown ANTLR camera reference")) {
        return 1;
    }
    std::cout << "ANTLR lowering-negative harness passed (full lowering mode).\n";
    return 0;
#endif
}
