#include "main.h"

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
#include <QPushButton>
#include <QtDebug>

#include <iostream>
using std::cout;
using std::endl;
#include <fstream>
#include <string>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <cmath>
#include <functional>

#define sign(x) (x > 0) - (x < 0)

#ifdef UNIT_TESTS
#define MAIN not_main
#else
#define MAIN main
#endif

typedef struct {
    ProcItem* work_item_todo; // the buffer
    size_t split_size;
    MANDELBROT *final_man; // final mandelbrot
    pthread_mutex_t mutex_producer_consumer; // needed to add/remove data from the buffer
    pthread_cond_t can_produce; // signaled when items are removed
    pthread_cond_t can_consume; // signaled when items are added
    ProcItem* work_item_done; // the result
    pthread_mutex_t mutex_assembler;
    pthread_cond_t can_assemble;
    Color* final_color;
    ScreenSize final_size;
    ImageViewer* imageViwer;
} buffer_t;

ScreenSize calculate_size(MANDELBROT *man) {
    return {man->pw, (int) ceil(man->h * man->pw / man->w)};
}

MANDELBROT** split_man(MANDELBROT *man, int size) {
    MANDELBROT** brots = (MANDELBROT**) calloc(size*size,sizeof(MANDELBROT*));
    long double pw = man->pw / size,
        width = man->w / size,
        height = man->h / size;
    long double left_x = man->cx > 0 ? man->cx - man->w / 2: man->cx + man->w / 2;
    long double down_y =  man->cy > 0 ? man->cy - man->h / 2: man->cy + man->h / 2;
    
    // O(n) deveria ser O(n^2)
    for (int i=0; i < size; i++) {
        for (int j=0; j< size; j++) {
            brots[i*size + j] = (MANDELBROT *) calloc(1, sizeof(MANDELBROT));
            *brots[i*size + j] = *man;
            brots[i*size + j]->pw = pw;
            brots[i*size + j]->w = width;
            brots[i*size + j]->h = height;
            long double x_offset = (width * (i % size));
            long double y_offset = (height * (j % size));
            brots[i*size + j]->cx = left_x > 0 ? left_x + x_offset : left_x - x_offset;
            brots[i*size + j]->cy = down_y > 0 ? down_y + y_offset : down_y - y_offset;
        }
    }
    return brots;
}

void *master_producer(void* arg) {
    buffer_t *buffer = (buffer_t*)arg;
    
    while(1) {
        pthread_mutex_lock(&buffer->mutex_producer_consumer);
        
        while(buffer->work_item_todo != NULL || buffer->final_man == NULL) { // workload linked list
            pthread_cond_wait(&buffer->can_produce, &buffer->mutex_producer_consumer);
        }
        MANDELBROT *man = buffer->final_man;
        ScreenSize size = calculate_size(man);
        buffer->final_size = size;
        buffer->final_color = (Color*) realloc(buffer->final_color, sizeof(Color) * size.width * size.height);
        int aux = size.width * size.height;
        for (int i=0; i < aux; i++) {
            buffer->final_color[i].a = 0xFF;
            buffer->final_color[i].r = 0xFF;
            buffer->final_color[i].g = 0xFF;
            buffer->final_color[i].b = 0xFF;
        }
        ProcItem** item = (ProcItem**) calloc(buffer->split_size * buffer->split_size, sizeof(ProcItem*));
        MANDELBROT** brots = split_man(man, buffer->split_size);
        ProcItem* lastItem = NULL;
        for (int i=0; i < buffer->split_size; i++) {
            for (int j=0; j< buffer->split_size; j++) {
                item[i*buffer->split_size + j] = (ProcItem*) calloc(1, sizeof(ProcItem));
                item[i*buffer->split_size + j]->man = brots[i*buffer->split_size + j];
                
                item[i*buffer->split_size + j]->grid_pos_x = i;
                item[i*buffer->split_size + j]->grid_pos_y = j;
                if (lastItem) {
                    lastItem->next_item = item[i*buffer->split_size + j];
                }
                lastItem = item[i*buffer->split_size + j];
            }
        }
        buffer->work_item_todo = item[0];
        // signal the fact that new items may be consumed
        pthread_cond_broadcast(&buffer->can_consume);
        pthread_mutex_unlock(&buffer->mutex_producer_consumer);
    }
    return NULL;

}

void rotate90(Color *src, Color *dst, const int N, const int M) {
    for(int i=0; i<N; i++) {
        for(int j=1; j<=M; j++) {
            dst[i * M + j] = src[(M-j) * N + i];
        }
    }
}

void rotate180(Color *src, Color *dst, const int N, const int M) {
    Color* aux = (Color*) calloc(N*M,sizeof(Color));
    rotate90(src, aux, N, M);
    rotate90(aux,dst,N,M);
    free(aux);
}


