#include "app/ImageOutputAdapter.h"

ImageOutputAdapter::ImageOutputAdapter(ImageOutput *delegate) : delegate(delegate)
{
}

const char *
ImageOutputAdapter::defaultFileName()
{
    return delegate->defaultFileName();
}

int
ImageOutputAdapter::open(char *name, int *width, int *height, int bufferSize, int mode,
    int firstLine)
{
    return delegate->open(name, width, height, bufferSize, mode, firstLine);
}

void
ImageOutputAdapter::writeLine(ColorRgba *lineData, int lineNumber)
{
    delegate->writeLine(lineData, lineNumber);
}

int
ImageOutputAdapter::readLine(ColorRgba *lineData, int *lineNumber)
{
    return delegate->readLine(lineData, lineNumber);
}

void
ImageOutputAdapter::close()
{
    delegate->close();
}
