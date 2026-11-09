#include "java/io/File.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/gui/framebuffer/FramebufferButton.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferFileDialog.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferLabel.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferList.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferTextField.h"

// Tags of the list rows: the parent folder, subfolders and files.
static constexpr long PARENT_ROW = 0;
static constexpr long FOLDER_ROW = 1;
static constexpr long FILE_ROW = 2;

FramebufferFileDialog::FramebufferFileDialog(const java::String &text, bool saving)
    : title(text), save(saving), folder("."), filterIndex(-1), acceptLabel(saving ? "Save" : "Open"),
      cancelLabel("Cancel"), folderLabel("Folder:"), fileNameLabel("File name:"), filterLabel("Filter:"),
      dialog(nullptr), owner(nullptr), folderText(nullptr), filterButton(nullptr), entries(nullptr),
      nameField(nullptr), acceptButton(nullptr) {}

FramebufferFileDialog::~FramebufferFileDialog() {
    for (long i = 0; i < filters.size(); i++) {
        delete filters[i];
    }
}

void
FramebufferFileDialog::addFilter(const DialogFileFilter &filter) {
    filters.add(new DialogFileFilter(filter));
}

void
FramebufferFileDialog::setFolder(const java::String &path) {
    folder = normalize(java::File(path).getAbsolutePath());
}

void
FramebufferFileDialog::setLabels(const java::String &accept, const java::String &cancel, const java::String &place,
                                 const java::String &fileName, const java::String &filter) {
    acceptLabel = accept;
    cancelLabel = cancel;
    folderLabel = place;
    fileNameLabel = fileName;
    filterLabel = filter;
}

java::String
FramebufferFileDialog::normalize(const java::String &path) {
    java::ArrayList<java::String> parts;
    int start = 0;
    for (int i = 0; i <= path.length(); i++) {
        if (i < path.length() && path.charAt(i) != '/') {
            continue;
        }
        java::String part = path.substring(start, i);
        start = i + 1;
        if (part.isEmpty() || part.equals(".")) {
            continue;
        }
        if (part.equals("..")) {
            if (parts.size() > 0) {
                parts.remove(parts.size() - 1);
            }
            continue;
        }
        parts.add(part);
    }
    java::String result("");
    for (long partIndex = 0; partIndex < parts.size(); partIndex++) {
        const java::String &part = parts[partIndex];
        result += "/";
        result += part;
    }
    return result.isEmpty() ? java::String("/") : result;
}

java::String
FramebufferFileDialog::filterText() const {
    if (filterIndex < 0 || filters.size() == 0) {
        return filterLabel + " *";
    }
    return filterLabel + " " + filters[filterIndex]->description();
}

java::String
FramebufferFileDialog::resolve(const java::String &name) const {
    if (name.startsWith("/")) {
        return normalize(name);
    }
    return normalize(folder + "/" + name);
}

static void
sortNames(java::ArrayList<java::String> &names) {
    for (int i = 1; i < (int)names.size(); i++) {
        java::String value = names[i];
        int k = i - 1;
        while (k >= 0 && names[k] > value) {
            names[k + 1] = names[k];
            k--;
        }
        names[k + 1] = value;
    }
}

void
FramebufferFileDialog::fill() {
    java::ArrayList<java::String> names = java::File(folder).list();
    java::ArrayList<java::String> folders;
    java::ArrayList<java::String> files;
    for (long i = 0; i < names.size(); i++) {
        const java::String &name = names.get(i);
        if (name.isEmpty() || name.charAt(0) == '.') {
            continue;
        }
        java::String path = resolve(name);
        bool directory = java::File(path).isDirectory();
        if (directory) {
            folders.add(name);
        } else if (filterIndex < 0 || filters[filterIndex]->accepts(path, false)) {
            files.add(name);
        }
    }
    sortNames(folders);
    sortNames(files);
    entries->clear();
    if (!folder.equals("/")) {
        entries->add("../", 0, PARENT_ROW);
    }
    for (long nameIndex = 0; nameIndex < folders.size(); nameIndex++) {
        const java::String &name = folders[nameIndex];
        entries->add(name + "/", 0, FOLDER_ROW);
    }
    for (long nameIndex = 0; nameIndex < files.size(); nameIndex++) {
        const java::String &name = files[nameIndex];
        entries->add(name, 0, FILE_ROW);
    }
    folderText->setText(folder);
    filterButton->setLabel(filterText());
}