void *consumer(void* arg) {
    buffer_t *buffer = (buffer_t*)arg;
    while (1) {
        pthread_mutex_lock(&buffer->mutex_producer_consumer);
        while(buffer->work_item_todo == NULL) {
            pthread_cond_wait(&buffer->can_consume, &buffer->mutex_producer_consumer);
        }
        ProcItem* item = buffer->work_item_todo;
        buffer->work_item_todo = buffer->work_item_todo->next_item;
        item->next_item = NULL;
        pthread_mutex_unlock(&buffer->mutex_producer_consumer);
        
        MANDELBROT *man = item->man;
        Color* colors;
        
        auto dimentions_func = [&](int w, int h) mutable {
            item->size.height = h;
            item->size.width = w;
            colors = (Color*) calloc(item->size.width * item->size.height, sizeof(struct Color));
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
        MAND_COLOR **palette = make_palette(man->cs, man->ncs, &total, man->ec);
        mandelbrot(man, palette, total, dim_thunk, &dimentions_func, colors_thunk, &colors_func);
        assert(iterator == item->size.width * item->size.height);
        assert(item->size.width == item->size.height);
        item->colors = (Color*) calloc(item->size.width * item->size.height, sizeof(Color));
        rotate180(colors, item->colors, item->size.width, item->size.height);
        free(colors);
//        item->colors = colors;
//        item->color_len = iterator;
        std::cout << "\n color eh " << colors;
        QImage* img = new QImage( (uint8_t*) item->colors, item->size.width, item->size.height, QImage::Format_ARGB32 );
        img->save(QString("mandelbrot_") + QString::fromStdString(std::to_string(item->grid_pos_x)) + "_" + QString::fromStdString(std::to_string(item->grid_pos_y)) + QString(".png"));
        pthread_mutex_lock(&buffer->mutex_assembler);
        ProcItem* last = buffer->work_item_done;
        if (last) {
            while (last->next_item) {
                last = last->next_item;
            }
            last->next_item = item;
        } else {
            buffer->work_item_done = item;
        }
        pthread_cond_broadcast(&buffer->can_assemble);
        pthread_mutex_unlock(&buffer->mutex_assembler);
    }
    return NULL;
}

void *assembler(void* arg) {
    buffer_t *buffer = (buffer_t*)arg;
    int total_bytes = 0;
    while (1) {
        pthread_mutex_lock(&buffer->mutex_assembler);
        while(buffer->work_item_done == NULL) {
            pthread_cond_wait(&buffer->can_assemble, &buffer->mutex_assembler);
        }
        ProcItem* item = buffer->work_item_done;
        buffer->work_item_done = buffer->work_item_done->next_item;
        item->next_item = NULL;
        std::cout << "\n color deveria " << item->colors;

        Color* color_from = item->colors;
        Color* curr_final_color = buffer->final_color;
        curr_final_color += item->grid_pos_x * item->size.width;
        curr_final_color += item->grid_pos_y * item->size.height * buffer->final_size.width;
        for (int i = 0;i < item->size.height; i++) {
            memmove(curr_final_color, color_from, item->size.width * sizeof(Color));
            color_from += item->size.width;
            curr_final_color += buffer->final_size.width;
        }
        total_bytes += item->color_len;
        std::cout << "\nAssembled bytes: " << total_bytes;
        
        free(item->man);
        free(item);
        
        QImage* image = new QImage( (uint8_t*) buffer->final_color, buffer->final_size.width, buffer->final_size.height, QImage::Format_ARGB32 );
   //     image->save(QString("mandelbrot_end.png"));
        buffer->imageViwer->setImage(*image);
        pthread_mutex_lock(&buffer->mutex_producer_consumer);
        if (buffer->work_item_todo == NULL && buffer->work_item_done == NULL && buffer->final_man != NULL) {
            free(buffer->final_man);
            buffer->final_man = NULL;
        }
        pthread_mutex_unlock(&buffer->mutex_producer_consumer);
        pthread_mutex_unlock(&buffer->mutex_assembler);
    }
    return NULL;
}
void resize_threads(pthread_t** thread, int *consum_threads_len, int new_len, buffer_t* buffer) {
    if (new_len <= 0) {
        return;
    }
    pthread_t* consumers = *thread;
    int delta = new_len - *consum_threads_len;
    if (delta > 0) {
        consumers = (pthread_t *) realloc(consumers, new_len * sizeof(pthread_t));
        int ret = 0;
        for (int i=*consum_threads_len; i< new_len; i++) {
            ret = pthread_create(consumers + i,  NULL,  consumer, (void*) buffer);
            if (ret) {
                fprintf(stderr,"Error - pthread_create() return code: %d\n", ret);
                exit(EXIT_FAILURE);
            }
        }
    }
    if (delta < 0) {
        for (int i = *consum_threads_len - 1; i >= new_len; i--) {
            pthread_join(consumers[i], NULL);
        }
        consumers = (pthread_t *) realloc(consumers, new_len * sizeof(pthread_t));
    }
    *consum_threads_len = new_len;
}

int* divide_chuncks(int size, int div) {
    int* chuncks = (int*) calloc(div, sizeof(int));
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
    PixelRect* blocks = (PixelRect*) calloc(div * div,sizeof(struct PixelRect));
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

void threadsChanged(QString numb) {
    std::cout << "novas threads: " << numb.toInt();
}

int MAIN(int argc, char** argv) {
    ScreenSize size;
    size.width = 500;
    size.height = 500;

    QApplication app(argc, argv);
    
    QWidget window;
    window.setMinimumSize(200, 200);

    QVBoxLayout layout;

    QFormLayout formLayout;
    
    QLineEdit threads;
    threads.setValidator( new QIntValidator(1, 100, &formLayout) );
    formLayout.addRow(QObject::tr("&Threads:"), &threads);
    threads.setText("5");
    QLineEdit split_size;
    split_size.setValidator( new QIntValidator(0, 10000, &formLayout) );
    formLayout.addRow(QObject::tr("&Split size (n^2):"), &split_size);
    split_size.setText("5");
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
    QPushButton button;
    button.setText(QObject::tr("&Start"));
    layout.addWidget(&button);
    
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
    
    ImageViewer view;
    QImage image = QPixmap(500, 500).toImage();
    view.setImage(image);
    
    layout.addWidget(&view);
    window.setLayout(&layout);
    window.show();
    
    buffer_t buffer = {
        NULL,
        16,
        NULL,
        PTHREAD_MUTEX_INITIALIZER,
        PTHREAD_COND_INITIALIZER,
        PTHREAD_COND_INITIALIZER,
        NULL,
        PTHREAD_MUTEX_INITIALIZER,
        PTHREAD_COND_INITIALIZER,
        NULL,
        { 500, 500 },
        &view
    };
    
    QObject::connect(&split_size, &QLineEdit::textEdited, [&](QString numb) {
        buffer.split_size = numb.toInt();
    });

  
    
    
    QObject::connect(&button, &QPushButton::pressed, [&]() {
        pthread_mutex_lock(&buffer.mutex_producer_consumer);
        ProcItem* proc = buffer.work_item_todo;
        while(proc != NULL && proc->next_item != NULL) {
            ProcItem* toClear = proc;
            proc = proc->next_item;
            free(toClear);
        }
        if (proc) {
            free(proc);
            buffer.work_item_todo = NULL;
        }
        if (buffer.final_man == NULL) {
            buffer.final_man = (MANDELBROT*) calloc(1, sizeof(MANDELBROT));
        }
        *buffer.final_man = {
            view.geometry().width(), // pw picture width
            centerX.text().toDouble(), // center_x
            centerY.text().toDouble(), // center_y
            mandelW.text().toDouble(), // w,
            mandelH.text().toDouble(), // h
            maxIter.text().toInt(), // i_max_iter
            radius.text().toDouble(), // radius
            SET_MANDELBROT, // set
            MAND_FALSE, 0.0, 0.0, // j, jr, ji julia fractal
            NULL, //ec
            // 02d/f80/ff0/f00/fff
            color_scheme, // cs color scheme
            5, // ncs number of color scheme
            MAND_TRUE // nic normalized colors
        };
        pthread_cond_broadcast(&buffer.can_produce);
        pthread_mutex_unlock(&buffer.mutex_producer_consumer);
    });
    
    int curr_thread_n = threads.text().toInt();
    pthread_t* consumers = (pthread_t*) calloc(curr_thread_n, sizeof(pthread_t));
    int consum_threads_len;
    resize_threads(&consumers, &consum_threads_len, curr_thread_n, &buffer);
    
    QObject::connect(&threads, &QLineEdit::textEdited, [&](QString numb) {
        resize_threads(&consumers, &consum_threads_len, numb.toInt(), &buffer);
    });
    
    pthread_t producer;
    
    int iret1 = pthread_create(&producer, NULL, master_producer, (void*) &buffer);
    if (iret1) {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",iret1);
        exit(EXIT_FAILURE);
    }
    
    pthread_t assembler_t;
    iret1 = pthread_create(&assembler_t, NULL, assembler, (void*) &buffer);
    if (iret1) {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",iret1);
        exit(EXIT_FAILURE);
    }
    
    
    int out = app.exec();
    resize_threads(&consumers, &consum_threads_len, 0, &buffer);
    pthread_join(producer, NULL);
    pthread_join(assembler_t, NULL);
    
    return out;
}

