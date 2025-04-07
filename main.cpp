#include "buddy_memory.h"
#include "image.h"
#include <cstdlib> // For std::stoi() and std::system()
#include <cstring> // For strcmp
#include <iostream>
#include <locale>
#include <sstream> // For std::ostringstream
#include <vector>

extern BuddyMemoryManager *buddyManager;

/**
 * @brief Main function to handle image transformation operations.
 *
 * This program applies transformations such as rotation and scaling to an
 * image. It accepts command-line arguments to specify the input image, output
 * path, rotation angle, scaling factor, and whether to use the buddy system for
 * processing.
 *
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line arguments.
 *        - "-angulo <value>": Specifies the rotation angle in degrees.
 *        - "-escalar <value>": Specifies the scaling factor (float).
 *        - "-entrada <path>": Specifies the input image file path.
 *        - "-salida <path>": Specifies the output image file path.
 *        - "-buddy": Enables the buddy system for processing.
 *
 * @return int Returns 0 upon successful execution.
 */
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
    } else if (strcmp(argv[i], "-escalar") == 0 && i + 1 < argc) {
      scaleFactor = std::stof(argv[i + 1]);
    } else if (strcmp(argv[i], "-entrada") == 0 && i + 1 < argc) {
      inputPath = argv[i + 1];
    } else if (strcmp(argv[i], "-salida") == 0 && i + 1 < argc) {
      outputPath = argv[i + 1];
    } else if (strcmp(argv[i], "-buddy") == 0) {
      buddySystem = true;
    }
  }

  // Apply transformations
  img.transformImage(inputPath, outputPath, angle, scaleFactor, buddySystem,
                     true);

  if (buddyManager != nullptr) {
    delete buddyManager;
    buddyManager = nullptr;
  }

  // Construct the command with parameters
  std::ostringstream command;
  command << "./Benchmark -entrada " << inputPath << " -angulo " << angle
          << " -escalar " << scaleFactor;

  // Execute Benchmarks
  int result = std::system(command.str().c_str());
  if (result != 0) {
    std::cerr << "Error: Failed to execute ./Benchmark" << std::endl;
    return result;
  }

  return 0;
}
