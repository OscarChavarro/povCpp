#ifndef __RENDER_FILE_HANDLE_ADAPTER_H__
#define __RENDER_FILE_HANDLE_ADAPTER_H__

class ImageFileHandle;
class RGBAColor;

class RenderFileInputStreamAdapter {
  public:
    static int readLine(
        ImageFileHandle *fileHandle, RGBAColor *lineData, int *lineNumber);
    static int open(ImageFileHandle *fileHandle, char *name, int *imageWidth,
        int *imageHeight, int bufferSize, int openMode);
    static void close(ImageFileHandle *fileHandle);
};

#endif
