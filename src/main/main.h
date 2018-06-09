#pragma once

struct Point {
    double x;
    double y;
};

struct Color {
    float r,g,b;
};

struct ProcItem {
    Point begin;
    Point end;
    int px_height;
    Color* colors;
    ProcItem* next_item;
};

struct ScreenSize {
    int width;
    int height;
};

int* divide_chuncks(int size, int div);
