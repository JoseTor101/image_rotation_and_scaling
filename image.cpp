#include "image.h"
#include <Eigen/Dense>
#include <cmath>
#include <cstring>

using namespace std;

// Constructor definition
// Image::Image() : data(nullptr), width(0), height(0), channels(0) {}
Image::Image() : width(0), height(0), channels(0), data(nullptr) {}

void Image::image(const char *path) {
  // Load the image and store it in the class members
  data = stbi_load(path, &width, &height, &channels, 0);

  if (data) {
    cout << "Imagen cargada: " << width << "x" << height << " con " << channels
         << " canales.\n";
  } else {
    cerr << "Error al cargar la imagen: " << stbi_failure_reason() << "\n";
  }
}

void Image::extractChannels() {

  canalRojo.resize(height, vector<int>(width));
  canalVerde.resize(height, vector<int>(width));
  canalAzul.resize(height, vector<int>(width));

  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      canalRojo[i][j] = data[(i * width + j) * channels];
      canalVerde[i][j] = data[(i * width + j) * channels + 1];
      canalAzul[i][j] = data[(i * width + j) * channels + 2];
    }
  }

  cout << "Canales extraídos.\n";
}

void Image::rotateImage(int angle) {
  // Convert angle to radians
  double radians = angle * M_PI / 180.0;

  // Compute new bounding box dimensions
  int newWidth = abs(width * cos(radians)) + abs(height * sin(radians));
  int newHeight = abs(width * sin(radians)) + abs(height * cos(radians));

  // Define the rotation matrix using Eigen
  Eigen::Matrix2f rotationMatrix;
  rotationMatrix << cos(radians), -sin(radians), sin(radians), cos(radians);

  // Create new blank image data
  Image rotatedImage;
  rotatedImage.data = new unsigned char[newWidth * newHeight * channels]();
  rotatedImage.width = newWidth;
  rotatedImage.height = newHeight;
  rotatedImage.channels = channels;

  // Find the center of the original and new images
  Eigen::Vector2f centerOriginal(width / 2.0, height / 2.0);
  Eigen::Vector2f centerNew(newWidth / 2.0, newHeight / 2.0);

  // Iterate over each pixel in the new image
  for (int i = 0; i < newHeight; i++) {
    for (int j = 0; j < newWidth; j++) {
      // Compute new pixel coordinates relative to center
      Eigen::Vector2f newCoords(j, i);
      Eigen::Vector2f oldCoords =
          rotationMatrix.inverse() * (newCoords - centerNew) + centerOriginal;

      int x = round(oldCoords[0]);
      int y = round(oldCoords[1]);

      // Check if the transformed coordinates are inside the original image
      // bounds
      if (x >= 0 && x < width && y >= 0 && y < height) {
        // Copy pixel values from original image
        for (int c = 0; c < channels; c++) {
          rotatedImage.data[(i * newWidth + j) * channels + c] =
              data[(y * width + x) * channels + c];
        }
      } else {
        // Assign black pixels for out-of-bounds areas
        for (int c = 0; c < channels; c++) {
          rotatedImage.data[(i * newWidth + j) * channels + c] = 0;
        }
      }
    }
  }

  cout << "Rotation complete!" << endl;

  rotatedImage.saveImage("./output/rotated.jpg");
}

// Save the transformed image
void Image::saveImage(const string &outputPath) {
  if (!data) {
    cerr << "No image data available to save!\n";
    return;
  }

  if (stbi_write_jpg(outputPath.c_str(), width, height, channels, data, 100)) {
    cout << "Transformed image saved to " << outputPath << "\n";
  } else {
    cerr << "Failed to save image.\n";
  }
}

// Destructor definition
Image::~Image() {
  if (data) {
    stbi_image_free(data);
    data = nullptr; // Avoids dangling pointer issues
  }
}
