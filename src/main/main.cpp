#include "main.h"

#include <math.h>
#include <iostream>
using std::cout;
using std::endl;
#include <fstream>
#include <string>
#include <pthread.h>

#ifdef UNIT_TESTS
#define MAIN not_main
#else
#define MAIN main
#endif


void *master_producer(void* arg) {
}


void calculate_mandelbrot(Point begin, Point end, int px_height, ProcItem* parent_item) {
    ProcItem* currentItem = new ProcItem();
    currentItem->begin = begin;
    currentItem->end = end;
    currentItem->px_height = px_height;
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

int MAIN(int argc, char** argv) {
    ScreenSize size;
    size.width = 1000;
    size.height = 1000;
    int div = 2;

    float aux;


    pthread_t thread;

    int iret1 = pthread_create(&thread, NULL, master_producer, (void*) &size);
    if (iret1) {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",iret1);
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}

