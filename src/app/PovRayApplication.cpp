#include "app/ImageOutputAdapter.h"
#include "app/PovRayApplication.h"
#include "app/options/CommandLineOptions.h"
#include "common/statistics/Statistics.h"
#include "environment/material/povray/PovRayMaterial.h"
#include "environment/material/RenderOutput.h"
#include "environment/material/PovRayRendererConfiguration.h"
#include "environment/scene/Scene.h"
#include "io/image/RawDumpFormat.h"
#include "io/image/RawFormat.h"
#include "io/image/TargaFormat.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/material/LoadedImageRegistry.h"
#include "io/pov/scene/SceneParser.h"
#include "render/bakedScene/BakedTracingCommon.h"
#include "vsdk/toolkit/common/logging/Logger.h"

void
PovRayApplication::printProgress(const char *message)
{
    Logger::reportMessage("PovRayApplication", Logger::WARNING, "", message);
}

void
PovRayApplication::printStatistics(
    const Statistics &stats,
    const Scene &frame,
    const PovRayRendererConfiguration &inputConfiguration)
{
    char buffer[1024];
    const long pixelsInImage =
        (long)frame.getScreenWidth() * (long)frame.getScreenHeight();

    snprintf(buffer, sizeof(buffer), "\n%s statistics",
        inputConfiguration.getInputFileName());
    printProgress(buffer);

    if (pixelsInImage > stats.getNumberOfPixels()) {
        printProgress("  Partial Image Rendered");
    }

    printProgress("--------------------------------------");
    snprintf(buffer, sizeof(buffer), "Resolution %d x %d", frame.getScreenWidth(),
        frame.getScreenHeight());
    printProgress(buffer);

    snprintf(buffer, sizeof(buffer),
        "# Rays:  %10ld     # Pixels:  %10ld  # Pixels super-sampled: %10ld",
        stats.getNumberOfRays(), stats.getNumberOfPixels(),
        stats.getNumberOfPixelsSuperSampled());
    printProgress(buffer);

    printProgress("  Ray->Shape Intersection Tests:");
    printProgress("    type                 Tests     Succeeded    Percentage");
    printProgress("  -----------------------------------------------------------");

    auto logIntersectionRow = [this, &buffer](const char *label, long tests, long succeeded) {
        if (tests) {
            snprintf(buffer, sizeof(buffer), "%s %10ld  %10ld  %10.2f", label, tests, succeeded,
                (((double)succeeded / (double)tests) * 100.0));
            printProgress(buffer);
        }
    };

    logIntersectionRow("  Sphere        ", stats.getRaySphereTests(), stats.getRaySphereTestsSucceeded());
    logIntersectionRow("  Plane         ", stats.getRayPlaneTests(), stats.getRayPlaneTestsSucceeded());
    logIntersectionRow("  Triangle     ", stats.getRayTriangleTests(), stats.getRayTriangleTestsSucceeded());
    logIntersectionRow("  Quadric       ", stats.getRayQuadricTests(), stats.getRayQuadricTestsSucceeded());
    logIntersectionRow("  Blob          ", stats.getRayBlobTests(), stats.getRayBlobTestsSucceeded());
    logIntersectionRow("  Box           ", stats.getRayBoxTests(), stats.getRayBoxTestsSucceeded());
    logIntersectionRow("  Quartic\\Poly ", stats.getRayPolyTests(), stats.getRayPolyTestsSucceeded());
    logIntersectionRow("  Bezier Patch ", stats.getRayBicubicTests(), stats.getRayBicubicTestsSucceeded());
    logIntersectionRow("  Height field   ", stats.getRayHtFieldTests(), stats.getRayHtFieldTestsSucceeded());
    logIntersectionRow("  Bounds        ", stats.getBoundingRegionTests(), stats.getBoundingRegionTestsSucceeded());
    logIntersectionRow("  Clips         ", stats.getClippingRegionTests(), stats.getClippingRegionTestsSucceeded());

    if (stats.getSolidTextureStatistics()->callsToNoise) {
        snprintf(buffer, sizeof(buffer), "  Calls to Noise:    %10ld", stats.getSolidTextureStatistics()->callsToNoise);
        printProgress(buffer);
    }
    if (stats.getSolidTextureStatistics()->callsToDNoise) {
        snprintf(buffer, sizeof(buffer), "  Calls to DNoise:  %10ld", stats.getSolidTextureStatistics()->callsToDNoise);
        printProgress(buffer);
    }
    if (stats.getShadowRayTests()) {
        snprintf(buffer, sizeof(buffer),
            "  Shadow Ray Tests: %10ld      Blocking Objects Found:  %10ld",
            stats.getShadowRayTests(), 0L);
        printProgress(buffer);
    }
    if (stats.getReflectedRaysTraced()) {
        snprintf(buffer, sizeof(buffer), "  Reflected Rays:    %10ld", stats.getReflectedRaysTraced());
        printProgress(buffer);
    }
    if (stats.getRefractedRaysTraced()) {
        snprintf(buffer, sizeof(buffer), "  Refracted Rays:    %10ld", stats.getRefractedRaysTraced());
        printProgress(buffer);
    }
    if (stats.getTransmittedRaysTraced()) {
        snprintf(buffer, sizeof(buffer), "  Transmitted Rays: %10ld", stats.getTransmittedRaysTraced());
        printProgress(buffer);
    }

    const BakedTracingCommon::FallbackCounters fallbackCounters =
        BakedTracingCommon::getFallbackCounters();
    snprintf(buffer, sizeof(buffer),
        "  Baked fallbacks: first-hit %ld  all-crossings %ld  containment %ld",
        fallbackCounters.firstHitObjectFallbacks,
        fallbackCounters.allCrossingsObjectFallbacks,
        fallbackCounters.containmentObjectFallbacks);
    printProgress(buffer);

    long simpleDirect = 0;
    long simpleTransformed = 0;
    long simpleBoundedOrClipped = 0;
    long simpleCsg = 0;
    long simpleGeneric = 0;
    for (long int i = 0; i < frame.getBakedSimpleBodies().size(); i++) {
        switch (frame.getBakedSimpleBodies()[i].executionKind) {
        case Scene::BakedSimpleBodyExecutionKind::DirectPrimitive:
            simpleDirect++;
            break;
        case Scene::BakedSimpleBodyExecutionKind::TransformedPrimitive:
            simpleTransformed++;
            break;
        case Scene::BakedSimpleBodyExecutionKind::BoundedOrClippedPrimitive:
        case Scene::BakedSimpleBodyExecutionKind::BoundedOrClippedCsg:
            simpleBoundedOrClipped++;
            break;
        case Scene::BakedSimpleBodyExecutionKind::DirectCsg:
        case Scene::BakedSimpleBodyExecutionKind::TransformedCsg:
            simpleCsg++;
            break;
        case Scene::BakedSimpleBodyExecutionKind::GenericFallback:
        case Scene::BakedSimpleBodyExecutionKind::Empty:
            simpleGeneric++;
            break;
        }
    }
    snprintf(buffer, sizeof(buffer),
        "  Baked simple bodies: direct %ld  transformed %ld  bounded/clipped %ld  csg %ld  generic %ld",
        simpleDirect,
        simpleTransformed,
        simpleBoundedOrClipped,
        simpleCsg,
        simpleGeneric);
    printProgress(buffer);

    long operandDirect = 0;
    long operandTransformed = 0;
    long operandTransformedQuadric = 0;
    long operandTransformedPrimitive = 0;
    long operandPlane = 0;
    long operandDirectPlane = 0;
    long operandTransformedPlane = 0;
    long operandNested = 0;
    long operandDirectNested = 0;
    long operandTransformedNested = 0;
    long transformedNestedGenericMorgan = 0;
    long transformedNestedSingleCorePlane = 0;
    long transformedNestedCoreDirectAnnotated = 0;
    long transformedNestedCoreDirectPrimitive = 0;
    long transformedNestedCoreTransformedQuadric = 0;
    long transformedNestedCoreOther = 0;
    long transformedNestedCompiledCorePlane = 0;
    long transformedNestedCompiledDirectQuadric = 0;
    long transformedNestedCompiledTransformedQuadric = 0;
    long transformedNestedCompiledPlaneRoles = 0;
    long transformedNestedCompiledContainmentRoles = 0;
    long transformedNestedOtherPlan = 0;
    long operandGeneric = 0;
    long operandCollapsedQuadric = 0;
    long operandCollapsedPlane = 0;
    for (long int i = 0; i < frame.getBakedCsgs().size(); i++) {
        const Scene::BakedConstructiveSolidGeometry &bakedCsg =
            frame.getBakedCsgs()[i];
        for (long int j = 0; j < bakedCsg.operands.size(); j++) {
            if (bakedCsg.operands[j].hasBakedQuadric) {
                operandCollapsedQuadric++;
            }
            if (bakedCsg.operands[j].hasBakedPlane) {
                operandCollapsedPlane++;
            }
            switch (bakedCsg.operands[j].executionKind) {
            case Scene::BakedCsgOperandExecutionKind::DirectAnnotatedPrimitive:
            case Scene::BakedCsgOperandExecutionKind::DirectPrimitive:
                operandDirect++;
                break;
            case Scene::BakedCsgOperandExecutionKind::TransformedQuadric:
                operandTransformed++;
                operandTransformedQuadric++;
                break;
            case Scene::BakedCsgOperandExecutionKind::TransformedSphere:
                operandTransformed++;
                operandTransformedPrimitive++;
                break;
            case Scene::BakedCsgOperandExecutionKind::TransformedPrimitive:
                operandTransformed++;
                operandTransformedPrimitive++;
                break;
            case Scene::BakedCsgOperandExecutionKind::DirectPlane:
                operandPlane++;
                operandDirectPlane++;
                break;
            case Scene::BakedCsgOperandExecutionKind::TransformedPlane:
                operandPlane++;
                operandTransformedPlane++;
                break;
            case Scene::BakedCsgOperandExecutionKind::NestedCsg:
                operandNested++;
                operandDirectNested++;
                break;
            case Scene::BakedCsgOperandExecutionKind::TransformedNestedCsg:
                operandNested++;
                operandTransformedNested++;
                if (bakedCsg.operands[j].compiledTransformedNestedCorePlane) {
                    transformedNestedCompiledCorePlane++;
                    transformedNestedCompiledPlaneRoles +=
                        bakedCsg.operands[j].compiledNestedPlaneOperandIndices.size();
                    transformedNestedCompiledContainmentRoles +=
                        bakedCsg.operands[j].
                            compiledNestedContainmentOperandIndices.size();
                    if (bakedCsg.operands[j].compiledNestedCoreDirectQuadric) {
                        transformedNestedCompiledDirectQuadric++;
                    }
                    if (bakedCsg.operands[j].compiledNestedCoreTransformedQuadric) {
                        transformedNestedCompiledTransformedQuadric++;
                    }
                }
                if (bakedCsg.operands[j].bakedCsgIndex >= 0 &&
                    bakedCsg.operands[j].bakedCsgIndex < frame.getBakedCsgs().size()) {
                    const Scene::BakedConstructiveSolidGeometry &nestedCsg =
                        frame.getBakedCsgs()[bakedCsg.operands[j].bakedCsgIndex];
                    if (nestedCsg.executionPlanKind ==
                        Scene::BakedCsgExecutionPlanKind::GenericMorgan) {
                        transformedNestedGenericMorgan++;
                    } else if (nestedCsg.executionPlanKind ==
                        Scene::BakedCsgExecutionPlanKind::SingleCorePlaneIntersection) {
                        transformedNestedSingleCorePlane++;
                        const long int coreIndex =
                            nestedCsg.specializationCoreOperandIndex;
                        if (coreIndex >= 0 && coreIndex < nestedCsg.operands.size()) {
                            switch (nestedCsg.operands[coreIndex].executionKind) {
                            case Scene::BakedCsgOperandExecutionKind::DirectAnnotatedPrimitive:
                                transformedNestedCoreDirectAnnotated++;
                                break;
                            case Scene::BakedCsgOperandExecutionKind::DirectPrimitive:
                                transformedNestedCoreDirectPrimitive++;
                                break;
                            case Scene::BakedCsgOperandExecutionKind::TransformedQuadric:
                                transformedNestedCoreTransformedQuadric++;
                                break;
                            default:
                                transformedNestedCoreOther++;
                                break;
                            }
                        }
                    } else {
                        transformedNestedOtherPlan++;
                    }
                }
                break;
            case Scene::BakedCsgOperandExecutionKind::GenericFallback:
            case Scene::BakedCsgOperandExecutionKind::Empty:
                operandGeneric++;
                break;
            }
        }
    }
    snprintf(buffer, sizeof(buffer),
        "  Baked CSG operands: direct %ld  transformed %ld  plane %ld  nested %ld  generic %ld",
        operandDirect,
        operandTransformed,
        operandPlane,
        operandNested,
        operandGeneric);
    printProgress(buffer);
    snprintf(buffer, sizeof(buffer),
        "  Baked CSG operand detail: transformed-quadric %ld  transformed-primitive %ld  direct-plane %ld  transformed-plane %ld  direct-nested %ld  transformed-nested %ld",
        operandTransformedQuadric,
        operandTransformedPrimitive,
        operandDirectPlane,
        operandTransformedPlane,
        operandDirectNested,
        operandTransformedNested);
    printProgress(buffer);
    snprintf(buffer, sizeof(buffer),
        "  Baked CSG coefficient collapse (Plan 5 Phase 3): quadric %ld  plane %ld  "
        "(these count against direct/direct-plane above, not transformed-quadric/transformed-plane)",
        operandCollapsedQuadric,
        operandCollapsedPlane);
    printProgress(buffer);
    snprintf(buffer, sizeof(buffer),
        "  Baked transformed-nested refs: generic-morgan %ld  core-plane %ld  other %ld",
        transformedNestedGenericMorgan,
        transformedNestedSingleCorePlane,
        transformedNestedOtherPlan);
    printProgress(buffer);
    snprintf(buffer, sizeof(buffer),
        "  Baked transformed-nested core-plane cores: direct-annotated %ld  direct %ld  transformed-quadric %ld  other %ld",
        transformedNestedCoreDirectAnnotated,
        transformedNestedCoreDirectPrimitive,
        transformedNestedCoreTransformedQuadric,
        transformedNestedCoreOther);
    printProgress(buffer);
    snprintf(buffer, sizeof(buffer),
        "  Baked transformed-nested compiled descriptors: core-plane %ld  direct-quadric %ld  transformed-quadric %ld  plane-roles %ld  containment-roles %ld",
        transformedNestedCompiledCorePlane,
        transformedNestedCompiledDirectQuadric,
        transformedNestedCompiledTransformedQuadric,
        transformedNestedCompiledPlaneRoles,
        transformedNestedCompiledContainmentRoles);
    printProgress(buffer);

    long planGenericMorgan = 0;
    long planGenericRaySegments = 0;
    long planTopLevelPlaneUnion = 0;
    long planDisjointBoundedUnion = 0;
    long planSingleCorePlaneIntersection = 0;
    long planFallback = 0;
    long planPlaneRoles = 0;
    long planNestedRoles = 0;
    long planTransformedPrimitiveRoles = 0;
    long planDirectPrimitiveRoles = 0;
    for (long int i = 0; i < frame.getBakedCsgs().size(); i++) {
        const Scene::BakedConstructiveSolidGeometry &bakedCsg =
            frame.getBakedCsgs()[i];
        switch (bakedCsg.executionPlanKind) {
        case Scene::BakedCsgExecutionPlanKind::GenericMorgan:
            planGenericMorgan++;
            break;
        case Scene::BakedCsgExecutionPlanKind::GenericRaySegments:
            planGenericRaySegments++;
            break;
        case Scene::BakedCsgExecutionPlanKind::TopLevelPlaneUnion:
            planTopLevelPlaneUnion++;
            break;
        case Scene::BakedCsgExecutionPlanKind::DisjointBoundedUnion:
            planDisjointBoundedUnion++;
            break;
        case Scene::BakedCsgExecutionPlanKind::SingleCorePlaneIntersection:
            planSingleCorePlaneIntersection++;
            break;
        case Scene::BakedCsgExecutionPlanKind::Fallback:
            planFallback++;
            break;
        }
        planPlaneRoles += bakedCsg.executionPlanPlaneOperandIndices.size();
        planNestedRoles += bakedCsg.executionPlanNestedOperandIndices.size();
        planTransformedPrimitiveRoles +=
            bakedCsg.executionPlanTransformedPrimitiveOperandIndices.size();
        planDirectPrimitiveRoles +=
            bakedCsg.executionPlanDirectPrimitiveOperandIndices.size();
    }
    snprintf(buffer, sizeof(buffer),
        "  Baked CSG plans: generic-morgan %ld  ray-segments %ld  plane-union %ld  disjoint-union %ld  core-plane %ld  fallback %ld",
        planGenericMorgan,
        planGenericRaySegments,
        planTopLevelPlaneUnion,
        planDisjointBoundedUnion,
        planSingleCorePlaneIntersection,
        planFallback);
    printProgress(buffer);
    snprintf(buffer, sizeof(buffer),
        "  Baked CSG plan roles: direct %ld  transformed %ld  plane %ld  nested %ld",
        planDirectPrimitiveRoles,
        planTransformedPrimitiveRoles,
        planPlaneRoles,
        planNestedRoles);
    printProgress(buffer);

    if (stats.getUsedTime() != 0.0) {
        const int hours = (int)stats.getUsedTime() / 3600;
        const int minutes = (int)(stats.getUsedTime() - hours * 3600) / 60;
        const double seconds =
            stats.getUsedTime() - (double)(hours * 3600 + minutes * 60);
        snprintf(buffer, sizeof(buffer),
            "  Time For Trace:    %2d hours %2d minutes %4.2f seconds", hours,
            minutes, seconds);
        printProgress(buffer);
    }
}

