#include "main.h"

#include <QApplication>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>

#include <iostream>
using std::cout;
using std::endl;
#include <fstream>
#include <string>
#include <unistd.h>
#include <pthread.h>
#include <math.h>


#ifdef UNIT_TESTS
#define MAIN not_main
#else
#define MAIN main
#endif


void *master_producer(void* arg) {
    Color* colors = (Color*) arg;
    printf("GOT INSIDE THEM THREAD");
    while(1) {
        sleep(1);
        for (int i=0; i< 1000 * 1000; i++) {
            uint8_t aux = colors[i].b;
            colors[i].b = colors[i].r;
            colors[i].r = colors[i].g;
            colors[i].g = aux;
        }
        printf("CHANGED COLORS!");
    }
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


    QSlider* slider = new QSlider();
    slider->setOrientation(Qt::Horizontal);
    slider->setRange(0, 1000);
    slider->setValue(0);
    //slider->setGeometry(10, 10, 10, 300);
    layout.addWidget(slider);
    Color* colors = (Color*) malloc(sizeof(struct Color) * size.width * size.height);
    for (int i=0; i< size.width * size.height; i++) {
        colors[i].a = 255;
        colors[i].r = 255;
    }
    QImage* image = new QImage( (uint8_t*) colors, size.width, size.height, QImage::Format_ARGB32 );
    QLabel* label = new QLabel();
    label->setPixmap(QPixmap::fromImage(*image));
    layout.addWidget(label);
    window.setLayout(&layout);
    window.show();
    
    pthread_t thread;
    
    int iret1 = pthread_create(&thread, NULL, master_producer, (void*) colors);
    if (iret1) {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",iret1);
        exit(EXIT_FAILURE);
    }

//   / QObject::connect(slider, SIGNAL (valueChanged(int)), NULL, SLOT (setValue(int)));
    
    return app.exec();
    return EXIT_SUCCESS;
}

