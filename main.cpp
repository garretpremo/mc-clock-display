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
  std::cout << "Ctrl + C detected, exiting...";
}

void draw(Canvas *canvas, int iteration) {

    float redPercentage = ((iteration % MAX_COLOR_VALUE) / MAX_COLOR_VALUE);
    uint8_t red = (uint8_t)ceil(redPercentage * MAX_COLOR_VALUE);

    // draw pixels on the canvas
    for (int y = 0; y < canvas->height(); y++) {
        for (int x = 0; x < canvas->width(); x++) {
            canvas->SetPixel(x, y, red, 0, 0);
        }
    }
}

int main(int argc, char* argv[]) {
    std::cout << "Beginning program...\nPress Ctrl + C to exit.\n";

    // define defaults
    RGBMatrix::Options defaults;
    defaults.hardware_mapping = "regular";
    defaults.rows = 16;
    defaults.cols = 32;
    defaults.chain_length = 1;
    defaults.parallel = 1;
    defaults.brightness = 50;

    Canvas *canvas = RGBMatrix::CreateFromFlags(&argc, &argv, &defaults);

    if (canvas == NULL) {
        return EXIT_FAILURE;
    }

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    int i = 0;

    ssize_t frame_size = canvas->width() * canvas->height() * 3;
    uint8_t buf[frame_size];

    while (1) {
        struct timespec start;
        timespec_get(&start, TIME_UTC);

        // handle interruptions
        ssize_t nread;
        ssize_t total_nread = 0;
        while ((nread = read(STDIN_FILENO, &buf[total_nread], frame_size - total_nread)) > 0) {
            if (program_interrupted) {
                return EXIT_FAILURE;
            }
            total_nread += nread;
        }
        
        if (total_nread < frame_size){
            break;
        }

        draw(canvas, i++);

        // throttle the loop so we only run 60 iterations per second
        struct timespec end;
        timespec_get(&end, TIME_UTC);
        long tudiff = (end.tv_nsec / 1000 + end.tv_sec * 1000000) - (start.tv_nsec / 1000 + start.tv_sec * 1000000);

        if (tudiff < 1000000l / FPS) {
            usleep(1000000l / FPS - tudiff);
        }
    }
    
    canvas->Clear();
    delete canvas;
    return EXIT_SUCCESS;
}