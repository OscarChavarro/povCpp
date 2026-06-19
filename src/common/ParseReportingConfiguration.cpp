#include "common/ParseReportingConfiguration.h"

#include "environment/material/RendererConfiguration.h"

bool
ParseReportingConfiguration::writesVerboseErrors()
{
    return RenderingConfiguration::global().hasOptionFlags(
        RenderingConfiguration::VERBOSE_FILE);
}

const char *
ParseReportingConfiguration::statFileName()
{
    return RenderingConfiguration::global().getStatFileName();
}
