#include "image.h"
#include <cstdlib>  // For std::stoi()
#include <cstring>  // For strcmp
#include <iostream>
#include <vector>
#include <locale>

int main(int argc, char *argv[]) {

    Image img;
    int angle = 0;
    float scaleFactor = 1.0f;
    bool buddySystem = false;
    std::string inputPath = "./test/fish.jpg";
    std::string outputPath = "./output/output.jpg";

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-angulo") == 0 && i + 1 < argc) {
            angle = std::stoi(argv[i + 1]);
        } 
        else if (strcmp(argv[i], "-escalar") == 0 && i + 1 < argc) {
          scaleFactor = std::stof(argv[i + 1]);
        } 
        else if (strcmp(argv[i], "-entrada") == 0 && i + 1 < argc) {
            inputPath = argv[i + 1];
        } 
        else if (strcmp(argv[i], "-salida") == 0 && i + 1 < argc) {
            outputPath = argv[i + 1];
        }
        else if (strcmp(argv[i], "-buddy") == 0) {
            buddySystem = true;
        }
    }

    // Apply transformations
    img.transformImage(inputPath, outputPath, angle, scaleFactor, buddySystem);

    return 0;
}
