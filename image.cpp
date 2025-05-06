#include "image.h"
#include "benchmark.h"
#include "buddy_memory.h"
#include <omp.h>
#include <chrono>
#include <vector>
#include <cmath>
#include <cstring>
#include <eigen3/Eigen/Dense>
#include <fstream>
#include <iostream>
#include <sys/resource.h>

using namespace std;

BuddyMemoryManager *buddyManager = nullptr;

/**
 * @brief Retrieves the memory usage of the current program in MB.
 *
 * This function uses `getrusage` to get memory statistics and converts
 * the result from KB to MB.
 *
 * @return double The memory usage in MB.
 */
double getMemoryUsageMB()
{
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  return usage.ru_maxrss / 1024.0; // Convert KB to MB
}

/**
 * @brief Default constructor for the Image class.
 *
 * Initializes the image properties such as width, height, channels, and
 * sets the data pointer to nullptr.
 */
Image::Image()
    : width(0), height(0), channels(0), data(nullptr), useBuddySystem(false) {}

/**
 * @brief Loads an image from the specified file path.
 *
 * This function uses the `stbi_load` function to load the image and store
 * its data in the class. It prints the image's dimensions and the number
 * of color channels. If an error occurs, it outputs an error message.
 *
 * @param path The file path of the image to load.
 */
void Image::image(const char *path)
{
  // Load the image and store it in the class members
  data = stbi_load(path, &width, &height, &channels, 0);

  if (data)
  {
    cout << "+---------------------------+\n";
    cout << "       Imagen Cargada      \n";
    cout << "+---------------------------+\n";
    cout << " Dimensiones: " << width << " x " << height << "\n";
    cout << " Canales: " << channels << " (RGB) \n";
    if (buddyManager == nullptr)
    {
      // Allocate enough memory for transformations (e.g., 4x the original image
      // size)
      size_t estimatedSize = width * height * channels * 4;
      buddyManager = new BuddyMemoryManager(estimatedSize);
    }
  }
  else
  {
    cerr << "+---------------------------+\n";
    cerr << "   Error al cargar imagen  \n";
    cerr << "+---------------------------+\n";
    cerr << " Motivo: " << stbi_failure_reason() << "\n";
  }
}

/**
 * @brief Extracts the RGB channels of the loaded image.
 *
 * This function extracts the red, green, and blue color channels from the
 * image and stores them into separate vectors. The function prints a
 * success message once the channels are extracted.
 */
void Image::extractChannels()
{

  canalRojo.resize(height, vector<int>(width));
  canalVerde.resize(height, vector<int>(width));
  canalAzul.resize(height, vector<int>(width));

  for (int i = 0; i < height; i++)
  {
    for (int j = 0; j < width; j++)
    {
      canalRojo[i][j] = data[(i * width + j) * channels];
      canalVerde[i][j] = data[(i * width + j) * channels + 1];
      canalAzul[i][j] = data[(i * width + j) * channels + 2];
    }
  }

  cout << "Canales extraídos.\n";
}

/**
 * @brief Rotates the image by a specified angle.
 *
 * This function performs a 2D rotation transformation on the image. The
 * angle is provided in degrees, and the function calculates the new
 * bounding box size to accommodate the rotated image. The rotated image is
 * then saved to the disk.
 *
 * @param angle The angle by which to rotate the image in degrees.
 */
