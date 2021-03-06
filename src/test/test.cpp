#include <main.h>

#include <gtest/gtest.h>

TEST(ChunckDivider, CanPatchSizes) {
    int* chuncks = divide_chuncks(1423, 3);
    ASSERT_EQ(chuncks[0], 474);
    ASSERT_EQ(chuncks[1], 474);
    ASSERT_EQ(chuncks[2], 475);
}

TEST(ChunckDivider, NoDivision) {
    int* chuncks = divide_chuncks(1000, 1);
    ASSERT_EQ(chuncks[0], 1000);
}

TEST(ChunckDivider, BigDivisions) {
    int* chuncks = divide_chuncks(1000, 7);
    int sum = 0;
    for (int i=0; i< 7; i++) {
        sum += chuncks[i];
    }
    ASSERT_EQ(sum, 1000);

    ASSERT_EQ(chuncks[0], 142);
    ASSERT_EQ(chuncks[1], 143);
    ASSERT_EQ(chuncks[2], 143);
    ASSERT_EQ(chuncks[3], 143);
    ASSERT_EQ(chuncks[4], 143);
    ASSERT_EQ(chuncks[5], 143);
    ASSERT_EQ(chuncks[6], 143);
}

TEST(PxChunckDivider, GetPixelRegions) {
    ScreenSize size;
    size.width = 1000;
    size.height = 1000;
    int len = 0;
    PixelRect* chuncks = divide_screen_in_px_chuncks(size, 2, &len);
    
    PixelRect first = { 500, 500, 500, 500 };
    ASSERT_TRUE(0 == std::memcmp( &(chuncks[0]), &first, sizeof(first)));
    
    PixelRect second = { 500, 500, 1000, 500 };
    ASSERT_TRUE(0 == std::memcmp( &(chuncks[1]), &second, sizeof(second)));
    
    PixelRect third = { 500, 500, 500, 1000 };
    ASSERT_TRUE(0 == std::memcmp( &(chuncks[2]), &third, sizeof(third)));
    
    PixelRect fourth = { 500, 500, 1000, 1000 };
    ASSERT_TRUE(0 == std::memcmp( &(chuncks[3]), &fourth, sizeof(fourth)));
}


TEST(MandelbrotDivider, DivideIn4) {
    MANDELBROT man = {
        500, // pw picture width
        0.0, // center_x
        0.0, // center_y
        2.0, // w
        2.0 // h
    };
    MANDELBROT** results = split_man(&man, 2);
    ASSERT_EQ(results[0]->pw, 250);
    ASSERT_EQ(results[0]->h, 1);
    ASSERT_EQ(results[0]->w, 1);
    ASSERT_EQ(results[0]->cx, -0.5);
    ASSERT_EQ(results[0]->cy, -0.5);
    
    ASSERT_EQ(results[01]->pw, 250);
    ASSERT_EQ(results[01]->h, 1);
    ASSERT_EQ(results[01]->w, 1);
    ASSERT_EQ(results[01]->cx, -0.5);
    ASSERT_EQ(results[01]->cy, -0.5);
}

