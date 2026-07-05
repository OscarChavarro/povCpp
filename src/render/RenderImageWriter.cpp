#include "vsdk/toolkit/common/logging/Logger.h"
#include "render/PovRayRendererConfiguration.h"
#include "render/RenderEngine.h"
#include "render/RenderImageWriter.h"
#include "render/RenderOutput.h"

void
RenderImageWriter::readRenderedPart(ColorRgba *lineBuffer)
{
    PovRayRendererConfiguration &config = renderEngine->getMutableConfig();
    RenderOutput *output = renderEngine->getOutputFileInputStream();
    Scene &scene = renderEngine->getScene();
    int rc;
    int lineNumber;

    while ((rc = output->readLine(lineBuffer, &lineNumber)) == 1) {
    }

    config.setFirstLine(lineNumber + 1);

    if (rc == 0) {
        output->close();
        if (output->open(
                config.getOutputFileNameBuffer(),
                &scene.getScreenWidth(),
                &scene.getScreenHeight(),
                config.getFileBufferSize(), RenderOutput::APPEND_MODE,
                config.getFirstLine()) != 1) {
            Logger::reportMessage("RenderImageWriter", Logger::FATAL_ERROR, "",
                "Error opening output file\n");
        }
        return;
    }

    Logger::reportMessage("RenderImageWriter", Logger::ERROR, "",
        "Error reading aborted data file\n");
}
