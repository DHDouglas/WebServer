#include <gtest/gtest.h>
#include <string>
#include "buffer.h"  

using namespace std;


TEST(BufferTest, appendretrieve) {
    Buffer buf;
    EXPECT_EQ(buf.readableBytes(), 0);
    EXPECT_EQ(buf.writableBytes(), Buffer::kInitialSize);
    EXPECT_EQ(buf.prependableBytes(), 8);

    const string str(200, 'x');
    buf.append(str);
    EXPECT_EQ(buf.readableBytes(), str.size());
    EXPECT_EQ(buf.writableBytes(), Buffer::kInitialSize - str.size());
    EXPECT_EQ(buf.prependableBytes(), 8);

    const string str2 = buf.retrieveToString(50);
    EXPECT_EQ(str2.size(), 50);
    EXPECT_EQ(buf.readableBytes(), str.size() - str2.size());
    EXPECT_EQ(buf.writableBytes(), Buffer::kInitialSize - str.size());
    EXPECT_EQ(buf.prependableBytes(), 8 + str2.size());
    EXPECT_EQ(str2, string(50, 'x'));

    buf.append(str);
    EXPECT_EQ(buf.readableBytes(), 2 * str.size() - str2.size());
    EXPECT_EQ(buf.writableBytes(), Buffer::kInitialSize - 2 * str.size());
    EXPECT_EQ(buf.prependableBytes(), 8 + str2.size());

    const string str3 = buf.retrieveAllToString();
    EXPECT_EQ(str3.size(), 350);
    EXPECT_EQ(buf.readableBytes(), 0);
    EXPECT_EQ(buf.writableBytes(), Buffer::kInitialSize);
    EXPECT_EQ(buf.prependableBytes(), 8);
    EXPECT_EQ(str3, string(350, 'x'));
}

TEST(BufferTest, Grow) {
    Buffer buf;
    buf.append(string(400, 'y'));
    EXPECT_EQ(buf.readableBytes(), 400);
    EXPECT_EQ(buf.writableBytes(), Buffer::kInitialSize - 400);

    buf.retrieve(50);
    EXPECT_EQ(buf.readableBytes(), 350);
    EXPECT_EQ(buf.writableBytes(), Buffer::kInitialSize - 400);
    EXPECT_EQ(buf.prependableBytes(), 8 + 50);

    buf.append(string(1000, 'z'));
    EXPECT_EQ(buf.readableBytes(), 1350);
    EXPECT_EQ(buf.writableBytes(), 0);
    EXPECT_EQ(buf.prependableBytes(), 8 + 50);

    buf.retrieveAll();
    EXPECT_EQ(buf.readableBytes(), 0);
    EXPECT_EQ(buf.writableBytes(), 1400);
    EXPECT_EQ(buf.prependableBytes(), 8);
}

TEST(BufferTest, InsideGrow) {
    Buffer buf;
    buf.append(string(800, 'y'));
    EXPECT_EQ(buf.readableBytes(), 800);
    EXPECT_EQ(buf.writableBytes(), Buffer::kInitialSize - 800);

    buf.retrieve(500);
    EXPECT_EQ(buf.readableBytes(), 300);
    EXPECT_EQ(buf.writableBytes(), Buffer::kInitialSize - 800);
    EXPECT_EQ(buf.prependableBytes(), 8 + 500);

    buf.append(string(300, 'z'));
    EXPECT_EQ(buf.readableBytes(), 600);
    EXPECT_EQ(buf.writableBytes(), Buffer::kInitialSize - 600);
    EXPECT_EQ(buf.prependableBytes(), 8);
}

// TEST(BufferTest, Shrink)
// {
//     Buffer buf;
//     buf.append(string(2000, 'y'));
//     EXPECT_EQ(buf.readableBytes(), 2000);
//     EXPECT_EQ(buf.writableBytes(), 0);
//     EXPECT_EQ(buf.prependableBytes(), 0);

//     buf.retrieve(1500);
//     EXPECT_EQ(buf.readableBytes(), 500);
//     EXPECT_EQ(buf.writableBytes(), 0);
//     EXPECT_EQ(buf.prependableBytes(), 0 + 1500);

//     buf.shrink(0);
//     EXPECT_EQ(buf.readableBytes(), 500);
//     EXPECT_EQ(buf.writableBytes(), Buffer::kInitialSize - 500);
//     EXPECT_EQ(buf.retrieveAllToStr(), string(500, 'y'));
//     EXPECT_EQ(buf.prependableBytes(), 0);
// }

// TEST(BufferTest, Prepend)
// {
//     Buffer buf;
//     buf.append(string(200, 'y'));
//     EXPECT_EQ(buf.readableBytes(), 200);
//     EXPECT_EQ(buf.writableBytes(), Buffer::kInitialSize - 200);
//     EXPECT_EQ(buf.prependableBytes(), 0);

//     int x = 0;
//     buf.prepend(&x, sizeof(x));
//     EXPECT_EQ(buf.readableBytes(), 204);
//     EXPECT_EQ(buf.writableBytes(), Buffer::kInitialSize - 200);
//     EXPECT_EQ(buf.prependableBytes(), 0 - 4);
// }


// TEST(BufferTest, ReadInt)
// {
//     Buffer buf;
//     buf.append("HTTP");

//     EXPECT_EQ(buf.readableBytes(), 4);
//     EXPECT_EQ(buf.PeekInt8(), 'H');
//     int top16 = buf.PeekInt16();
//     EXPECT_EQ(top16, 'H' * 256 + 'T');
//     EXPECT_EQ(buf.PeekInt32(), top16 * 65536 + 'T' * 256 + 'P');

//     EXPECT_EQ(buf.readInt8(), 'H');
//     EXPECT_EQ(buf.readInt16(), 'T' * 256 + 'T');
//     EXPECT_EQ(buf.readInt8(), 'P');
//     EXPECT_EQ(buf.readableBytes(), 0);
//     EXPECT_EQ(buf.writableBytes(), Buffer::kInitialSize);

//     buf.appendInt8(-1);
//     buf.appendInt16(-2);
//     buf.appendInt32(-3);
//     EXPECT_EQ(buf.readableBytes(), 7);
//     EXPECT_EQ(buf.readInt8(), -1);
//     EXPECT_EQ(buf.readInt16(), -2);
//     EXPECT_EQ(buf.readInt32(), -3);
// }


// TEST(BufferTest, FindEOL)
// {
//     Buffer buf;
//     buf.append(string(100000, 'x'));
//     const char* null = nullptr;
//     EXPECT_EQ(buf.findEOL(), null);
//     EXPECT_EQ(buf.findEOL(buf.Peek() + 90000), null);
// }

// void output(Buffer&& buf, const void* inner)
// {
//     Buffer newbuf(std::move(buf));
//     EXPECT_EQ(inner, newbuf.Peek());
// }

// NOTE: This test fails in g++ 4.4, passes in g++ 4.6.
// TEST(BufferTest, Move)
// {
//     Buffer buf;
//     buf.append("muduo", 5);
//     const void* inner = buf.Peek();
//     output(std::move(buf), inner);
// }
