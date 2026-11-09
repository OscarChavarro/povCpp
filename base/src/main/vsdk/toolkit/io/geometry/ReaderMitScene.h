#ifndef __READER_MIT_SCENE__
#define __READER_MIT_SCENE__

class SimpleScene;

class ReaderMitScene {
public:
    ReaderMitScene();
    virtual ~ReaderMitScene() {}

    void importEnvironment(const char* fileName, SimpleScene* outScene);
};

#endif
