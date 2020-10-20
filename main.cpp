#include "./rpi-rgb-led-matrix/include/led-matrix.h"
#include "./rpi-rgb-led-matrix/include/graphics.h"

#include <math.h>
#include <signal.h>
#include <iostream>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;
using rgb_matrix::Color;

#define FPS 60
#define MAX_COLOR_VALUE 255

volatile bool program_interrupted = false;
static void InterruptHandler(int signo) {
  program_interrupted = true;
  std::cout << std::endl << "Ctrl + C detected, exiting..." << std::endl;
}

void draw(Canvas *canvas, Color background, Color foreground) {

    canvas->Fill(background.r, background.g, background.b);

    int center_x = canvas->width() / 2;
    int center_y = canvas->height() / 2;
    float radius_max = canvas->width() / 2;
    float angle_step = 1.0 / 360;
    
    for (float a = 0, r = 0; r < radius_max; a += angle_step, r += angle_step) {
        if (program_interrupted) {
            return;
        }
        
        float dot_x = cos(a * 2 * M_PI) * r;
        float dot_y = sin(a * 2 * M_PI) * r;
        canvas->SetPixel(center_x + dot_x, center_y + dot_y, foreground.r, foreground.g, foreground.b);
        usleep(1 * 1000);  // wait a little to slow down things.
    }
}

// get a random number between 0 and 255
Color randomColor() {
    int r = rand() % (MAX_COLOR_VALUE + 1);
    int g = rand() % (MAX_COLOR_VALUE + 1);
    int b = rand() % (MAX_COLOR_VALUE + 1);

    return Color(r, g, b);
}

int main(int argc, char* argv[]) {
    std::cout << "Beginning program...\nPress Ctrl + C to exit." << std::endl;

    srand (time(NULL));

    // define matrix defaults
    RGBMatrix::Options defaults;
    defaults.hardware_mapping = "regular";
    defaults.rows = 16;
    defaults.cols = 32;
    defaults.chain_length = 1;
    defaults.parallel = 1;
    defaults.brightness = 40;

    Canvas *canvas = RGBMatrix::CreateFromFlags(&argc, &argv, &defaults);

    if (canvas == NULL) {
        return EXIT_FAILURE;
    }

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    Color background = randomColor();

    while(true) {
        if (program_interrupted) {
            break;
        }

        Color foreground = randomColor();

        draw(canvas, background, foreground);

        background = foreground;
    }
    
    canvas->Clear();
    delete canvas;
    return EXIT_SUCCESS;
}