#include "main.h"
#include "vendor/mandelbrot/mandelbrot.h"
#include "QtObjects.h"

#include <QObject>
#include <QApplication>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QPainter>
#include <QtDebug>

#include <iostream>
using std::cout;
using std::endl;
#include <fstream>
#include <string>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <functional>


#ifdef UNIT_TESTS
#define MAIN not_main
#else
#define MAIN main
#endif



void *master_producer(void* arg) {
//    Color* colors = (Color*) arg;
//    printf("GOT INSIDE THEM THREAD");
//    while(1) {
//        sleep(1);
//        for (int i=0; i< 1000 * 1000; i++) {
//            uint8_t aux = colors[i].b;
//            colors[i].b = colors[i].r;
//            colors[i].r = colors[i].g;
//            colors[i].g = aux;
//        }
//        printf("CHANGED COLORS!");
//    }
}


void calculate_mandelbrot(Point begin, Point end, int px_height, ProcItem* parent_item) {
    ProcItem* currentItem = new ProcItem();
    currentItem->begin = begin;
    currentItem->end = end;
    currentItem->screen_position.height = px_height;
    // allocate the colors
    Color* colors = (Color*) malloc(sizeof(struct Color) * px_height * px_height);

    // calculate colors
    currentItem->colors = colors;
    // lock
    ProcItem* previous_item = parent_item;
    while (previous_item->next_item) previous_item = previous_item->next_item;
    previous_item->next_item = currentItem;
    // unlock
}


int* divide_chuncks(int size, int div) {
    int* chuncks = (int*) malloc(div * sizeof(int));
    float rem = (float) size/ (float) div;
    if (size % div != 0) {
        float remainder = 0;
        for (int i = 0; i< div; i++) {
            chuncks[i] = (int) floor(rem);
            remainder += rem - chuncks[i];
            if (remainder >= 1) {
                chuncks[i]++;
                remainder--;
            }
        }
    } else {
        for (int i = 0; i< div; i++) {
            chuncks[i] = (int) rem;
        }
    }
    return chuncks;
}

PixelRect* divide_screen_in_px_chuncks(ScreenSize size, int div, int* length) {
    int* height_chunck = divide_chuncks(size.height, div);
    int* width_chunck = divide_chuncks(size.width, div);
    *length = div * div;
    PixelRect* blocks = (PixelRect*) malloc(sizeof(struct PixelRect) * div * div);
    int blocks_curr = 0;
    int curr_px_h_pos = 0;
    for (int i=0; i< div; i++) {
        curr_px_h_pos += height_chunck[i];
        int curr_px_w_pos = 0;
        for (int j=0; j<div; j++) {
            curr_px_w_pos += width_chunck[j];
            PixelRect rect;
            rect.height = height_chunck[i];
            rect.width = width_chunck[j];
            rect.x_pos = curr_px_w_pos;
            rect.y_pos = curr_px_h_pos;
            blocks[blocks_curr++] = rect;
        }
    }
    
    free(height_chunck);
    free(width_chunck);
    
    return blocks;
}


