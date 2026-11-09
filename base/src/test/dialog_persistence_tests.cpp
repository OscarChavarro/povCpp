#include "java/lang/IllegalArgumentException.h"
#include "java/lang/String.h"
#include <gtest/gtest.h>

#include "java/io/FileInputStream.h"
#include "java/lang/StringBuilder.h"
#include "java/util/ArrayList.txx"
#include "java/util/TreeMap.txx"
#include "vsdk/toolkit/gui/dialog/components/DialogFileFilter.h"
#include "vsdk/toolkit/gui/dialog/components/DialogTreeNode.h"
#include "vsdk/toolkit/gui/dialog/runtime/DialogSession.h"
#include "vsdk/toolkit/gui/widget/Widget.h"
#include "vsdk/toolkit/gui/widget/WidgetDialog.h"
#include "vsdk/toolkit/io/gui/DialogPersistence.h"

namespace {

java::String readFile(const java::String& path)
{
    java::FileInputStream input(path.c_str());
    java::StringBuilder text;
    unsigned char buffer[4096];
    int count;
    while ( (count = input.read(buffer, 0, (int)sizeof(buffer))) > 0 ) {
        text.append((const char *)buffer, count);
    }
    input.close();
    return text.toString();
}

java::String importError(const char* json)
{
    Widget widget;
    try {
        DialogPersistence::importDialogs(json, &widget);
    }
    catch ( const java::IllegalArgumentException& e ) {
        return e.getMessage().c_str();
    }
    return "";
}

}

TEST(DialogPersistenceTest, ImportsSharedDialogDefinitions)
{
    Widget widget;
    DialogPersistence::importDialogs(
        readFile(java::String(VITRAL_REPOSITORY_ROOT) +
                 "/etc/gui/dialogs.json").c_str(), &widget);

    WidgetDialog* tangible = widget.getDialogById("tangible.connect");
    ASSERT_NE(tangible, nullptr);
    EXPECT_STREQ(tangible->getTitleKey().c_str(), "IDM_TANGIBLE_INTERFACE_TITLE");
    EXPECT_TRUE(tangible->isModal());
    ASSERT_EQ(tangible->getElements().size(), 2u);
    const WidgetDialogElement& field = tangible->getElements()[0];
    EXPECT_STREQ(field.getKind().c_str(), "textInput");
    EXPECT_STREQ(field.getBinding().c_str(), "serviceUrl");
    EXPECT_STREQ(field.getLabelKey().c_str(), "IDM_TANGIBLE_INTERFACE_SERVER_PROMPT");
    const WidgetDialogElement* cancel = tangible->getElementByAction("cancel");
    ASSERT_NE(cancel, nullptr);
    EXPECT_STREQ(cancel->getId().c_str(), "cancel");
    EXPECT_EQ(tangible->getElementByAction("missing"), nullptr);

    WidgetDialog* selector = widget.getDialogById("scene.selector");
    ASSERT_NE(selector, nullptr);
    EXPECT_FALSE(selector->isModal());
    WidgetDialog* import = widget.getDialogById("scene.import");
    ASSERT_NE(import, nullptr);
    ASSERT_EQ(import->getElements()[0].getChildren().size(), 6u);
    EXPECT_EQ(widget.getDialogById("missing"), nullptr);
}

