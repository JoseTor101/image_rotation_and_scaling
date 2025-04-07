#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <string>
#include <vector>

// Struct to store performance results
struct PerformanceResult {
  std::string method;
  int angle;
  float scaleFactor;
  int imageWidth;
  int imageHeight;
  double memoryUsageMB;
  double processingTimeMs;
  double allocationTimeMs;
};

// Function to print the performance table
void printPerformanceTable(const std::vector<PerformanceResult> &results);

// Function to run benchmarks
std::vector<PerformanceResult>
runBenchmarks(const std::string &inputPath,
              const std::vector<std::pair<int, float>> &transformParams);

#endif // BENCHMARK_H