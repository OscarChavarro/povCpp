#ifndef __UNIX_H__
#define __UNIX_H__

class UnixPlatform {
  public:
    static void initPovray();
    static void displayFinished();
    static void displayInit(int width, int height);
    static void displayClose();
    static void displayPlot(
        int x, int y, unsigned char red, unsigned char green, unsigned char blue);
};

#endif