void Image::rotateImage(int angle)
{
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
  rotatedImage.useBuddySystem = useBuddySystem;
  if (useBuddySystem && buddyManager != nullptr)
  {
    rotatedImage.data = static_cast<unsigned char *>(
        buddyManager->allocate(newWidth * newHeight * channels));
    memset(rotatedImage.data, 0, newWidth * newHeight * channels);
  }
  else
  {
    rotatedImage.data = new unsigned char[newWidth * newHeight * channels]();
  }

  rotatedImage.width = newWidth;
  rotatedImage.height = newHeight;
  rotatedImage.channels = channels;

  // Find the center of the original and new images
  Eigen::Vector2f centerOriginal(width / 2.0, height / 2.0);
  Eigen::Vector2f centerNew(newWidth / 2.0, newHeight / 2.0);

  // Iterate over each pixel in the new image
  for (int i = 0; i < newHeight; i++)
  {
    for (int j = 0; j < newWidth; j++)
    {
      // Compute new pixel coordinates relative to center
      Eigen::Vector2f newCoords(j, i);
      Eigen::Vector2f oldCoords =
          rotationMatrix.inverse() * (newCoords - centerNew) + centerOriginal;

      int x = round(oldCoords[0]);
      int y = round(oldCoords[1]);

      // Check if the transformed coordinates are inside the original image
      // bounds
      if (x >= 0 && x < width && y >= 0 && y < height)
      {
        // Copy pixel values from original image
        for (int c = 0; c < channels; c++)
        {
          rotatedImage.data[(i * newWidth + j) * channels + c] =
              data[(y * width + x) * channels + c];
        }
      }
      else
      {
        // Assign black pixels for out-of-bounds areas
        for (int c = 0; c < channels; c++)
        {
          rotatedImage.data[(i * newWidth + j) * channels + c] = 0;
        }
      }
    }
  }

  cout << "Rotación completa" << endl;

  rotatedImage.saveImage("./output/rotated.jpg");

  if (useBuddySystem && buddyManager != nullptr &&
      buddyManager->isManaged(rotatedImage.data))
  {
    buddyManager->deallocate(rotatedImage.data);
    rotatedImage.data = nullptr;
  }
}

/**
 * @brief Scales the image by a specified factor.
 *
 * This function resizes the image based on a scale factor. It performs
 * bilinear interpolation to ensure the image is scaled smoothly. The scaled
 * image is then saved to the disk.
 *
 * @param scaleFactor The factor by which to scale the image.
 */
void Image::scaleImage(float scaleFactor)
{
  if (scaleFactor <= 0)
  {
    cerr << "El factor de escala debe ser mayor que 0." << endl;
    return;
  }

  // Calculate new size
  int newWidth = static_cast<int>(width * scaleFactor);
  int newHeight = static_cast<int>(height * scaleFactor);

  // Create new blank image data
  Image scaledImage;
  scaledImage.useBuddySystem = useBuddySystem;

  if (useBuddySystem && buddyManager != nullptr)
  {
    scaledImage.data = static_cast<unsigned char *>(
        buddyManager->allocate(newWidth * newHeight * channels));
    memset(scaledImage.data, 0, newWidth * newHeight * channels);
  }
  else
  {
    scaledImage.data = new unsigned char[newWidth * newHeight * channels]();
  }

  scaledImage.width = newWidth;
  scaledImage.height = newHeight;
  scaledImage.channels = channels;

  float scaleX = static_cast<float>(width) / newWidth;
  float scaleY = static_cast<float>(height) / newHeight;

  // Interpolation
  for (int i = 0; i < newHeight; i++)
  {
    for (int j = 0; j < newWidth; j++)
    {
      float srcX = j * scaleX;
      float srcY = i * scaleY;

      int x1 = static_cast<int>(srcX);
      int y1 = static_cast<int>(srcY);
      int x2 = min(x1 + 1, width - 1);
      int y2 = min(y1 + 1, height - 1);

      float dx = srcX - x1;
      float dy = srcY - y1;

      for (int c = 0; c < channels; c++)
      {
        float pixelValue =
            (1 - dx) * (1 - dy) * data[(y1 * width + x1) * channels + c] +
            dx * (1 - dy) * data[(y1 * width + x2) * channels + c] +
            (1 - dx) * dy * data[(y2 * width + x1) * channels + c] +
            dx * dy * data[(y2 * width + x2) * channels + c];

        scaledImage.data[(i * newWidth + j) * channels + c] =
            static_cast<unsigned char>(pixelValue);
      }
    }
  }

  cout << "Escalado completado! Nuevo tamaño: " << newWidth << " x "
       << newHeight << endl;

  scaledImage.saveImage("./output/scaled.jpg");
  if (useBuddySystem && buddyManager != nullptr &&
      buddyManager->isManaged(scaledImage.data))
  {
    buddyManager->deallocate(scaledImage.data);
    scaledImage.data = nullptr;
  }
}