void
PovRayApplication::run(int argc, char *argv[])
{
    initializeFromCommandLine(argc, argv);
    configureOutputTarget();
    parseSceneDescription();
    prepareRendering();
    runRenderLoop();
    finalizeRun();
}

void
PovRayApplication::initializeFromCommandLine(int argc, char *argv[])
{
    if (argc == 1) {
        CommandLineOptions::usage();
    }

    initVars();
    fileLocator.clearSearchPaths();

    CommandLineOptions::loadDefaults(configuration, fileLocator, scene);
    CommandLineOptions::parseArguments(argc, argv, configuration, fileLocator, scene);

    if (configuration.getLastLine() == -1) {
        configuration.setLastLine(
            engine.getScene().getScreenHeight());
    }
}

void
PovRayApplication::configureOutputTarget()
{
    if (!configuration.hasOptionFlags(PovRayRendererConfiguration::DISK_WRITE)) {
        return;
    }

    switch (configuration.getOutputFormat()) {
    case '\0':
    case 'd':
    case 'D':
        if ((selectedImageOutput = new RawDumpFormat()) == nullptr) {
            closeAll();
            exit(1);
        }
        break;
    case 'r':
    case 'R':
        if ((selectedImageOutput = new RawFormat()) == nullptr) {
            closeAll();
            exit(1);
        }
        break;
    case 't':
    case 'T':
        if ((selectedImageOutput = new TargaFormat()) == nullptr) {
            closeAll();
            exit(1);
        }
        break;
    default:
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "Unrecognized output file format %c\n", configuration.getOutputFormat());
            Logger::reportMessage("PovRayApplication", Logger::FATAL_ERROR, "", _logMsg);
        }
    }

    selectedImageOutput->setFileLocator(&fileLocator);
    configuration.setOutputFileInputStream(
        new ImageOutputAdapter(selectedImageOutput));

    if (!configuration.hasOutputFileName()) {
        configuration.setOutputFileName(
            configuration.getOutputFileInputStream()->defaultFileName());
    }
}

