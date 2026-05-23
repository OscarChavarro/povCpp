#ifndef __RENDER_FILE_HANDLE_ADAPTER_H__
#define __RENDER_FILE_HANDLE_ADAPTER_H__

class FileInputStream;
class RGBAColor;

class RenderFileInputStreamAdapter {
  public:
    static constexpr int APPEND_MODE = 2;

    static int readLine(
        FileInputStream *fileHandle, RGBAColor *lineData, int *lineNumber);
    static int open(FileInputStream *fileHandle, char *name, int *imageWidth,
        int *imageHeight, int bufferSize, int openMode);
    static void close(FileInputStream *fileHandle);
};

#endif
