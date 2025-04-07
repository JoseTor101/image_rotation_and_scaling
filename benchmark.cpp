#include "buddy_memory.h"
#include "image.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sys/resource.h>
#include <vector>

using namespace std;
extern BuddyMemoryManager *buddyManager;

struct PerformanceResult {
  string method;
  int angle;
  float scaleFactor;
  int imageWidth;
  int imageHeight;
  double memoryUsageMB;
  double processingTimeMs;
  double allocationTimeMs;
};

/**
 * @brief Prints a performance comparison table and calculates speedup and
 * memory reduction.
 *
 * This function takes a vector of performance results, displays them in a
 * formatted table, and calculates the time speedup and memory reduction when
 * comparing the "Std" method with the "Buddy" method, if both are present in
 * the results.
 *
 * @param results A vector of PerformanceResult objects containing method names,
 *                processing times (in milliseconds), memory usage (in MB),
 *                and allocation times (in milliseconds).
 */
void printPerformanceTable(const vector<PerformanceResult> &results) {

  cout << "\033[1;34m\n+-------------------------------------------------------"
          "-----"
          "-----------------------+\n";
  cout << "|              COMPARACIÓN DE RENDIMIENTO                     "
          "                     |\n";
  cout << "+--------------------------------------------------------------"
          "---------------------+\n";
  cout << "| Método  | Grados  | Escala   | Procesamiento (ms) | Memoria (MB) "
          "| Alloc (ns)    |\n";
  cout << "+--------------------------------------------------------------"
          "---------------------+\n";

  for (const auto &result : results) {
    cout << "| " << setw(7) << left << result.method << " | " << setw(7)
         << right << result.angle << " | " << setw(8) << right << fixed
         << setprecision(2) << result.scaleFactor << " | " << setw(15) << right
         << fixed << setprecision(2) << result.processingTimeMs << " | "
         << setw(10) << right << fixed << setprecision(6)
         << result.memoryUsageMB << " | " << setw(18) << right << fixed
         << setprecision(2) << result.allocationTimeMs << " |\n";
  }

  cout << "+--------------------------------------------------------------"
          "---------------------+\n";
  // Calcular y mostrar aceleración
  if (results.size() >= 2) {
    double tiempoEstandar = 0, tiempoBuddy = 0;
    double memoriaEstandar = 0, memoriaBuddy = 0;

    for (const auto &result : results) {
      if (result.method == "Std") {
        tiempoEstandar = result.processingTimeMs;
        memoriaEstandar = result.memoryUsageMB;
      } else if (result.method == "Buddy") {
        tiempoBuddy = result.processingTimeMs;
        memoriaBuddy = result.memoryUsageMB;
      }
    }

    if (tiempoEstandar > 0 && tiempoBuddy > 0) {
      double aceleracionTiempo = tiempoEstandar / tiempoBuddy;
      double reduccionMemoria =
          (memoriaEstandar - memoriaBuddy) / memoriaEstandar * 100.0;

      cout << "Aceleración de tiempo con sistema buddy: " << fixed
           << setprecision(2) << aceleracionTiempo << "x\n";
      cout << "Reducción de memoria con sistema buddy: " << fixed
           << setprecision(2) << reduccionMemoria << "%\n";
    }
  }
  cout << "\033[0m"; // Restablecer color
}

/**
 * @brief Runs a series of benchmarks to test image transformation performance
 *        (rotation and scaling) with and without a buddy memory manager.
 *
 * @param inputPath The file path to the input image to be transformed.
 * @return A vector of PerformanceResult objects containing benchmark results,
 *         including memory usage, execution time, and allocation time for each
 * test case.
 */
vector<PerformanceResult>
runBenchmarks(const string &inputPath,
              const vector<pair<int, float>> &transformParams) {
  vector<PerformanceResult> results;

  auto getMemoryUsageMB = []() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss / 1024.0; // Convert KB to MB
  };

  // Run each benchmark with and without buddy system
  for (const auto &param : transformParams) {
    int angle = param.first;
    float scaleFactor = param.second;

    // For each parameter set, run with standard allocation and buddy system
    for (bool useBuddy : {false, true}) {
      if (buddyManager != nullptr) {
        delete buddyManager;
        buddyManager = nullptr;
      }

      double memoryBefore = getMemoryUsageMB();

      Image img;
      img.image(inputPath.c_str());
      int width = img.getWidth();
      int height = img.getHeight();

      auto start = chrono::high_resolution_clock::now();

      string outputPath = "../output/benchmark_" + to_string(angle) + "_" +
                          to_string(static_cast<int>(scaleFactor * 10)) +
                          (useBuddy ? "_buddy.jpg" : "_std.jpg");

      auto allocStart = chrono::high_resolution_clock::now();

      // Here we stimate the size of the image to be allocated
      if (useBuddy && buddyManager == nullptr) {
        double radians = angle * M_PI / 180.0;
        int newWidth = abs(width * scaleFactor * cos(radians)) +
                       abs(height * scaleFactor * sin(radians));
        int newHeight = abs(width * scaleFactor * sin(radians)) +
                        abs(height * scaleFactor * cos(radians));
        size_t estimatedSize =
            newWidth * newHeight * img.getChannels() * 4; // Add some extra
        buddyManager = new BuddyMemoryManager(estimatedSize);
      }

      auto allocEnd = chrono::high_resolution_clock::now();
      auto allocDuration =
          chrono::duration_cast<chrono::nanoseconds>(allocEnd - allocStart)
              .count();

      // Call the actual transformation
      img.transformImage(inputPath, outputPath, angle, scaleFactor, useBuddy,
                         false);
      cout << " \n";

      auto end = chrono::high_resolution_clock::now();
      auto duration =
          chrono::duration_cast<chrono::milliseconds>(end - start).count();

      double memoryAfter = getMemoryUsageMB();
      double memoryUsed = memoryAfter - memoryBefore;

      results.push_back({useBuddy ? "Buddy" : "Std", angle, scaleFactor, width,
                         height, memoryUsed, static_cast<double>(duration),
                         static_cast<double>(allocDuration)});
    }
  }

  return results;
}

/**
 * @brief Entry point for the performance benchmarking application.
 *
 * This function parses command line arguments to determine the input image
 * path, runs performance benchmarks on the specified image, and displays the
 * results in a performance comparison table. It also ensures proper cleanup of
 * resources used during the benchmarking process.
 *
 * @param argc The number of command line arguments.
 * @param argv The array of command line arguments.
 * @return int Returns 0 on successful execution.
 */
int main(int argc, char *argv[]) {
  // Default input image path
  string inputPath = "../imgs/fish.jpg";
  int angulo = 0;
  float escalar = 1.0f;

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "-entrada") == 0 && i + 1 < argc) {
      inputPath = argv[i + 1];
    } else if (strcmp(argv[i], "-angulo") == 0 && i + 1 < argc) {
      angulo = stoi(argv[i + 1]);
    } else if (strcmp(argv[i], "-escalar") == 0 && i + 1 < argc) {
      escalar = stof(argv[i + 1]);
    }
  }

  cerr << "\033[1;33m\n+---------------------------+\033[0m\n";
  cout << "\033[1;33mEjecutando prueba de rendimiento con entrada: \033[0m"
       << inputPath << "\033[1;33m\n...\033[0m" << endl;

  // Benchmark test cases
  vector<pair<int, float>> transformParams = {
      {angulo, escalar},
  };

  auto results = runBenchmarks(inputPath, transformParams);
  printPerformanceTable(results);

  if (buddyManager != nullptr) {
    delete buddyManager;
    buddyManager = nullptr;
  }

  return 0;
}