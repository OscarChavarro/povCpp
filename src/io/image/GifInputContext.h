#ifndef __GIF_INPUT_CONTEXT__
#define __GIF_INPUT_CONTEXT__

class GifInputContext {
  public:
    GifInputContext(
        int (*byteReader)(void *context),
        int (*lineWriter)(void *context, const unsigned char *pixels, int linelen),
        void *context) :
        byteReader(byteReader), lineWriter(lineWriter), context(context) {}

    int (*getByteReader() const)(void *context) { return byteReader; }
    int (*getLineWriter() const)(void *context, const unsigned char *pixels, int linelen) {
        return lineWriter;
    }
    void *getContext() const { return context; }

  private:
    int (*byteReader)(void *context);
    int (*lineWriter)(void *context, const unsigned char *pixels, int linelen);
    void *context;
};

#endif
