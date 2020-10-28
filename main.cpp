#include "./rpi-rgb-led-matrix/include/led-matrix.h"
#include "./rpi-rgb-led-matrix/include/graphics.h"

#include <signal.h>
#include <iostream>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <png.h>
#include <cmath>
#include <chrono>
#include <ctime>
#include <cstring>
#include <string>

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

class MColor {
public:
    int r, g, b, a;

    MColor(int red, int green, int blue, int alpha) {
        r = red, g = green, b = blue, a = alpha;
    }

    static MColor White() {
        return MColor(255, 255, 255, 255);
    }

    static MColor Red() {
        return MColor(255, 0, 0, 255);
    }

    static MColor Green() {
        return MColor(0, 255, 0, 255);
    }

    static MColor Blue() {
        return MColor(0, 0, 255, 255);
    }
};

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
        a = normalize(alpha);
    }

    static Pixel Empty() {
        return Pixel();
    }

    static Pixel From(MColor color) {
        return Pixel(color.r, color.g, color.b, color.a);
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
        
        unsigned int y, x;

        for (y = 0; y < pixelMatrix.size(); y++) {
            if (program_interrupted) {
                return;
            }

            std::vector<Pixel> pixelRow = pixelMatrix[y];

            for (x = 0; x < pixelRow.size(); x++) {
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
    std::string filename;
    int width;
    int height;
    png_byte colorType;
    png_byte bitDepth;
    png_bytep *rowPointers = NULL;

    Image() {
        std::string emptyString ("");
        filename = emptyString;
    }

    Image(std::string _filename) {
        filename = _filename;
        initialize();
    }

    ~Image() {
        if (rowPointers != NULL) {
            int y;
            for (y = 0; y < height; y++) {
                png_bytep row = rowPointers[y];
                free(row);
            }

            free(rowPointers);
        }
    }

    void drawAndWait(Canvas* canvas) {
        PixelMatrix::draw(canvas);

        if (!program_interrupted) {
            usleep(1 * 40000);
        }
    }

    void drawAndWait(Canvas* canvas, int time) {
        PixelMatrix::draw(canvas);

        if (!program_interrupted) {
            usleep(time);
        }
    }

private:
    void initialize() {

        char* f_cstr = new char[filename.length()+1];
        std::strcpy(f_cstr, filename.c_str());

        FILE* file = fopen(f_cstr, "rb");

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

        int y;
        for (y = 0; y < height; y++) {
            png_byte *byte = (png_byte *)malloc(png_get_rowbytes(png, info));
            rowPointers[y] = byte;
        }

        png_read_image(png, rowPointers);

        pixelMatrix = std::vector<std::vector<Pixel>>(height);

        // initialize matrix of Pixels
        for (y = 0; y < height; y++) {
            std::vector<Pixel> pixelRow(width);
            png_bytep row = rowPointers[y];

            for (int x = 0; x < width; x++) {
                png_bytep px = &(row[x * 4]);

                Pixel pixel = Pixel(px[0], px[1], px[2], px[3]);
                pixelRow.at(x) = pixel;
            }

            pixelMatrix.at(y) = pixelRow;
        }

        fclose(file);
        delete f_cstr;
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

private:

    Number(std::vector<std::vector<Pixel>> _pixelMatrix) {
        pixelMatrix = _pixelMatrix;
    }

public:

    void draw(Canvas* canvas, int startX, int startY) {
        PixelMatrix::draw(canvas, startX, startY);
    }

    static Number From(unsigned int number, MColor color) {
        // class designed for only single-digit numbers
        unsigned int numberNormalized = number % 10;
        switch (numberNormalized) {
        case 1:
            return Number::One(color);
        case 2:
            return Number::Two(color);
        case 3:
            return Number::Three(color);
        case 4:
            return Number::Four(color);
        case 5:
            return Number::Five(color);
        case 6:
            return Number::Six(color);
        case 7:
            return Number::Seven(color);
        case 8:
            return Number::Eight(color);
        case 9:
            return Number::Nine(color);
        default:
            return Number::Zero(color);
        }
    }

    static Number One(MColor c) {
        std::vector<std::vector<Pixel>> matrix = { 
            { Pixel::Empty(), Pixel::From(c), Pixel::Empty() },  //  *
            { Pixel::From(c), Pixel::From(c), Pixel::Empty() },  // **
            { Pixel::Empty(), Pixel::From(c), Pixel::Empty() },  //  *
            { Pixel::Empty(), Pixel::From(c), Pixel::Empty() },  //  *
            { Pixel::From(c), Pixel::From(c), Pixel::From(c) }   // ***
        };
        return Number(matrix);
    }

    static Number Two(MColor c) {
        std::vector<std::vector<Pixel>> matrix = { 
            { Pixel::From(c), Pixel::From(c), Pixel::From(c) },  // ***
            { Pixel::Empty(), Pixel::Empty(), Pixel::From(c) },  //   *
            { Pixel::From(c), Pixel::From(c), Pixel::From(c) },  // ***
            { Pixel::From(c), Pixel::Empty(), Pixel::Empty() },  // *
            { Pixel::From(c), Pixel::From(c), Pixel::From(c) }   // ***
        };
        return Number(matrix);
    }

    static Number Three(MColor c) {
        std::vector<std::vector<Pixel>> matrix = { 
            { Pixel::From(c), Pixel::From(c), Pixel::From(c) },  // ***
            { Pixel::Empty(), Pixel::Empty(), Pixel::From(c) },  //   *
            { Pixel::From(c), Pixel::From(c), Pixel::From(c) },  // ***
            { Pixel::Empty(), Pixel::Empty(), Pixel::From(c) },  //   *
            { Pixel::From(c), Pixel::From(c), Pixel::From(c) }   // ***
        };
        return Number(matrix);
    }

    static Number Four(MColor c) {
        std::vector<std::vector<Pixel>> matrix = { 
            { Pixel::From(c), Pixel::Empty(), Pixel::From(c) },  // * *
            { Pixel::From(c), Pixel::Empty(), Pixel::From(c) },  // * *
            { Pixel::From(c), Pixel::From(c), Pixel::From(c) },  // ***
            { Pixel::Empty(), Pixel::Empty(), Pixel::From(c) },  //   *
            { Pixel::Empty(), Pixel::Empty(), Pixel::From(c) }   //   *
        };
        return Number(matrix);
    }

    static Number Five(MColor c) {
        std::vector<std::vector<Pixel>> matrix = { 
            { Pixel::From(c), Pixel::From(c), Pixel::From(c) },  // ***
            { Pixel::From(c), Pixel::Empty(), Pixel::Empty() },  // * 
            { Pixel::From(c), Pixel::From(c), Pixel::From(c) },  // ***
            { Pixel::Empty(), Pixel::Empty(), Pixel::From(c) },  //   *
            { Pixel::From(c), Pixel::From(c), Pixel::From(c) }   // ***
        };
        return Number(matrix);
    }

    static Number Six(MColor c) {
        std::vector<std::vector<Pixel>> matrix = { 
            { Pixel::From(c), Pixel::From(c), Pixel::From(c) },  // ***
            { Pixel::From(c), Pixel::Empty(), Pixel::Empty() },  // * 
            { Pixel::From(c), Pixel::From(c), Pixel::From(c) },  // ***
            { Pixel::From(c), Pixel::Empty(), Pixel::From(c) },  // * *
            { Pixel::From(c), Pixel::From(c), Pixel::From(c) }   // ***
        };
        return Number(matrix);
    }

    static Number Seven(MColor c) {
        std::vector<std::vector<Pixel>> matrix = { 
            { Pixel::From(c), Pixel::From(c), Pixel::From(c) },  // ***
            { Pixel::Empty(), Pixel::Empty(), Pixel::From(c) },  //   *
            { Pixel::Empty(), Pixel::Empty(), Pixel::From(c) },  //   *
            { Pixel::Empty(), Pixel::Empty(), Pixel::From(c) },  //   *
            { Pixel::Empty(), Pixel::Empty(), Pixel::From(c) }   //   *
        };
        return Number(matrix);
    }

    static Number Eight(MColor c) {
        std::vector<std::vector<Pixel>> matrix = { 
            { Pixel::From(c), Pixel::From(c), Pixel::From(c) },  // ***
            { Pixel::From(c), Pixel::Empty(), Pixel::From(c) },  // * *
            { Pixel::From(c), Pixel::From(c), Pixel::From(c) },  // ***
            { Pixel::From(c), Pixel::Empty(), Pixel::From(c) },  // * *
            { Pixel::From(c), Pixel::From(c), Pixel::From(c) }   // ***
        };
        return Number(matrix);
    }

    static Number Nine(MColor c) {
        std::vector<std::vector<Pixel>> matrix = { 
            { Pixel::From(c), Pixel::From(c), Pixel::From(c) },  // ***
            { Pixel::From(c), Pixel::Empty(), Pixel::From(c) },  // * *
            { Pixel::From(c), Pixel::From(c), Pixel::From(c) },  // ***
            { Pixel::Empty(), Pixel::Empty(), Pixel::From(c) },  //   *
            { Pixel::From(c), Pixel::From(c), Pixel::From(c) }   // ***
        };
        return Number(matrix);
    }

    static Number Zero(MColor c) {
        std::vector<std::vector<Pixel>> matrix = { 
            { Pixel::From(c), Pixel::From(c), Pixel::From(c) },  // ***
            { Pixel::From(c), Pixel::Empty(), Pixel::From(c) },  // * *
            { Pixel::From(c), Pixel::Empty(), Pixel::From(c) },  // * *
            { Pixel::From(c), Pixel::Empty(), Pixel::From(c) },  // * *
            { Pixel::From(c), Pixel::From(c), Pixel::From(c) }   // ***
        };
        return Number(matrix);
    }

    static Number Empty() {
        std::vector<std::vector<Pixel>> matrix = { 
            { Pixel::Empty(), Pixel::Empty(), Pixel::Empty() },
            { Pixel::Empty(), Pixel::Empty(), Pixel::Empty() },
            { Pixel::Empty(), Pixel::Empty(), Pixel::Empty() },
            { Pixel::Empty(), Pixel::Empty(), Pixel::Empty() },
            { Pixel::Empty(), Pixel::Empty(), Pixel::Empty() }
        };
        return Number(matrix);
    }
};

class Colon: public PixelMatrix {

private:
    Colon(std::vector<std::vector<Pixel>> _pixelMatrix) {
        pixelMatrix = _pixelMatrix;
    }

public:

    static Colon New(MColor c) {
        std::vector<std::vector<Pixel>> matrix = { 
            { Pixel::Empty() },
            { Pixel::From(c) },
            { Pixel::Empty() },
            { Pixel::From(c) },
            { Pixel::Empty() }
        };
        return Colon(matrix);
    }
};

class ClockFace {

public:
    int hour;
    int min;
    float timeWindow;
    Image* image;

    ClockFace(std::string _filename, int _hour, int _min, float _timeWindow) {
        image = new Image(_filename);
        hour = _hour;
        min = _min;
        timeWindow = _timeWindow;
    }

    ~ClockFace() {
        delete image;
    }

    void draw(Canvas* canvas) {
        image->draw(canvas);
    }

    void drawAndWait(Canvas* canvas) {
        image->drawAndWait(canvas);
    }

    bool withinCurrentTime(time_t &now) {
        tm adjustedTime = *localtime(&now);

        adjustedTime.tm_hour = hour;
        adjustedTime.tm_min = min;

        double seconds = difftime(now, mktime(&adjustedTime));
        double minutes = std::abs(seconds / 60);

        return minutes < timeWindow;
    }
};

class MinecraftClock {

private:
    Canvas* canvas;
    std::vector<ClockFace*> clockFaces = {};
    
    int currentIndex = -1;
    time_t lastChecked;
    int clockUpdateInterval = 60; // seconds

public:

    MinecraftClock(Canvas* _canvas) {
        canvas = _canvas;
        initializeClockFaces();
    }

    ~MinecraftClock() {
        unsigned int i;
        for (i = 0; i < clockFaces.size(); i++) {
            delete clockFaces[i]; 
        }
    }

    void draw() {
        if (shouldCheckForClockFaceUpdate()) {
            determineCurrentClockFaceIndex();
        }

        clockFaces[currentIndex]->draw(canvas);
    }

    void spin() {
        unsigned int i;
        for (i = 0; i < clockFaces.size() + 1; i++) {
            int indexToDraw = (currentIndex + i) % clockFaces.size();
            clockFaces[indexToDraw]->drawAndWait(canvas);
        }
    }

private:

    void initializeClockFaces() {
        int images = 64;
        float currentMinutes = 0;
        float minutesBetweenFaces = (24.0 / images) * 60;
        float timeWindow = minutesBetweenFaces / 2;

        std::cout << getcwd() << std::endl;
        std::string filePrefix("./assets/all_images_numbered/");
        std::string fileExtension(".png");

        int i;
        for (i = 0; i < images; i++) {
            std::string filename = filePrefix + std::to_string(i) + fileExtension;

            int currentHour = std::floor(currentMinutes / 60);
            int currentMinute = ((int)std::floor(currentMinutes)) % 60;

            ClockFace* clockFace = new ClockFace(filename, currentHour, currentMinute, timeWindow);
            clockFaces.push_back(clockFace);

            currentMinutes += minutesBetweenFaces;
        }
    }

    void determineCurrentClockFaceIndex() {
        time_t now = time(0);

        unsigned int i;
        for (i = 0; i < clockFaces.size(); i++) {
            ClockFace* clockFace = clockFaces[i];

            if (clockFace->withinCurrentTime(now)) {
                currentIndex = i;
                break;
            }
        }

        lastChecked = now;
    }

    bool shouldCheckForClockFaceUpdate() {
        if (currentIndex == -1) {
            return true;
        }

        time_t now = time(0);

        double seconds = difftime(now, lastChecked);

        return seconds >= clockUpdateInterval;
    }
};

void drawCurrentTime(Canvas* canvas, MColor color) {
    time_t now = time(0);

    tm* localTime = localtime(&now);

    int hour = localTime->tm_hour;
    int min = localTime->tm_min;

    int hourTensDigit = (int)std::floor(hour / 10.0);
    int minTensDigit = (int)std::floor(min / 10.0);

    Number::From(hourTensDigit, color).draw(canvas, 14, 1);
    Number::From(hour % 10, color).draw(canvas, 18, 1);
    Colon::New(color).draw(canvas, 22, 1);
    Number::From(minTensDigit, color).draw(canvas, 24, 1);
    Number::From(min % 10, color).draw(canvas, 28, 1);
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

    // MColor bg = MColor(100, 40, 0, 255);
    MColor defaultTextColor = MColor(100, 100, 100, 255);

    // define matrix defaults
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

    int iteration = 0;

    MinecraftClock* minecraftClock = new MinecraftClock(canvas);

    while (!program_interrupted) {
        canvas->Clear();

        drawCurrentTime(canvas, defaultTextColor);
        minecraftClock->draw();

        if (!program_interrupted) {
            usleep(1 * 100000);
        }

        if (iteration++ % 140 == 139) {
            minecraftClock->spin();
            iteration = 0;
        }
    }

    canvas->Clear();
    delete canvas;
    delete minecraftClock;
    return EXIT_SUCCESS;
}