TEST(DialogPersistenceTest, RejectsInvalidDefinitions)
{
    EXPECT_EQ(importError("{\"schemaVersion\":2,\"dialogs\":[]}"),
              "dialogs.json: unsupported schemaVersion");
    EXPECT_EQ(importError("{\"schemaVersion\":1,\"dialogs\":{}}"),
              "dialogs.json: dialogs must be an array");
    EXPECT_EQ(importError("{\"schemaVersion\":1,\"dialogs\":["
                          "{\"id\":\"a\",\"titleKey\":\"T\",\"content\":[]},"
                          "{\"id\":\"a\",\"titleKey\":\"T\",\"content\":[]}]}"),
              "dialogs[1]: duplicate dialog id a");
    EXPECT_EQ(importError("{\"schemaVersion\":1,\"dialogs\":["
                          "{\"id\":\"a\",\"content\":[]}]}"),
              "dialogs[0]: missing titleKey");
    EXPECT_EQ(importError("{\"schemaVersion\":1,\"dialogs\":["
                          "{\"id\":\"a\",\"titleKey\":\"T\",\"content\":["
                          "{\"kind\":\"slider\"}]}]}"),
              "dialogs[0].content[0]: unsupported kind slider");
    EXPECT_EQ(importError("{\"schemaVersion\":1,\"dialogs\":["
                          "{\"id\":\"a\",\"titleKey\":\"T\",\"content\":["
                          "{\"kind\":\"row\",\"children\":[{\"kind\":\"button\"}]}]}]}"),
              "dialogs[0].content[0].children[0]: missing action");
    EXPECT_EQ(importError("{\"schemaVersion\":1,\"dialogs\":["
                          "{\"id\":\"a\",\"titleKey\":\"T\",\"content\":["
                          "{\"kind\":\"textInput\",\"id\":\"x\",\"binding\":\"b\"},"
                          "{\"kind\":\"button\",\"id\":\"x\",\"action\":\"c\"}]}]}"),
              "dialogs[0].content[1]: duplicate element id x");
    EXPECT_NE(importError("{\"schemaVersion\":1,"), "");
}

TEST(DialogPersistenceTest, ReadsJsonEscapes)
{
    Widget widget;
    DialogPersistence::importDialogs(
        "{\"schemaVersion\":1,\"dialogs\":[{\"id\":\"d\\u00e9\","
        "\"titleKey\":\"a\\\"b\",\"modal\":true,\"content\":[]}]}", &widget);
    WidgetDialog* dialog = widget.getDialogById("d\xc3\xa9");
    ASSERT_NE(dialog, nullptr);
    EXPECT_STREQ(dialog->getTitleKey().c_str(), "a\"b");
}

TEST(DialogSessionTest, IgnoresStaleAndClosedActions)
{
    WidgetDialog dialog;
    DialogSession session(&dialog);
    long first = session.beginAction();
    EXPECT_GT(first, 0);
    EXPECT_EQ(session.beginAction(), -1);
    EXPECT_TRUE(session.completeAction(first));
    EXPECT_FALSE(session.completeAction(first));

    long second = session.beginAction();
    session.close();
    EXPECT_FALSE(session.completeAction(second));
    EXPECT_EQ(session.beginAction(), -1);

    session.setDraft("serviceUrl", "ws://host");
    EXPECT_STREQ(session.getDraft("serviceUrl").c_str(), "ws://host");
    EXPECT_STREQ(session.getDraft("missing").c_str(), "");
    EXPECT_THROW(DialogSession(nullptr), java::IllegalArgumentException);
}

TEST(DialogFileFilterTest, AcceptsSuffixIgnoringCase)
{
    DialogFileFilter filter("OBJ", "obj Alias/Wavefront text mesh");
    EXPECT_TRUE(filter.accepts("/a/b/model.Obj", false));
    EXPECT_TRUE(filter.accepts("/a/b/folder", true));
    EXPECT_FALSE(filter.accepts("/a/b/model.ply", false));
    EXPECT_FALSE(filter.accepts("/a/b/obj", false));
    EXPECT_STREQ(filter.description().c_str(),
                 "obj Alias/Wavefront text mesh (*.obj)");
}

TEST(DialogTreeNodeTest, KeepsLabelsIdentitiesAndChildren)
{
    int element = 7;
    DialogTreeNode root("Scene", nullptr);
    DialogTreeNode& group = root.add(DialogTreeNode("Lights", nullptr));
    group.add(DialogTreeNode("Light 1", &element));
    ASSERT_EQ(root.getChildren().size(), 1u);
    const DialogTreeNode& light = root.getChildren()[0].getChildren()[0];
    EXPECT_STREQ(light.getLabel().c_str(), "Light 1");
    EXPECT_EQ(light.getIdentity(), &element);
    EXPECT_EQ(root.getIdentity(), nullptr);
    EXPECT_STREQ(group.toString().c_str(), "Lights");
}
