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

std::string MIDNIGHT_MINUS_THREE_QUARTERS ("./assets/images/midnight-minus-three-quarters.png");
std::string MIDNIGHT_MINUS_HALF ("./assets/images/midnight-minus-half.png");
std::string MIDNIGHT_MINUS_ONE_QUARTER ("./assets/images/midnight-minus-one-quarter.png");
std::string MIDNIGHT_FILENAME ("./assets/images/midnight.png");
std::string MIDNIGHT_PLUS_ONE_QUARTER ("./assets/images/midnight-plus-one-quarter.png");
std::string MIDNIGHT_PLUS_HALF ("./assets/images/midnight-plus-half.png");
std::string MIDNIGHT_PLUS_THREE_QUARTERS ("./assets/images/midnight-plus-three-quarters.png");
std::string DAWN_FILENAME ("./assets/images/dawn.png");
std::string NOON_MINUS_THREE_QUARTERS ("./assets/images/noon-minus-three-quarters.png");
std::string NOON_MINUS_HALF ("./assets/images/noon-minus-half.png");
std::string NOON_MINUS_ONE_QUARTER ("./assets/images/noon-minus-one-quarter.png");
std::string NOON_FILENAME ("./assets/images/noon.png");
std::string NOON_PLUS_ONE_QUARTER ("./assets/images/noon-plus-one-quarter.png");
std::string NOON_PLUS_HALF ("./assets/images/noon-plus-half.png");
std::string NOON_PLUS_THREE_QUARTERS ("./assets/images/noon-plus-three-quarters.png");
std::string DUSK_FILENAME ("./assets/images/dusk.png");
// char* DAWN_FILENAME = (char*)"./assets/images/dawn.png";
// char* NOON_FILENAME = (char*)"./assets/images/noon.png";
// char* DUSK_FILENAME = (char*)"./assets/images/dusk.png";
// char* MIDNIGHT_FILENAME = (char*)"./assets/images/midnight.png";

