#ifndef IMAGEN_H
#define IMAGEN_H

#include "stb_image.h"
#include "stb_image_write.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

class Image {
public:
  Image();  // Constructor
  ~Image(); // Destructor

  void image(const char *); // Load an image
  void extractChannels();   // Extract RGB channels
  void rotateImage(int angle);
  void scaleImage(float scaleFactor);
  void transformImage(const string &inputPath, const string &outputPath,
                      int angle, float scaleFactor, bool buddySystem);
  void saveImage(const string &outputPath); // Save image
  
  int getWidth() const { return width; }
  int getHeight() const { return height; }
  int getChannels() const { return channels; }

private:
  vector<vector<int>> canalRojo;
  vector<vector<int>> canalVerde;
  vector<vector<int>> canalAzul;
  int width, height, channels;
  unsigned char *data;
  bool useBuddySystem;
};

#endif // IMAGEN_H