/**
 * @brief Processes a specific quadrant of the image and applies the transformation (rotation + scaling).
 *
 * This function processes a rectangular region (quadrant) of the transformed image based on the
 * provided bounds (start and end rows and columns). It computes the corresponding coordinates in
 * the original image using the inverse transformation matrix, and then maps the pixel values from
 * the original image to the transformed image.
 *
 * The function performs the following steps:
 * 1. For each pixel in the specified quadrant, it calculates the corresponding pixel coordinates
 *    in the original image by applying the inverse transformation matrix.
 * 2. It then checks whether the calculated coordinates are within the bounds of the original image.
 * 3. If the coordinates are valid, it copies the pixel values from the original image to the transformed
 *    image; otherwise, it sets the pixel value in the transformed image to zero (black).
 *
 * @param transformedImage The image that will store the result of the transformation.
 * @param transformMatrix The transformation matrix (including rotation and scaling).
 * @param centerOriginal The center coordinates of the original image.
 * @param centerNew The center coordinates of the transformed image.
 * @param startRow The starting row index for the quadrant.
 * @param endRow The ending row index for the quadrant.
 * @param startCol The starting column index for the quadrant.
 * @param endCol The ending column index for the quadrant.
 * @param width The width of the original image.
 * @param height The height of the original image.
 * @param channels The number of color channels in the image (e.g., 3 for RGB).
 * @param data The raw pixel data of the original image.
 */
void processQuadrant(Image &transformedImage, const Eigen::Matrix2f &transformMatrix,
                     const Eigen::Vector2f &centerOriginal, const Eigen::Vector2f &centerNew,
                     int startRow, int endRow, int startCol, int endCol,
                     int width, int height, int channels, unsigned char *data)
{
  for (int i = startRow; i < endRow; i++)
  {
    for (int j = startCol; j < endCol; j++)
    {
      Eigen::Vector2f newCoords(j, i);
      Eigen::Vector2f oldCoords = transformMatrix.inverse() * (newCoords - centerNew) + centerOriginal;

      int x = round(oldCoords[0]);
      int y = round(oldCoords[1]);

      unsigned char* imageData = transformedImage.getData();

      if (x >= 0 && x < width && y >= 0 && y < height)
      {
        for (int c = 0; c < channels; c++)
        {
          imageData[(i * transformedImage.getWidth() + j) * channels + c] =
              data[(y * width + x) * channels + c];
        }
      }
      else
      {
        for (int c = 0; c < channels; c++)
        {
          imageData[(i * transformedImage.getWidth() + j) * channels + c] = 0;
        }
      }
    }
  }
}

/**
 * @brief Transforms the image by applying rotation and scaling.
 *
 * This function combines both rotation and scaling transformations
 * in sequence. It also tracks the time and memory usage of the process.
 *
 * @param inputPath The path to the input image.
 * @param outputPath The path where the transformed image will be saved.
 * @param angle The rotation angle in degrees.
 * @param scaleFactor The scaling factor.
 * @param buddySystem A flag indicating whether to use the buddy system for
 * memory allocation.
 */
