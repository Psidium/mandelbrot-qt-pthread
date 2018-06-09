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