void
PovRayApplication::parseSceneDescription()
{
    ParserContext ctx;
    ctx.tokenizer().setCaseSensitiveIdentifiers(configuration.getTokenizerCaseSensitiveMode());
    ctx.tokenizer().setMaxSymbols(configuration.getTokenizerMaxSymbols());
    ctx.tokenizer().setFileLocator(&fileLocator);
    ctx.tokenizer().initializeTokenizer(configuration.getInputFileName());
    printProgress("Parsing...");

    ctx.setDiagnostics(
        configuration.hasOptionFlags(PovRayRendererConfiguration::VERBOSE_FILE),
        configuration.getStatFileName());
    ctx.setCsgRoth(configuration.hasOptionFlags(PovRayRendererConfiguration::CSG_ROTH));
    ctx.setAntialiasThreshold(configuration.getAntialiasThreshold());
    ctx.setRuntimeState(&runtimeState);
    SceneParser::parse(&engine.getScene(), ctx);
    ctx.tokenizer().terminateTokenizer();
}

void
PovRayApplication::prepareRendering()
{
    if (configuration.hasOptionFlags(PovRayRendererConfiguration::DISPLAY)) {
        printProgress("Displaying...\n");
    }

    engine.getScene().finalizeCompiledTracingScene();

    if (configuration.hasOptionFlags(PovRayRendererConfiguration::DISK_WRITE)) {
        if (configuration.hasOptionFlags(PovRayRendererConfiguration::CONTINUE_TRACE)) {
            if (configuration.getOutputFileInputStream()->open(
                    configuration.getOutputFileNameBuffer(),
                    &engine.getScene().getScreenWidth(), &engine.getScene().getScreenHeight(),
                    configuration.getFileBufferSize(), RenderOutput::READ_MODE,
                    configuration.getFirstLine()) != 1) {
                Logger::reportMessage("PovRayApplication", Logger::ERROR, "", "Error opening continue trace output file");
                char buffer[512];
                snprintf(buffer, sizeof(buffer), "Opening new output file %s.", configuration.getOutputFileName());
                printProgress(buffer);
                configuration.clearOptionFlags(PovRayRendererConfiguration::CONTINUE_TRACE);

                if (configuration.getOutputFileInputStream()->open(
                        configuration.getOutputFileNameBuffer(),
                        &engine.getScene().getScreenWidth(), &engine.getScene().getScreenHeight(),
                        configuration.getFileBufferSize(), RenderOutput::WRITE_MODE,
                        configuration.getFirstLine()) != 1) {
                    Logger::reportMessage("PovRayApplication", Logger::ERROR, "", "Error opening output file\n");
                    closeAll();
                    exit(1);
                }
            }

            engine.initializeRenderer();
            if (configuration.hasOptionFlags(PovRayRendererConfiguration::CONTINUE_TRACE)) {
                engine.readRenderedPart();
            }
        } else {
            if (configuration.getOutputFileInputStream()->open(
                    configuration.getOutputFileNameBuffer(),
                    &engine.getScene().getScreenWidth(), &engine.getScene().getScreenHeight(),
                    configuration.getFileBufferSize(), RenderOutput::WRITE_MODE,
                    configuration.getFirstLine()) != 1) {
                Logger::reportMessage("PovRayApplication", Logger::ERROR, "", "Error opening output file\n");
                closeAll();
                exit(1);
            }

            engine.initializeRenderer();
        }
    } else {
        engine.initializeRenderer();
    }

    engine.getIntersectionQueuePool().init();
    textureUtils.initialize(statistics.getSolidTextureStatistics());
    textureUtils.initializeNoise(PovRayMaterial::DEFAULT_NUMBER_OF_WAVES);
}