void
FramebufferFileDialog::enter(const java::String &path) {
    folder = normalize(path);
    fill();
    nameField->setText("");
    dialog->redraw(folderText);
    dialog->redraw(entries);
    dialog->redraw(nameField);
}

// Returns true when the name was accepted as the chosen file.
bool
FramebufferFileDialog::enterOrAccept(const java::String &name) {
    java::String trimmed = name.trim();
    if (trimmed.isEmpty()) {
        return false;
    }
    java::String path = resolve(trimmed);
    java::File file(path);
    if (file.isDirectory()) {
        enter(path);
        return false;
    }
    if (!save && !file.exists()) {
        return false;
    }
    chosen = path;
    return true;
}

void
FramebufferFileDialog::redrawBehind() {
    if (owner) {
        owner->redrawBehind();
    }
}

bool
FramebufferFileDialog::controlResponded(FramebufferDialog &box, FramebufferControl *control,
                                        FramebufferResponse response) {
    if (control == entries) {
        int row = entries->getLead();
        if (row < 0) {
            return true;
        }
        long kind = entries->tagAt(row);
        java::String name = entries->textAt(row);
        if (response == FramebufferResponse::Changed) {
            if (kind == FILE_ROW) {
                nameField->setText(name);
                box.redraw(nameField);
            }
            return true;
        }
        if (kind == PARENT_ROW) {
            enter(folder + "/..");
        } else if (kind == FOLDER_ROW) {
            enter(folder + "/" + name.substring(0, name.length() - 1));
        } else if (enterOrAccept(name)) {
            box.close(acceptButton);
        }
        return true;
    }
    if (control == filterButton && response == FramebufferResponse::Activated) {
        // Every file, then each filter.
        filterIndex = filterIndex + 1 < (int)filters.size() ? filterIndex + 1 : -1;
        fill();
        box.redraw(filterButton);
        box.redraw(entries);
        return true;
    }
    if ((control == nameField || control == acceptButton) && response == FramebufferResponse::Activated) {
        if (enterOrAccept(nameField->getText())) {
            box.close(acceptButton);
        }
        return true;
    }
    return false;
}

bool
FramebufferFileDialog::run(Framebuffer &surface, FramebufferInput &input, const FramebufferTheme &theme,
                           FramebufferDialogListener *listener, java::String &path) {
    FramebufferDialog box(title);
    dialog = &box;
    owner = listener;
    box.setMinimumSize(56, 27);
    box.add(new FramebufferLabel(folderLabel));
    // The path alone, without its beginning when it is too long.
    folderText = static_cast<FramebufferLabel *>(box.add(new FramebufferLabel("")));
    folderText->setElideLeft(true);
    filterButton = new FramebufferButton(filterText(), "filter");
    box.add(filterButton);
    entries = static_cast<FramebufferList *>(box.add(new FramebufferList(50, 12, false)));
    box.add(new FramebufferLabel(fileNameLabel));
    nameField = static_cast<FramebufferTextField *>(box.add(new FramebufferTextField("", 50)));
    acceptButton = box.addButton(new FramebufferButton(acceptLabel, "accept"), true);
    box.addButton(new FramebufferButton(cancelLabel, "cancel"), false, true);
    fill();
    box.setFocus(save ? static_cast<FramebufferControl *>(nameField) : entries);
    chosen = "";
    FramebufferButton *answer = box.run(surface, input, theme, this);
    dialog = nullptr;
    if (answer != acceptButton || chosen.isEmpty()) {
        return false;
    }
    path = chosen;
    return true;
}
