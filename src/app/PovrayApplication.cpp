#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "java/util/PriorityQueue.txx"
#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "common/RenderRuntimeState.h"
#include "common/statistics/Statistics.h"
#include "environment/material/PovrayMaterialUtils.h"
#include "environment/material/RendererConfiguration.h"
#include "environment/material/RenderOutput.h"
#include "environment/geometry/element/Intersection.h"
#include "environment/scene/Scene.h"
#include "environment/light/Light.h"
#include "environment/camera/Camera.h"
#include "io/image/ImageOutput.h"
#include "io/image/RawDumpFormat.h"
#include "io/image/RawFormat.h"
#include "io/image/TargaFormat.h"
#include "io/binaryIo/FileLocator.h"
#include "io/pov/lexer/Tokenizer.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/scene/SceneParser.h"
#include "render/RenderEngine.h"
#include "app/ImageOutputAdapter.h"
#include "app/PovrayApplication.h"
#include "app/options/CommandLineOptions.h"

static constexpr const char *COMPILER_VER = ".u";

void
PovrayApplication::printStatistics(
    const Statistics &stats,
    const Scene &frame,
    const RenderingConfiguration &configuration)
{
    FILE *statOut = stdout;
    if (configuration.getOptions() & RenderingConfiguration::VERBOSE_FILE) {
        statOut = fopen(configuration.getStatFileName(), "w+t");
    }

    const long pixelsInImage =
        (long)frame.getScreenWidth() * (long)frame.getScreenHeight();

    fprintf(statOut, "\n%s statistics\n", configuration.getInputFileName());
    if (pixelsInImage > stats.getNumberOfPixels()) {
        fprintf(statOut, "  Partial Image Rendered");
    }

    fprintf(statOut, "--------------------------------------\n");
    fprintf(statOut, "Resolution %d x %d\n", frame.getScreenWidth(),
        frame.getScreenHeight());
    fprintf(statOut,
        "# Rays:  %10ld     # Pixels:  %10ld  # Pixels supersampled: %10ld\n",
        stats.getNumberOfRays(), stats.getNumberOfPixels(),
        stats.getNumberOfPixelsSupersampled());

    fprintf(statOut, "  Ray->Shape Intersection Tests:\n");
    fprintf(statOut,
        "    type                 Tests     Succeeded    Percentage\n");
    fprintf(statOut,
        "  -----------------------------------------------------------\n");
#define PRINT_INTERSECTION_ROW(label, tests, succeeded)                         \
    if (tests) {                                                                 \
        fprintf(statOut, label " %10ld  %10ld  %10.2f\n", tests, succeeded,    \
            (((double)succeeded / (double)tests) * 100.0));                     \
    }
    PRINT_INTERSECTION_ROW("  Sphere        ", stats.getRaySphereTests(), stats.getRaySphereTestsSucceeded());
    PRINT_INTERSECTION_ROW("  Plane         ", stats.getRayPlaneTests(), stats.getRayPlaneTestsSucceeded());
    PRINT_INTERSECTION_ROW(
        "  Triangle     ", stats.getRayTriangleTests(), stats.getRayTriangleTestsSucceeded());
    PRINT_INTERSECTION_ROW("  Quadric       ", stats.getRayQuadricTests(), stats.getRayQuadricTestsSucceeded());
    PRINT_INTERSECTION_ROW("  Blob          ", stats.getRayBlobTests(), stats.getRayBlobTestsSucceeded());
    PRINT_INTERSECTION_ROW("  Box           ", stats.getRayBoxTests(), stats.getRayBoxTestsSucceeded());
    PRINT_INTERSECTION_ROW("  Quartic\\Poly ", stats.getRayPolyTests(), stats.getRayPolyTestsSucceeded());
    PRINT_INTERSECTION_ROW(
        "  Bezier Patch ", stats.getRayBicubicTests(), stats.getRayBicubicTestsSucceeded());
    PRINT_INTERSECTION_ROW(
        "  Height Fld   ", stats.getRayHtFieldTests(), stats.getRayHtFieldTestsSucceeded());
    PRINT_INTERSECTION_ROW(
        "  Hght Fld Box ", stats.getRayHtFieldBoxTests(), stats.getRayHFieldBoxTestsSucceeded());
    PRINT_INTERSECTION_ROW(
        "  Bounds        ", stats.getBoundingRegionTests(), stats.getBoundingRegionTestsSucceeded());
    PRINT_INTERSECTION_ROW(
        "  Clips         ", stats.getClippingRegionTests(), stats.getClippingRegionTestsSucceeded());
#undef PRINT_INTERSECTION_ROW

    if (stats.getSolidTextureStatistics()->callsToNoise) {
        fprintf(statOut, "  Calls to Noise:    %10ld\n", stats.getSolidTextureStatistics()->callsToNoise);
    }
    if (stats.getSolidTextureStatistics()->callsToDNoise) {
        fprintf(statOut, "  Calls to DNoise:  %10ld\n", stats.getSolidTextureStatistics()->callsToDNoise);
    }
    if (stats.getShadowRayTests()) {
        fprintf(statOut,
            "  Shadow Ray Tests: %10ld      Blocking Objects Found:  %10ld\n",
            stats.getShadowRayTests(), stats.getShadowRaysSucceeded());
    }
    if (stats.getReflectedRaysTraced()) {
        fprintf(statOut, "  Reflected Rays:    %10ld\n", stats.getReflectedRaysTraced());
    }
    if (stats.getRefractedRaysTraced()) {
        fprintf(statOut, "  Refracted Rays:    %10ld\n", stats.getRefractedRaysTraced());
    }
    if (stats.getTransmittedRaysTraced()) {
        fprintf(statOut, "  Transmitted Rays: %10ld\n", stats.getTransmittedRaysTraced());
    }

    if (stats.getUsedTime() != 0.0) {
        const int hours = (int)stats.getUsedTime() / 3600;
        const int minutes = (int)(stats.getUsedTime() - hours * 3600) / 60;
        const double seconds =
            stats.getUsedTime() - (double)(hours * 3600 + minutes * 60);
        fprintf(statOut,
            "  Time For Trace:    %2d hours %2d minutes %4.2f seconds\n", hours,
            minutes, seconds);
    }
    if (configuration.getOptions() & RenderingConfiguration::VERBOSE_FILE) {
        fclose(statOut);
    }
}