void
PovRayApplication::runRenderLoop()
{
    if (configuration.hasOptionFlags(PovRayRendererConfiguration::VERBOSE) && (configuration.getVerboseFormat() != '1')) {
        printProgress("Rendering...");
    } else if (configuration.hasOptionFlags(PovRayRendererConfiguration::VERBOSE) && (configuration.getVerboseFormat() == '1')) {
        char buffer[512];
        snprintf(buffer, sizeof(buffer), "POV-Ray rendering %s to %s :",
            configuration.getInputFileName(), configuration.getOutputFileName());
        printProgress(buffer);
    }

    if (configuration.hasOptionFlags(PovRayRendererConfiguration::PARALLEL)) {
        engine.startTracingParallel();
    } else {
        engine.startTracing();
    }
}

void
PovRayApplication::finalizeRun()
{
    closeAll();
    printStatistics(statistics, engine.getScene(), configuration);

    // image_map/bump_map/material_map images are intentionally shared (not
    // cloned) by every PovRayMaterial generation/clone that references them
    // (see LoadedImageRegistry), so they can only be freed once rendering -
    // the last thing that could still read one via a texture lookup - has
    // fully finished. runRenderLoop() (called before finalizeRun() in run())
    // is synchronous, so this is safe here.
    LoadedImageRegistry::freeAll();

    printProgress("Done Tracing");
}

void
PovRayApplication::initVars()
{
    configuration.reset();
    runtimeState.reset();
    statistics.reset();

    engine.getScene().setScreenHeight(100);
    engine.getScene().setScreenWidth(100);
}

// Close all the stuff that has been opened
void
PovRayApplication::closeAll()
{
    if (configuration.getOutputFileInputStream()) {
        configuration.getOutputFileInputStream()->close();
        delete configuration.getOutputFileInputStream();
        configuration.setOutputFileInputStream(nullptr);
    }
    if (selectedImageOutput) {
        delete selectedImageOutput;
        selectedImageOutput = nullptr;
    }
}
