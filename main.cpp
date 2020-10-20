#include "./rpi-rgb-led-matrix/include/led-matrix.h"
#include "./rpi-rgb-led-matrix/include/graphics.h"

#include <math.h>
#include <signal.h>
#include <iostream>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <png.h>

using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;
using rgb_matrix::Color;

#define FPS 60
#define MAX_COLOR_VALUE 255

// PNG Image wrapper for pnglib
class Image {

public:
    char *filename;
    int width;
    int height;
    png_byte colorType;
    png_byte bitDepth;
    png_bytep *rowPointers = NULL;

    Image(char* _filename) {
        filename = _filename;
        initialize();
        // printStatistics();
    }

private:
    void initialize() {
        FILE* file = fopen(filename, "rb");

        if (!file) {
            fclose(file);
            return;
        }
        
        png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png) {
            failInitialization(file);
        }

        png_infop info = png_create_info_struct(png);
        if (!info) {
            failInitialization(file);
        }

        if (setjmp(png_jmpbuf(png))) {
            failInitialization(file);
        }

        png_init_io(png, file);
        png_read_info(png, info);

        width = png_get_image_width(png, info);
        height = png_get_image_height(png, info);
        colorType = png_get_color_type(png, info);
        bitDepth = png_get_bit_depth(png, info);

        png_set_palette_to_rgb(png);

        if (png_get_valid(png, info, PNG_INFO_tRNS)) {
            png_set_tRNS_to_alpha(png);
        }

        // fill alpha channel with 0xFF
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
        png_read_update_info(png, info);

        // initialize rows
        rowPointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
        
        for(int y = 0; y < height; y++) {
            rowPointers[y] = (png_byte*)malloc(png_get_rowbytes(png, info));
        }

        png_read_image(png, rowPointers);

        fclose(file);
        png_destroy_read_struct(&png, &info, NULL);
    }

    void determineIfPngFile(FILE* file) {
        int headerSize = 8;
        unsigned char* header = (unsigned char*) malloc(sizeof(unsigned char) * headerSize);

        fread(header, 1, headerSize, file);
        bool isPng = !png_sig_cmp(header, 0, headerSize);

        if (!isPng) {
            std::cerr << "Error - Could not open file - File is not of type 'png'." << std::endl;
            fclose(file);
            free(header);
            abort();
        }

        free(header);
    }

    void failInitialization(FILE* file) {
        fclose(file);
        abort();
    }

    void printStatistics() {
        std::cout << "Width: " << width << std::endl;
        std::cout << "Height: " << height << std::endl;
        std::cout << "Color Depth: " << colorType << std::endl;
        std::cout << "Bit Depth: " << bitDepth << std::endl;
    }
};

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

void drawImage(Canvas* canvas, Image* image) {
    for (int y = 0; y < image->height && !program_interrupted; y++) {
        png_bytep row = image->rowPointers[y];
        
        for (int x = 0; x < image->width; x++) {
            png_bytep px = &(row[x * 4]);

            canvas->SetPixel(x, y, px[0], px[1], px[2]);
        }

        usleep(1 * 100000);  // wait a little to slow down things.
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
    defaults.brightness = 75;

    Canvas *canvas = RGBMatrix::CreateFromFlags(&argc, &argv, &defaults);
    Image dawn = Image("./assets/images/dawn.png");
    Image noon = Image("./assets/images/noon.png");
    Image dusk = Image("./assets/images/dusk.png");
    Image midnight = Image("./assets/images/midnight.png");

    if (canvas == NULL) {
        return EXIT_FAILURE;
    }

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    // Color background = randomColor();

    while (!program_interrupted) {
        drawImage(canvas, &dawn);
        drawImage(canvas, &noon);
        drawImage(canvas, &dusk);
        drawImage(canvas, &midnight);

        // Color foreground = randomColor();
        // draw(canvas, background, foreground);
        // background = foreground;
    }
    
    canvas->Clear();
    delete canvas;
    return EXIT_SUCCESS;
}
