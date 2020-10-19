#include "./rpi-rgb-led-matrix/include/led-matrix.h"

#include <math.h>
#include <signal.h>
#include <iostream>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;

#define FPS 60
#define MAX_COLOR_VALUE 255

volatile bool program_interrupted = false;
static void InterruptHandler(int signo) {
  program_interrupted = true;
  std::cout << "Ctrl + C detected, exiting..." << std::endl;
}

void draw(Canvas *canvas) {

    canvas->Fill(255, 0, 0);

    int center_x = canvas->width() / 2;
    int center_y = canvas->height() / 2;
    float radius_max = canvas->width() / 2;
    float angle_step = 1.0 / 360;
    
    for (float a = 0, r = 0; r < radius_max; a += angle_step, r += angle_step) {
        if (interrupt_received)
        return;
        float dot_x = cos(a * 2 * M_PI) * r;
        float dot_y = sin(a * 2 * M_PI) * r;
        canvas->SetPixel(center_x + dot_x, center_y + dot_y, 255, 0, 0);
        usleep(1 * 1000);  // wait a little to slow down things.
    }
}

int main(int argc, char* argv[]) {
    std::cout << "Beginning program...\nPress Ctrl + C to exit." << std::endl;

    // define defaults
    RGBMatrix::Options defaults;
    defaults.hardware_mapping = "regular";
    defaults.rows = 16;
    defaults.cols = 32;
    defaults.chain_length = 1;
    defaults.parallel = 1;
    // defaults.brightness = 50;

    Canvas *canvas = RGBMatrix::CreateFromFlags(&argc, &argv, &defaults);

    if (canvas == NULL) {
        return EXIT_FAILURE;
    }

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    draw(canvas);
    // int i = 0;

    // ssize_t frame_size = canvas->width() * canvas->height() * 3;
    // uint8_t buf[frame_size];

    // while (1) {
    //     struct timespec start;
    //     timespec_get(&start, TIME_UTC);

    //     // handle interruptions
    //     ssize_t nread;
    //     ssize_t total_nread = 0;
    //     while ((nread = read(STDIN_FILENO, &buf[total_nread], frame_size - total_nread)) > 0) {
    //         if (program_interrupted) {
    //             return EXIT_FAILURE;
    //         }
    //         total_nread += nread;
    //     }
        
    //     if (total_nread < frame_size){
    //         break;
    //     }

    //     draw(canvas, i++);

    //     // throttle the loop so we only run 60 iterations per second
    //     struct timespec end;
    //     timespec_get(&end, TIME_UTC);
    //     long tudiff = (end.tv_nsec / 1000 + end.tv_sec * 1000000) - (start.tv_nsec / 1000 + start.tv_sec * 1000000);

    //     if (tudiff < 1000000l / FPS) {
    //         usleep(1000000l / FPS - tudiff);
    //     }
    // }
    
    canvas->Clear();
    delete canvas;
    return EXIT_SUCCESS;
}