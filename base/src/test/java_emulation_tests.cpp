#include "java/lang/IllegalStateException.h"
#include "java/lang/RuntimeException.h"
#include <gtest/gtest.h>

#include <cstring>

#include "java/io/BufferedReader.h"
#include "java/io/InputStream.h"
#include "java/io/OutputStream.h"
#include "java/io/PrintWriter.h"
#include "java/lang/Runnable.h"
#include "java/lang/String.h"
#include "java/lang/StringBuilder.h"
#include "java/lang/Thread.h"
#include "java/util/ArrayList.txx"
#include "java/util/concurrent/atomic/AtomicLong.h"
#include "java/util/regex/Matcher.h"
#include "java/util/regex/Pattern.h"

using java::util::regex::Matcher;
using java::util::regex::Pattern;

namespace {

/** Gives its text in chunks of a fixed size, as a socket may do. */
class ChunkedInputStream : public java::InputStream {
    java::String text;
    size_t position;
    int chunk;

public:
    ChunkedInputStream(const java::String &text, int chunk)
        : text(text), position(0), chunk(chunk) {}

    int read() override
    {
        return position < text.size() ?
            static_cast<unsigned char>(text[position++]) : -1;
    }

    int read(unsigned char *buffer, int offset, int length) override
    {
        if ( position >= text.size() ) {
            return -1;
        }
        int n = static_cast<int>(text.size() - position);
        if ( n > length ) n = length;
        if ( n > chunk ) n = chunk;
        std::memcpy(buffer + offset, text.data() + position, n);
        position += n;
        return n;
    }

    void close() override {}
};

class StringOutputStream : public java::OutputStream {
public:
    java::String text;
    bool failing = false;
    int flushes = 0;

    void write(int value) override
    {
        if ( failing ) throw java::RuntimeException("closed");
        text += static_cast<char>(value);
    }

    void write(const unsigned char *buffer, int offset, int length) override
    {
        if ( failing ) throw java::RuntimeException("closed");
        for ( int i = 0; i < length; i++ ) {
            text += static_cast<char>(buffer[offset + i]);
        }
    }

    void flush() override { flushes++; }
    void close() override {}
};

class Counter : public java::Runnable {
public:
    java::AtomicLong runs;
    void run() override { runs.incrementAndGet(); }
};

}

TEST(JavaEmulationTest, StringHelpersFollowJava)
{
    java::String text("  a\"b\\c  ");

    EXPECT_STREQ("a\"b\\c", text.trim().c_str());
    EXPECT_STREQ("  a\\\"b\\\\c  ",
        text.replace("\\", "\\\\").replace("\"", "\\\"").c_str());
    EXPECT_STREQ("PHONG", java::String("phong").toUpperCase().c_str());
    EXPECT_STREQ("cpu", java::String("CPU").toLowerCase().c_str());
    EXPECT_TRUE(java::String("GPU").equalsIgnoreCase("gpu"));
    EXPECT_EQ(3, java::String("abcabc").indexOf(java::String("abc"), 1));
    EXPECT_EQ(-1, java::String("abc").indexOf(java::String("x")));
    EXPECT_TRUE(java::String("file.json").endsWith(".json"));
    EXPECT_STREQ("aa", java::String("aa").replace("", "x").c_str());
}

TEST(JavaEmulationTest, StringBuilderAppendsValuesAsJava)
{
    java::StringBuilder sb;

    sb.append("[").append(1).append(',').append(2.0).append(',')
        .append(0.5).append(',').append(true).append(',').append(-7LL)
        .append("]");
    EXPECT_STREQ("[1,2.0,0.5,true,-7]", sb.toString().c_str());
}

TEST(JavaEmulationTest, BufferedReaderReadsLinesOverChunks)
{
    ChunkedInputStream in("first line\r\nsecond\n\nlast\rafter", 3);
    java::BufferedReader reader(&in);
    java::String line;

    ASSERT_TRUE(reader.readLine(line));
    EXPECT_STREQ("first line", line.c_str());
    ASSERT_TRUE(reader.readLine(line));
    EXPECT_STREQ("second", line.c_str());
    ASSERT_TRUE(reader.readLine(line));
    EXPECT_STREQ("", line.c_str());
    ASSERT_TRUE(reader.readLine(line));
    EXPECT_STREQ("last", line.c_str());
    ASSERT_TRUE(reader.readLine(line));
    EXPECT_STREQ("after", line.c_str());
    EXPECT_FALSE(reader.readLine(line));
}

TEST(JavaEmulationTest, PrintWriterFlushesAndReportsErrors)
{
    StringOutputStream out;
    java::PrintWriter writer(&out, true);

    writer.println("{\"id\":1}");
    EXPECT_STREQ("{\"id\":1}\n", out.text.c_str());
    EXPECT_EQ(1, out.flushes);
    EXPECT_FALSE(writer.checkError());

    out.failing = true;
    writer.println("lost");
    EXPECT_TRUE(writer.checkError());
}

TEST(JavaEmulationTest, PatternFindsSuccessiveMatchesAndGroups)
{
    Pattern pattern = Pattern::compile("\"" + Pattern::quote("x.y") +
        "\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?)");
    Matcher matcher = pattern.matcher(
        "{\"xzy\":9,\"x.y\" : -1.5,\"x.y\":2}");

    ASSERT_TRUE(matcher.find());
    EXPECT_STREQ("-1.5", matcher.group(1).c_str());
    EXPECT_EQ(1, matcher.groupCount());
    ASSERT_TRUE(matcher.find());
    EXPECT_STREQ("2", matcher.group(1).c_str());
    EXPECT_FALSE(matcher.find());
    EXPECT_THROW(matcher.group(1), java::IllegalStateException);
}

TEST(JavaEmulationTest, PatternMatchesEscapedJsonStrings)
{
    Matcher matcher = Pattern::compile(
        "\"name\"\\s*:\\s*\"((?:\\\\.|[^\"])*)\"").matcher(
        "{\"name\":\"a \\\"quoted\\\" text\"}");

    ASSERT_TRUE(matcher.find());
    EXPECT_STREQ("a \\\"quoted\\\" text", matcher.group(1).c_str());
    EXPECT_EQ(1, matcher.start());
    EXPECT_TRUE(Pattern::compile("[a-z]+").matcher("abc").matches());
    EXPECT_FALSE(Pattern::compile("[a-z]+").matcher("ab1").matches());
}

TEST(JavaEmulationTest, ThreadRunsItsTarget)
{
    Counter counter;
    java::Thread thread(&counter);

    thread.setName("counter");
    thread.start();
    thread.join();
    EXPECT_EQ(1, counter.runs.get());
    EXPECT_STREQ("counter", thread.getName().c_str());
}

TEST(JavaEmulationTest, ThreadMayDestroyItsOwnObject)
{
    struct SelfDeleting : public java::Runnable {
        java::Thread *thread = nullptr;
        java::AtomicLong *done = nullptr;
        void run() override
        {
            delete thread;
            done->set(1);
            delete this;
        }
    };
    java::AtomicLong done;
    SelfDeleting *task = new SelfDeleting();
    task->done = &done;
    task->thread = new java::Thread(task);
    java::Thread *thread = task->thread;
    thread->start();
    for ( int i = 0; i < 200 && done.get() == 0; i++ ) {
        java::Thread::sleep(5);
    }
    EXPECT_EQ(1, done.get());
}