// Image dawn = Image("./assets/images/dawn.png");
// Image noon = Image("./assets/images/noon.png");
// Image dusk = Image("./assets/images/dusk.png");
// Image midnight = Image("./assets/images/midnight.png");

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
            for (int y = 0; y < height; y++) {
                png_bytep row = rowPointers[y];
                free(row);
            }

            free(rowPointers);
        }
    }

    void drawAndWait(Canvas* canvas) {
        PixelMatrix::draw(canvas);

        if (!program_interrupted) {
            usleep(1 * 100000);
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

    static Number From(uint number, MColor color) {
        // class designed for only single-digit numbers
        uint numberNormalized = number % 10;
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
    float window;
    Image* image;

    ClockFace(std::string _filename, int _hour, int _min, float _window) {
        image = new Image(_filename);
        hour = _hour;
        min = _min;
        window = _window;
    }

    ~ClockFace() {
        delete image;
    }
};

class MinecraftClock {

private:
    Image* currentImage = NULL;
    Canvas* canvas;

    std::vector<ClockFace*> clockFaces = {};

public:

    MinecraftClock(Canvas* canvas) {
        canvas = canvas;
        initializeClockFaces();
    }

    ~MinecraftClock() {
        for (uint i = 0; i < clockFaces.size(); i++) {
            delete clockFaces[i]; 
        }
    }

private:

    void initializeClockFaces() {
        std::cout << "initializing clock faces..." << std::endl;
        int images = 16;
        float currentMinutes = 0;

        float minutesBetweenFaces = (24 / 16) * 60;
        std::cout << "minutes between faces" << minutesBetweenFaces << std::endl;
        
        float timeWindow = minutesBetweenFaces / 2;
        std::cout << "time window" << timeWindow << std::endl;

        for (int i = 0; i < images; i++) {
            std::string filePrefix("./assets/images_numbered/");
            std::string fileSuffix(".png");
            std::string filename = filePrefix + std::to_string(i) + fileSuffix;

            int currentHour = std::floor(currentMinutes / 60);
            int currentMinute = ((int)std::floor(currentMinutes)) % 60;

            ClockFace* clockFace = new ClockFace(filename, currentHour, currentMinute, timeWindow);
            clockFaces.push_back(clockFace);

            std::cout << "filename: " << filename << ", start hour: " << currentHour << ", start minute: " << currentMinute << ", time window: " << timeWindow << std::endl;
            currentMinutes += minutesBetweenFaces;
        }
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

bool withinTime(time_t &now, int withinMinutes, int ofHours, int ofMinutes) {
    tm adjustedTime = *localtime(&now);

    adjustedTime.tm_hour = ofHours;
    adjustedTime.tm_min = ofMinutes;

    double seconds = difftime(now, mktime(&adjustedTime));
    double minutes = std::abs(seconds / 60);

    return minutes < withinMinutes;
}

void drawCurrentClockFace(Canvas* canvas, Image &image) {
    time_t now = time(0);

    if (withinTime(now, 45, 6, 0)) { // dawn
        if (image.filename.compare(DAWN_FILENAME) != 0) {
            image = Image(DAWN_FILENAME);
        }
    } else if (withinTime(now, 45, 7, 30)) {
        if (image.filename.compare(NOON_MINUS_THREE_QUARTERS) != 0) {
            image = Image(NOON_MINUS_THREE_QUARTERS);
        }
    } else if (withinTime(now, 45, 9, 0)) {
        if (image.filename.compare(NOON_MINUS_HALF) != 0) {
            image = Image(NOON_MINUS_HALF);
        }
    } else if (withinTime(now, 45, 10, 30)) {
        if (image.filename.compare(NOON_MINUS_ONE_QUARTER) != 0) {
            image = Image(NOON_MINUS_ONE_QUARTER);
        }
    } else if (withinTime(now, 45, 12, 0)) { // noon
        if (image.filename.compare(NOON_FILENAME) != 0) {
            image = Image(NOON_FILENAME);
        }
    } else if (withinTime(now, 45, 13, 30)) {
        if (image.filename.compare(NOON_PLUS_ONE_QUARTER) != 0) {
            image = Image(NOON_PLUS_ONE_QUARTER);
        }
    } else if (withinTime(now, 45, 15, 0)) {
        if (image.filename.compare(NOON_PLUS_HALF) != 0) {
            image = Image(NOON_PLUS_HALF);
        }
    } else if (withinTime(now, 45, 16, 30)) {
        if (image.filename.compare(NOON_PLUS_THREE_QUARTERS) != 0) {
            image = Image(NOON_PLUS_THREE_QUARTERS);
        }
    } else if (withinTime(now, 45, 18, 0)) { // dusk
        if (image.filename.compare(DUSK_FILENAME) != 0) {
            image = Image(DUSK_FILENAME);
        }
    } else if (withinTime(now, 45, 19, 30)) {
        if (image.filename.compare(MIDNIGHT_MINUS_THREE_QUARTERS) != 0) {
            image = Image(MIDNIGHT_MINUS_THREE_QUARTERS);
        }
    } else if (withinTime(now, 45, 21, 0)) {
        if (image.filename.compare(MIDNIGHT_MINUS_HALF) != 0) {
            image = Image(MIDNIGHT_MINUS_HALF);
        }
    } else if (withinTime(now, 45, 22, 30)) {
        if (image.filename.compare(MIDNIGHT_MINUS_ONE_QUARTER) != 0) {
            image = Image(MIDNIGHT_MINUS_ONE_QUARTER);
        }
    } else if (withinTime(now, 45, 0, 0)) { // midnight
        if (image.filename.compare(MIDNIGHT_FILENAME) != 0) {
            image = Image(MIDNIGHT_FILENAME);
        }
    } else if (withinTime(now, 45, 1, 30)) {
        if (image.filename.compare(MIDNIGHT_PLUS_ONE_QUARTER) != 0) {
            image = Image(MIDNIGHT_PLUS_ONE_QUARTER);
        }
    } else if (withinTime(now, 45, 3, 0)) {
        if (image.filename.compare(MIDNIGHT_PLUS_HALF) != 0) {
            image = Image(MIDNIGHT_PLUS_HALF);
        }
    } else if (withinTime(now, 45, 4, 30)) {
        if (image.filename.compare(MIDNIGHT_PLUS_THREE_QUARTERS) != 0) {
            image = Image(MIDNIGHT_PLUS_THREE_QUARTERS);
        }
    }

    image.draw(canvas);
}

void spinClock(Canvas* canvas) {
    Image(NOON_FILENAME).drawAndWait(canvas);
    Image(NOON_PLUS_ONE_QUARTER).drawAndWait(canvas);
    Image(NOON_PLUS_HALF).drawAndWait(canvas);
    Image(NOON_PLUS_THREE_QUARTERS).drawAndWait(canvas);
    Image(DUSK_FILENAME).drawAndWait(canvas);
    Image(MIDNIGHT_MINUS_THREE_QUARTERS).drawAndWait(canvas);
    Image(MIDNIGHT_MINUS_HALF).drawAndWait(canvas);
    Image(MIDNIGHT_MINUS_ONE_QUARTER).drawAndWait(canvas);
    Image(MIDNIGHT_FILENAME).drawAndWait(canvas);
    Image(MIDNIGHT_PLUS_ONE_QUARTER).drawAndWait(canvas);
    Image(MIDNIGHT_PLUS_HALF).drawAndWait(canvas);
    Image(MIDNIGHT_PLUS_THREE_QUARTERS).drawAndWait(canvas);
    Image(DAWN_FILENAME).drawAndWait(canvas);
    Image(NOON_MINUS_THREE_QUARTERS).drawAndWait(canvas);
    Image(NOON_MINUS_HALF).drawAndWait(canvas);
    Image(NOON_MINUS_ONE_QUARTER).drawAndWait(canvas);
    Image(NOON_FILENAME).drawAndWait(canvas);
}

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

    srand(time(NULL));

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
    Image currentImage = Image();

    if (canvas == NULL) {
        return EXIT_FAILURE;
    }

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    int iteration = 0;
    // Color background = randomColor();

    MinecraftClock* minecraftClock = new MinecraftClock(canvas);

    // while (!program_interrupted) {
    //     canvas->Clear();

    //     drawCurrentTime(canvas, defaultTextColor);
    //     drawCurrentClockFace(canvas, currentImage);

    //     if (!program_interrupted) {
    //         usleep(1 * 100000);
    //     }

    //     // Color foreground = randomColor();
    //     // draw(canvas, background, foreground);
    //     // background = foreground;
    //     if (iteration++ % 80 == 79) {
    //         spinClock(canvas);
    //     }
    // }

    canvas->Clear();
    delete canvas;
    delete minecraftClock;
    return EXIT_SUCCESS;
}