void Image::transformImage(const string &inputPath, const string &outputPath,
                           int angle, float scaleFactor, bool buddySystem,
                           bool showOutput, int numDivisions) {
  using namespace std::chrono;
                            
  useBuddySystem = buddySystem;

  auto start = high_resolution_clock::now();
  double memoryBefore = getMemoryUsageMB();

  image(inputPath.c_str());

  if (scaleFactor <= 0) {
    if (showOutput) {
      cerr << "El factor de escala debe ser mayor que 0." << endl;
    }
    return;
  }

  // Setup for the transformMatrix and new image dimensions
  double radians = angle * M_PI / 180.0;
  Eigen::Matrix2f transformMatrix;
  transformMatrix << scaleFactor * cos(radians), -scaleFactor * sin(radians),
      scaleFactor * sin(radians), scaleFactor * cos(radians);

  int newWidth = abs(width * scaleFactor * cos(radians)) +
                 abs(height * scaleFactor * sin(radians));
  int newHeight = abs(width * scaleFactor * sin(radians)) +
                  abs(height * scaleFactor * cos(radians));

  Image transformedImage;
  transformedImage.useBuddySystem = useBuddySystem;

  auto buddyStart = high_resolution_clock::now();
  if (useBuddySystem && buddyManager != nullptr) {
    transformedImage.data = static_cast<unsigned char *>(buddyManager->allocate(newWidth * newHeight * channels));
    memset(transformedImage.data, 0, newWidth * newHeight * channels);
  } else {
    transformedImage.data = new unsigned char[newWidth * newHeight * channels]();
  }
  auto buddyEnd = high_resolution_clock::now();
  auto buddyDuration = duration_cast<milliseconds>(buddyEnd - buddyStart);

  transformedImage.width = newWidth;
  transformedImage.height = newHeight;
  transformedImage.channels = channels;

  Eigen::Vector2f centerOriginal(width / 2.0, height / 2.0);
  Eigen::Vector2f centerNew(newWidth / 2.0, newHeight / 2.0);

  // Divisions for processing the image in parallel
  int blockRows = newHeight / numDivisions;
  int blockCols = newWidth / numDivisions;

  // Vector to store time per thread
  std::vector<long long> threadTimes(omp_get_max_threads(), 0);

  #pragma omp parallel for collapse(2)
  for (int rowDiv = 0; rowDiv < numDivisions; rowDiv++) {
    for (int colDiv = 0; colDiv < numDivisions; colDiv++) {
      int startRow = rowDiv * blockRows;
      int endRow = (rowDiv == numDivisions - 1) ? newHeight : (rowDiv + 1) * blockRows;
      int startCol = colDiv * blockCols;
      int endCol = (colDiv == numDivisions - 1) ? newWidth : (colDiv + 1) * blockCols;

      auto threadStart = high_resolution_clock::now(); 

      processQuadrant(transformedImage, transformMatrix, centerOriginal, centerNew,
                      startRow, endRow, startCol, endCol,
                      width, height, channels, data);

      auto threadEnd = high_resolution_clock::now();
      long long threadDuration = duration_cast<milliseconds>(threadEnd - threadStart).count();
      int threadId = omp_get_thread_num();
      threadTimes[threadId] += threadDuration; 
    }
  }

  auto stop = high_resolution_clock::now();
  auto totalDuration = duration_cast<milliseconds>(stop - start);

  double memoryAfter = getMemoryUsageMB();
  double memoryUsed = memoryAfter - memoryBefore;

  // Show output for processing times

    cout << "\033[32m+---------------------------+\n";
    cout << "   TIEMPO DE PROCESAMIENTO   \n";
    cout << "+---------------------------+\n";
    cout << "- Total Processing Time: " << totalDuration.count() << " ms\n";
    cout << "- Tiempo de asignación con Buddy: " << buddyDuration.count() << " ms\n";
    cout << "- Memoria utilizada: " << memoryUsed << " MB\n\033[0m";


    // Print out the time for each thread
    cout << "\033[32m+---------------------------+\n";
    cout << "       CONCURRENCIA  \n";
    cout << "+---------------------------+\n";
    cout << "+ Número de divisiones: " << numDivisions << "\n";
    cout << "+ Número de filas: " << blockRows << "\n";
    cout << "+ Número de columnas: " << blockCols << "\n";
    cout << "+ Número de hilos: " << omp_get_max_threads() << "\n";

    for (size_t i = 0; i < threadTimes.size(); i++) {
      if (threadTimes[i] > 0) {
        cout << "🧵 Hilo " << i+1 << " tiempo: " << threadTimes[i] << " ms" << endl;
      }
    }
  

  transformedImage.saveImage(outputPath);

  if (useBuddySystem && buddyManager != nullptr && buddyManager->isManaged(transformedImage.data)) {
    buddyManager->deallocate(transformedImage.data);
    transformedImage.data = nullptr;
  }
}


/**
 * @brief Saves the image data to the specified file path.
 *
 * This function writes the image to the disk in JPG format. If the data is
 * invalid, an error message is displayed.
 *
 * @param outputPath The file path where the image will be saved.
 */
void Image::saveImage(const string &outputPath)
{
  if (!data)
  {
    cerr << "[ERROR] No hay datos de imagen disponibles para guardar\n";
    return;
  }

  if (stbi_write_jpg(outputPath.c_str(), width, height, channels, data, 100))
  {
    cout << "[INFO] Imagen guardada correctamente en " << outputPath << "\n";
  }
  else
  {
    cerr << "[ERROR] Error al guardar la imagen \n";
  }
}

/**
 * @brief Destructor for the Image class.
 *
 * Frees the allocated image data memory to avoid memory leaks.
 */
Image::~Image()
{
  if (data)
  {
    if (useBuddySystem && buddyManager != nullptr &&
        buddyManager->isManaged(data))
    {
      buddyManager->deallocate(data);
    }
    else
    {
      stbi_image_free(data);
    }
    data = nullptr;
  }
}