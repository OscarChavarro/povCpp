#include "common/ParseReportingConfiguration.h"

#include "environment/material/RendererConfiguration.h"

bool
ParseReportingConfiguration::writesVerboseErrors(const RenderingConfiguration *config)
{
    const RenderingConfiguration &cfg = config ? *config : RenderingConfiguration::global();
    return cfg.hasOptionFlags(RenderingConfiguration::VERBOSE_FILE);
}

const char *
ParseReportingConfiguration::statFileName(const RenderingConfiguration *config)
{
    const RenderingConfiguration &cfg = config ? *config : RenderingConfiguration::global();
    return cfg.getStatFileName();
}
