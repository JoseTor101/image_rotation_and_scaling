#include "image.h"
#include <cstdlib> // For std::stoi()
#include <cstring> // For strcmp
#include <iostream>
#include <vector>

int main(int argc, char *argv[]) {
  Image img;
  img.image("./test/fish.jpg");

  int angle = 0; // Default angle
  float scaleFactor = 1; //Default scale factor

  // Parse command line arguments
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "-angulo") == 0 && i + 1 < argc) {
      angle = std::stoi(argv[i + 1]); // Convert string to int
    } else if (strcmp(argv[i], "-escala") == 0 && i + 1 < argc) {
      scaleFactor = std::stof(argv[i + 1]);  // Convert string to float
    }
  }

  // Rotate the image with the given angle only if angle different than 0.
  if (angle != 0) {
    img.rotateImage(angle);
  }

  // Scale the image with the given scale factor only if it's different than 1.
  if (scaleFactor != 1) {
      img.scaleImage(scaleFactor);
  }

  return 0;
}
