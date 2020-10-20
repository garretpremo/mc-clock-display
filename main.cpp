#include "./rpi-rgb-led-matrix/include/led-matrix.h"
#include "./rpi-rgb-led-matrix/include/graphics.h"

#include <math.h>
#include <signal.h>
#include <iostream>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <png.h>

using rgb_matrix::Canvas;
using rgb_matrix::Color;
using rgb_matrix::RGBMatrix;

#define FPS 60
#define MAX_COLOR_VALUE 255

volatile bool program_interrupted = false;
static void InterruptHandler(int signo) {
    program_interrupted = true;
    std::cout << std::endl
              << "Ctrl + C detected, exiting..." << std::endl;
}

class Pixel {

public:
    int r, g, b, a;

    Pixel() {
        r = 0, g = 0, b = 0, a = 0;
    }

    Pixel(int red, int green, int blue, int alpha) {
        r = normalize(red);
        g = normalize(green);
        b = normalize(blue);
        a = alpha;
    }

    static Pixel Empty() {
        return Pixel();
    }

    static Pixel White() {
        return Pixel(255, 255, 255, 255);
    }

    void print() {
        std::cout << "(" << r << ", " << g << ", " << b << ", " << a << ")" << std::endl;
    }

    bool isInvisible() {
        return a == 0;
    }

private:
    int normalize(int pixelValue) {
        if (pixelValue < 0) {
            return 0;
        } else if (pixelValue > MAX_COLOR_VALUE) {
            return MAX_COLOR_VALUE;
        }

        return pixelValue;
    }
};

class PixelMatrix {

public:
    std::vector<std::vector<Pixel>> pixelMatrix;

    void draw(Canvas* canvas, int startX, int startY) {
        for (uint y = 0; y < pixelMatrix.size(); y++) {
            if (program_interrupted) {
                return;
            }

            std::vector<Pixel> pixelRow = pixelMatrix[y];

            for (uint x = 0; x < pixelRow.size(); x++) {
                Pixel pixel = pixelRow[x];

                if (!pixel.isInvisible()) {
                    canvas->SetPixel(x + startX, y + startY, pixel.r, pixel.g, pixel.b);
                }
            }
        }
    }

    void draw(Canvas* canvas) {
        draw(canvas, 0, 0);
    }
};

// PNG Image wrapper for pnglib
class Image: public PixelMatrix {

public:
    const char *filename;
    int width;
    int height;
    png_byte colorType;
    png_byte bitDepth;
    png_bytep *rowPointers = NULL;

    Image(const char *_filename) {
        filename = _filename;
        initialize();
    }

    ~Image() {
        if (rowPointers != NULL) {
            for (int y = 0; y < height; y++) {
                png_bytep row = rowPointers[y];
                free(row);
            }

            free(rowPointers);
        }
    }

    void drawImage(Canvas* canvas) {
        PixelMatrix::draw(canvas);
        usleep(1 * 1000000);
    }

private:
    void initialize() {
        FILE *file = fopen(filename, "rb");

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
        rowPointers = (png_bytep *)malloc(sizeof(png_bytep) * height);

        for (int y = 0; y < height; y++) {
            png_byte *byte = (png_byte *)malloc(png_get_rowbytes(png, info));
            rowPointers[y] = byte;
        }

        png_read_image(png, rowPointers);

        // initialize matrix of Pixels
        for (int y = 0; y < height; y++) {
            std::vector<Pixel> pixelRow = {};
            png_bytep row = rowPointers[y];

            for (int x = 0; x < width; x++) {
                png_bytep px = &(row[x * 4]);

                Pixel pixel = Pixel(px[0], px[1], px[2], px[3]);
                pixelRow.push_back(pixel);
            }

            pixelMatrix.push_back(pixelRow);
        }

        fclose(file);
        png_destroy_read_struct(&png, &info, NULL);
    }

    void determineIfPngFile(FILE *file) {
        int headerSize = 8;
        unsigned char *header = (unsigned char *)malloc(sizeof(unsigned char) * headerSize);

        fread(header, 1, headerSize, file);
        bool isPng = !png_sig_cmp(header, 0, headerSize);

        if (!isPng)
        {
            std::cerr << "Error - Could not open file - File is not of type 'png'." << std::endl;
            fclose(file);
            free(header);
            abort();
        }

        free(header);
    }

    void failInitialization(FILE *file) {
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

class Number: public PixelMatrix {

public:
    Number(std::vector<std::vector<Pixel>> _pixelMatrix) {
        pixelMatrix = _pixelMatrix;
    }

    static Number One() {
        std::vector<std::vector<Pixel>> matrix = { 
            { Pixel::Empty(), Pixel::White(), Pixel::Empty() },  //  *
            { Pixel::White(), Pixel::White(), Pixel::Empty() },  // **
            { Pixel::Empty(), Pixel::White(), Pixel::Empty() },  //  *
            { Pixel::Empty(), Pixel::White(), Pixel::Empty() },  //  *
            { Pixel::White(), Pixel::White(), Pixel::White() }   // ***
        };
        return Number(matrix);
    }

    static Number Two() {
        std::vector<std::vector<Pixel>> matrix = { 
            { Pixel::White(), Pixel::White(), Pixel::White() },  // ***
            { Pixel::Empty(), Pixel::Empty(), Pixel::White() },  //   *
            { Pixel::White(), Pixel::White(), Pixel::White() },  // ***
            { Pixel::White(), Pixel::Empty(), Pixel::Empty() },  // *
            { Pixel::White(), Pixel::White(), Pixel::White() }   // ***
        };
        return Number(matrix);
    }
};

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
        usleep(1 * 1000); // wait a little to slow down things.
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

    srand(time(NULL));

    // define matrix defaults
    RGBMatrix::Options defaults;
    defaults.hardware_mapping = "regular";
    defaults.rows = 16;
    defaults.cols = 32;
    defaults.chain_length = 1;
    defaults.parallel = 1;
    defaults.brightness = 100;

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

        canvas->Fill(150, 60, 0);

        // Number::One().draw(canvas, 16, 0);
        // Number::Two().draw(canvas, 19, 0);
        
        dawn.draw(canvas);
        noon.draw(canvas);
        dusk.draw(canvas);
        midnight.draw(canvas);

        // Color foreground = randomColor();
        // draw(canvas, background, foreground);
        // background = foreground;
    }

    canvas->Clear();
    delete canvas;
    return EXIT_SUCCESS;
}
