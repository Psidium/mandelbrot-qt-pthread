#pragma once

#include <cstdint>

struct Point {
    double x;
    double y;
};

struct Color {
    // this order is everything!
    uint8_t b,g,r,a;
};

struct PixelRect {
    int height;
    int width;
    int x_pos;
    int y_pos;
};

struct ProcItem {
    Point begin;
    Point end;
    PixelRect screen_position;
    // colors' size is defined by screen_position.height * screen_position.width
    Color* colors;
    ProcItem* next_item;
};

struct ScreenSize {
    int width;
    int height;
};

int* divide_chuncks(int size, int div);
PixelRect* divide_screen_in_px_chuncks(ScreenSize size, int div, int* length);
