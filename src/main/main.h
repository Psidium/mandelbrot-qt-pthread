#pragma once

#include <cstdint>
#include "vendor/mandelbrot/mandelbrot.h"

struct Point {
    double x;
    double y;
};

struct Color {
    // this order is everything!
    uint8_t b,g,r,a;
};

struct ProducerItem {
    MANDELBROT *man;
    Color* colors;
};

struct PixelRect {
    int height;
    int width;
    int x_pos;
    int y_pos;
};

struct ScreenSize {
    int width;
    int height;
};

struct ProcItem {
    MANDELBROT *man;
    Color* colors;
    int color_len;
    ScreenSize size;
    ProcItem* next_item;
    int generated_index;
    int grid_pos_x;
    int grid_pos_y;
};

MANDELBROT** split_man(MANDELBROT *man, int size);
int* divide_chuncks(int size, int div);
PixelRect* divide_screen_in_px_chuncks(ScreenSize size, int div, int* length);