void
PovrayApplication::run(int argc, char *argv[])
{
    initializeFromCommandLine(argc, argv);
    configureOutputTarget();
    parseSceneDescription();
    const char *parseOnly = std::getenv("POVCPP_PARSE_ONLY");
    if (parseOnly != nullptr && parseOnly[0] == '1') {
        return;
    }
    prepareRendering();
    runRenderLoop();
    finalizeRun();
}

void
PovrayApplication::initializeFromCommandLine(int argc, char *argv[])
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
PovrayApplication::configureOutputTarget()
{
    if (!configuration.hasOptionFlags(RenderingConfiguration::DISKWRITE)) {
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
            Logger::reportMessage("PovrayApplication", Logger::FATAL_ERROR, "", _logMsg);
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
PovrayApplication::parseSceneDescription()
{
    FILE *statFile;

    ParserContext ctx;
    ctx.tokenizer().setCaseSensitiveIdentifiers(configuration.getTokenizerCaseSensitiveMode());
    ctx.tokenizer().setMaxSymbols(configuration.getTokenizerMaxSymbols());
    ctx.tokenizer().setFileLocator(&fileLocator);
    ctx.tokenizer().initializeTokenizer(configuration.getInputFileName());
    fprintf(stderr, "Parsing...");
    if (configuration.hasOptionFlags(RenderingConfiguration::VERBOSE_FILE)) {
        statFile = fopen(configuration.getStatFileName(), "w+t");
        fprintf(statFile, "Parsing...\n");
        fclose(statFile);
    }

    ctx.setReportingConfig(&configuration);
    ctx.setRuntimeState(&runtimeState);
    SceneParser::parse(&engine.getScene(), ctx);
    ctx.tokenizer().terminateTokenizer();
}

void
PovrayApplication::prepareRendering()
{
    if (configuration.hasOptionFlags(RenderingConfiguration::DISPLAY)) {
        Logger::reportMessage("PovrayApplication", Logger::WARNING, "", "Displaying...\n");
    }

    if (configuration.hasOptionFlags(RenderingConfiguration::DISKWRITE)) {
        if (configuration.hasOptionFlags(RenderingConfiguration::CONTINUE_TRACE)) {
            if (configuration.getOutputFileInputStream()->open(
                    configuration.getOutputFileNameBuffer(),
                    &engine.getScene().getScreenWidth(), &engine.getScene().getScreenHeight(),
                    configuration.getFileBufferSize(), RenderOutput::READ_MODE,
                    configuration.getFirstLine()) != 1) {
                Logger::reportMessage("PovrayApplication", Logger::ERROR, "", "Error opening continue trace output file\n");
                fprintf(
                    stderr, "Opening new output file %s.\n", configuration.getOutputFileName());
                configuration.clearOptionFlags(RenderingConfiguration::CONTINUE_TRACE);

                if (configuration.getOutputFileInputStream()->open(
                        configuration.getOutputFileNameBuffer(),
                        &engine.getScene().getScreenWidth(), &engine.getScene().getScreenHeight(),
                        configuration.getFileBufferSize(), RenderOutput::WRITE_MODE,
                        configuration.getFirstLine()) != 1) {
                    Logger::reportMessage("PovrayApplication", Logger::ERROR, "", "Error opening output file\n");
                    closeAll();
                    exit(1);
                }
            }

            engine.initializeRenderer();
            if (configuration.hasOptionFlags(RenderingConfiguration::CONTINUE_TRACE)) {
                engine.readRenderedPart();
            }
        } else {
            if (configuration.getOutputFileInputStream()->open(
                    configuration.getOutputFileNameBuffer(),
                    &engine.getScene().getScreenWidth(), &engine.getScene().getScreenHeight(),
                    configuration.getFileBufferSize(), RenderOutput::WRITE_MODE,
                    configuration.getFirstLine()) != 1) {
                Logger::reportMessage("PovrayApplication", Logger::ERROR, "", "Error opening output file\n");
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
    textureUtils.initializeNoise(PovrayMaterial::DEFAULT_NUMBER_OF_WAVES);
}

void
PovrayApplication::runRenderLoop()
{
    FILE *statFile;

    statistics.startTimer();

    if (configuration.hasOptionFlags(RenderingConfiguration::VERBOSE) && (configuration.getVerboseFormat() != '1')) {
        Logger::reportMessage("PovrayApplication", Logger::WARNING, "", "Rendering...\n");
    } else if (configuration.hasOptionFlags(RenderingConfiguration::VERBOSE) && (configuration.getVerboseFormat() == '1')) {
        fprintf(stderr, "POV-Ray rendering %s to %s :\n", configuration.getInputFileName(),
            configuration.getOutputFileName());
    }
    if (configuration.hasOptionFlags(RenderingConfiguration::VERBOSE_FILE)) {
        statFile = fopen(configuration.getStatFileName(), "w+t");
        fprintf(statFile, "Parsed ok. Now rendering %s to %s :\n",
            configuration.getInputFileName(), configuration.getOutputFileName());
        fclose(statFile);
    }

    engine.startTracing();

    if (configuration.hasOptionFlags(RenderingConfiguration::VERBOSE) && configuration.getVerboseFormat() == '1') {
        fprintf(stderr, "\n");
    }
}

void
PovrayApplication::finalizeRun()
{
    FILE *statFile;

    statistics.stopTimer();

    closeAll();
    printStatistics(statistics, engine.getScene(), configuration);

    if (configuration.hasOptionFlags(RenderingConfiguration::VERBOSE_FILE)) {
        statFile = fopen(configuration.getStatFileName(), "a+t");
        fprintf(statFile, "Done Tracing\n");
        fclose(statFile);
    }
}

void
PovrayApplication::initVars()
{
    configuration.reset();
    runtimeState.reset();
    statistics.reset();

    engine.getScene().setScreenHeight(100);
    engine.getScene().setScreenWidth(100);
}

// Close all the stuff that has been opened
void
PovrayApplication::closeAll()
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

#include "java/util/PriorityQueue.txx"