int MAIN(int argc, char** argv) {
    ScreenSize size;
    size.width = 1000;
    size.height = 1000;
    int div = 2;
    int squares = 0;

    QApplication app(argc, argv);
    
    QWidget window;
    window.setMinimumSize(200, 200);

    QVBoxLayout layout;

    QFormLayout formLayout;
    
    QLineEdit threads;
    threads.setValidator( new QIntValidator(0, 32, &formLayout) );
    formLayout.addRow(QObject::tr("&Threads:"), &threads);
    threads.setText("4");
    QLineEdit centerX;
    centerX.setValidator( new QDoubleValidator(0, 100, 10, &formLayout) );
    formLayout.addRow(QObject::tr("&Center X:"), &centerX);
    centerX.setText("-0.743643135");
    QLineEdit centerY;
    centerY.setValidator( new QDoubleValidator(0, 100, 10, &formLayout) );
    formLayout.addRow(QObject::tr("&Center Y:"), &centerY);
    centerY.setText("0.131825963");
    QLineEdit mandelW;
    mandelW.setValidator( new QDoubleValidator(0, 100, 10, &formLayout) );
    formLayout.addRow(QObject::tr("&Mandelbrot Width:"), &mandelW);
    mandelW.setText("0.000014628");
    QLineEdit mandelH;
    mandelH.setValidator( new QDoubleValidator(0, 100, 10, &formLayout) );
    formLayout.addRow(QObject::tr("&Mandelbrot Height:"), &mandelH);
    mandelH.setText("0.000014628");
    
    QLineEdit maxIter;
    maxIter.setValidator( new QIntValidator(0, 5000, &formLayout) );
    formLayout.addRow(QObject::tr("&Maximum Iterations:"), &maxIter);
    maxIter.setText("2048");
    
    QLineEdit radius;
    radius.setValidator( new QDoubleValidator(0, 100, 2, &formLayout) );
    formLayout.addRow(QObject::tr("&Mandelbrot Height:"), &radius);
    radius.setText("2.0");
    
    layout.addLayout(&formLayout);
    
    
    
    MAND_COLOR ** color_scheme = (MAND_COLOR**) calloc(5, sizeof(MAND_COLOR*));
    MAND_COLOR color1 = { 0x00, 0x22, 0xdd };
    color_scheme[0] = &color1;
    MAND_COLOR color2 = { 0xff, 0x88, 0x00 };
    color_scheme[1] = &color2;
    MAND_COLOR color3 = { 0xff, 0xff, 0x00 };
    color_scheme[2] = &color3;
    MAND_COLOR color4 = { 0xff, 0x00, 0x00 };
    color_scheme[3] = &color4;
    MAND_COLOR color5 = { 0xff, 0xff, 0xff };
    color_scheme[4] = &color5;
    // create mandelbrott
    MANDELBROT man = {
        size.width, // pw picture width
        -0.743643135, 0.131825963, // center_x, center_y
        0.000014628, 0.000014628, // w, h
        2048, // i_max_iter
        2.0, // radius
        SET_MANDELBROT, // set
        MAND_FALSE, 0.0, 0.0, // j, jr, ji julia fractal
        NULL, //ec
        // 02d/f80/ff0/f00/fff
        color_scheme, // cs color scheme
        5, // ncs number of color scheme
        MAND_TRUE // nic normalized colors
    };

    Color* colors;
    
    auto dimentions_func = [&](int w, int h) mutable {
        size.height = h;
        size.width = w;
        colors = (Color*) malloc(sizeof(struct Color) * size.width * size.height);
    };
    
    auto dim_thunk = [](int w, int h, void* arg) {
        (*static_cast<decltype(dimentions_func)*>(arg))(w, h);
    };
    
    long iterator = 0;
    auto colors_func = [&](int r, int g, int b) {
        colors[iterator].r = r;
        colors[iterator].g = g;
        colors[iterator].b = b;
        colors[iterator].a = 0xFF;
        iterator++;
    };
    
    auto colors_thunk = [](int r, int g, int b, void* arg) {
        (*static_cast<decltype(colors_func)*>(arg))(r, g, b);
    };
    
    int total;
    MAND_COLOR **palette = make_palette(man.cs, man.ncs, &total, man.ec);
    mandelbrot(&man, palette, total, dim_thunk, &dimentions_func, colors_thunk, &colors_func);
    
    QImage* image = new QImage( (uint8_t*) colors, size.width, size.height, QImage::Format_ARGB32 );
    ImageViewer view;
    view.setImage(*image);
    
    layout.addWidget(&view);
    window.setLayout(&layout);
    window.show();
    
    pthread_t thread;
    
    int iret1 = pthread_create(&thread, NULL, master_producer, (void*) colors);
    if (iret1) {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",iret1);
        exit(EXIT_FAILURE);
    }

//  QObject::connect(slider, SIGNAL (valueChanged(int)), NULL, SLOT (setValue(int)));
    
    return app.exec();
    return EXIT_SUCCESS;
}

