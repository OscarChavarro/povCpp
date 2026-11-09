#ifndef VITRAL_FRAMEBUFFER_FILE_DIALOG_H
#define VITRAL_FRAMEBUFFER_FILE_DIALOG_H

#include "java/util/ArrayList.h"
#include "java/lang/String.h"
#include "vsdk/toolkit/gui/dialog/components/DialogFileFilter.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferDialog.h"

class FramebufferButton;
class FramebufferLabel;
class FramebufferList;
class FramebufferTextField;

// A Motif style file selection box on a FramebufferDialog: the folder, a button that cycles through every file and
// the filters, the list of subfolders (with a trailing '/', and ".." first) and accepted files, sorted by name and
// without hidden entries, the file name field and the accept and cancel buttons. Selecting a file puts its name in the
// field; activating a folder (Enter, double click) enters it and activating a file accepts it. A name typed in the
// field is taken relative to the folder unless it starts with '/'; when it names a folder, the dialog enters it.
// Open dialogs only accept existing files; save dialogs accept new names. It reads folders with java::File, so it
// works wherever the base library does.
class FramebufferFileDialog : private FramebufferDialogListener {
public:
    FramebufferFileDialog(const java::String &title, bool save);
    ~FramebufferFileDialog();
    FramebufferFileDialog(const FramebufferFileDialog &) = delete;
    FramebufferFileDialog &operator=(const FramebufferFileDialog &) = delete;
    void addFilter(const DialogFileFilter &filter);
    void setFolder(const java::String &folder);
    // Texts of the dialog, in the language of the application.
    void setLabels(const java::String &accept, const java::String &cancel, const java::String &folder,
                   const java::String &fileName, const java::String &filter);
    // Shows the dialog. Returns true with the chosen path; false when it was canceled or the input ended. The
    // owner redraws the application when the surface is resized.
    bool run(Framebuffer &surface, FramebufferInput &input, const FramebufferTheme &theme,
             FramebufferDialogListener *owner, java::String &path);
    // The folder shown last, to start there next time.
    const java::String &getFolder() const { return folder; }

    // A path without "." and ".." components and without a trailing '/'.
    static java::String normalize(const java::String &path);

private:
    java::String title;
    bool save;
    java::String folder;
    // Owned copies of the filters.
    java::ArrayList<DialogFileFilter *> filters;
    // The filter shown, or -1 for every file (the first choice).
    int filterIndex;
    java::String acceptLabel;
    java::String cancelLabel;
    java::String folderLabel;
    java::String fileNameLabel;
    java::String filterLabel;
    FramebufferDialog *dialog;
    FramebufferDialogListener *owner;
    FramebufferLabel *folderText;
    FramebufferButton *filterButton;
    FramebufferList *entries;
    FramebufferTextField *nameField;
    FramebufferButton *acceptButton;
    java::String chosen;

    void fill();
    java::String filterText() const;
    java::String resolve(const java::String &name) const;
    bool enterOrAccept(const java::String &name);
    void enter(const java::String &path);
    void redrawBehind() override;
    bool controlResponded(FramebufferDialog &dialog, FramebufferControl *control,
                          FramebufferResponse response) override;
};

#endif
