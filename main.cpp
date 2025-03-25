#include <vector>
#include <iostream>
#include <cstring>  // For strcmp
#include <cstdlib>  // For std::stoi()
#include "image.h"

int main(int argc, char *argv[]) {
    Image img;
    img.image("./test/fish.jpg");

    int angle = 0;  // Default angle

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-angulo") == 0 && i + 1 < argc) {
            angle = std::stoi(argv[i + 1]);  // Convert string to int
        }
    }

    // Rotate the image with the given angle
    img.rotateImage(angle);

    return 0;
}
