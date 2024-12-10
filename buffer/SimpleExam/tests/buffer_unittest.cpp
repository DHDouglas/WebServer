#include <gtest/gtest.h>
#include <string>
#include "../buffer.h"  

using namespace std;


TEST(BufferTest, AppendRetrieve)
{
    Buffer buf;
    EXPECT_EQ(buf.ReadableBytes(), 0);
    EXPECT_EQ(buf.WritableBytes(), Buffer::kInitialSize);
    EXPECT_EQ(buf.PrependableBytes(), 0);

    const string str(200, 'x');
    buf.Append(str);
    EXPECT_EQ(buf.ReadableBytes(), str.size());
    EXPECT_EQ(buf.WritableBytes(), Buffer::kInitialSize - str.size());
    EXPECT_EQ(buf.PrependableBytes(), 0);

    const string str2 = buf.RetrieveToStr(50);
    EXPECT_EQ(str2.size(), 50);
    EXPECT_EQ(buf.ReadableBytes(), str.size() - str2.size());
    EXPECT_EQ(buf.WritableBytes(), Buffer::kInitialSize - str.size());
    EXPECT_EQ(buf.PrependableBytes(), 0 + str2.size());
    EXPECT_EQ(str2, string(50, 'x'));

    buf.Append(str);
    EXPECT_EQ(buf.ReadableBytes(), 2 * str.size() - str2.size());
    EXPECT_EQ(buf.WritableBytes(), Buffer::kInitialSize - 2 * str.size());
    EXPECT_EQ(buf.PrependableBytes(), 0 + str2.size());

    const string str3 = buf.RetrieveAllToStr();
    EXPECT_EQ(str3.size(), 350);
    EXPECT_EQ(buf.ReadableBytes(), 0);
    EXPECT_EQ(buf.WritableBytes(), Buffer::kInitialSize);
    EXPECT_EQ(buf.PrependableBytes(), 0);
    EXPECT_EQ(str3, string(350, 'x'));
}

TEST(BufferTest, Grow)
{
    Buffer buf;
    buf.Append(string(400, 'y'));
    EXPECT_EQ(buf.ReadableBytes(), 400);
    EXPECT_EQ(buf.WritableBytes(), Buffer::kInitialSize - 400);

    buf.Retrieve(50);
    EXPECT_EQ(buf.ReadableBytes(), 350);
    EXPECT_EQ(buf.WritableBytes(), Buffer::kInitialSize - 400);
    EXPECT_EQ(buf.PrependableBytes(), 0 + 50);

    buf.Append(string(1000, 'z'));
    EXPECT_EQ(buf.ReadableBytes(), 1350);
    EXPECT_EQ(buf.WritableBytes(), 0);
    EXPECT_EQ(buf.PrependableBytes(), 0 + 50);

    buf.RetrieveAll();
    EXPECT_EQ(buf.ReadableBytes(), 0);
    EXPECT_EQ(buf.WritableBytes(), 1400);
    EXPECT_EQ(buf.PrependableBytes(), 0);
}

TEST(BufferTest, InsideGrow)
{
    Buffer buf;
    buf.Append(string(800, 'y'));
    EXPECT_EQ(buf.ReadableBytes(), 800);
    EXPECT_EQ(buf.WritableBytes(), Buffer::kInitialSize - 800);

    buf.Retrieve(500);
    EXPECT_EQ(buf.ReadableBytes(), 300);
    EXPECT_EQ(buf.WritableBytes(), Buffer::kInitialSize - 800);
    EXPECT_EQ(buf.PrependableBytes(), 0 + 500);

    buf.Append(string(300, 'z'));
    EXPECT_EQ(buf.ReadableBytes(), 600);
    EXPECT_EQ(buf.WritableBytes(), Buffer::kInitialSize - 600);
    EXPECT_EQ(buf.PrependableBytes(), 0);
}

// TEST(BufferTest, Shrink)
// {
//     Buffer buf;
//     buf.Append(string(2000, 'y'));
//     EXPECT_EQ(buf.ReadableBytes(), 2000);
//     EXPECT_EQ(buf.WritableBytes(), 0);
//     EXPECT_EQ(buf.PrependableBytes(), 0);

//     buf.Retrieve(1500);
//     EXPECT_EQ(buf.ReadableBytes(), 500);
//     EXPECT_EQ(buf.WritableBytes(), 0);
//     EXPECT_EQ(buf.PrependableBytes(), 0 + 1500);

//     buf.shrink(0);
//     EXPECT_EQ(buf.ReadableBytes(), 500);
//     EXPECT_EQ(buf.WritableBytes(), Buffer::kInitialSize - 500);
//     EXPECT_EQ(buf.RetrieveAllToStr(), string(500, 'y'));
//     EXPECT_EQ(buf.PrependableBytes(), 0);
// }

// TEST(BufferTest, Prepend)
// {
//     Buffer buf;
//     buf.Append(string(200, 'y'));
//     EXPECT_EQ(buf.ReadableBytes(), 200);
//     EXPECT_EQ(buf.WritableBytes(), Buffer::kInitialSize - 200);
//     EXPECT_EQ(buf.PrependableBytes(), 0);

//     int x = 0;
//     buf.prepend(&x, sizeof(x));
//     EXPECT_EQ(buf.ReadableBytes(), 204);
//     EXPECT_EQ(buf.WritableBytes(), Buffer::kInitialSize - 200);
//     EXPECT_EQ(buf.PrependableBytes(), 0 - 4);
// }


// TEST(BufferTest, ReadInt)
// {
//     Buffer buf;
//     buf.Append("HTTP");

//     EXPECT_EQ(buf.ReadableBytes(), 4);
//     EXPECT_EQ(buf.PeekInt8(), 'H');
//     int top16 = buf.PeekInt16();
//     EXPECT_EQ(top16, 'H' * 256 + 'T');
//     EXPECT_EQ(buf.PeekInt32(), top16 * 65536 + 'T' * 256 + 'P');

//     EXPECT_EQ(buf.readInt8(), 'H');
//     EXPECT_EQ(buf.readInt16(), 'T' * 256 + 'T');
//     EXPECT_EQ(buf.readInt8(), 'P');
//     EXPECT_EQ(buf.ReadableBytes(), 0);
//     EXPECT_EQ(buf.WritableBytes(), Buffer::kInitialSize);

//     buf.AppendInt8(-1);
//     buf.AppendInt16(-2);
//     buf.AppendInt32(-3);
//     EXPECT_EQ(buf.ReadableBytes(), 7);
//     EXPECT_EQ(buf.readInt8(), -1);
//     EXPECT_EQ(buf.readInt16(), -2);
//     EXPECT_EQ(buf.readInt32(), -3);
// }


// TEST(BufferTest, FindEOL)
// {
//     Buffer buf;
//     buf.Append(string(100000, 'x'));
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
//     buf.Append("muduo", 5);
//     const void* inner = buf.Peek();
//     output(std::move(buf), inner);
// }
