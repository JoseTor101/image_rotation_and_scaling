#include "image.h"
#include "buddy_memory.h"
#include <chrono>
#include <iostream>
#include <vector>
#include <iomanip>
#include <sys/resource.h>
#include <algorithm>
#include <cstring>

extern BuddyMemoryManager* buddyManager;

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

// Function to print performance table
void printPerformanceTable(const std::vector<PerformanceResult>& results) {
    std::cout << "\n+--------------------------------------------------+\n";
    std::cout << "|              PERFORMANCE COMPARISON               |\n";
    std::cout << "+--------------------------------------------------+\n";
    std::cout << "| Method  | Processing (ms) | Memory (MB) | Alloc (ms) |\n";
    std::cout << "+--------------------------------------------------+\n";
    
    for (const auto& result : results) {
        std::cout << "| " << std::setw(7) << std::left << result.method 
                  << " | " << std::setw(15) << std::right << std::fixed << std::setprecision(2) << result.processingTimeMs
                  << " | " << std::setw(10) << std::right << std::fixed << std::setprecision(2) << result.memoryUsageMB
                  << " | " << std::setw(10) << std::right << std::fixed << std::setprecision(2) << result.allocationTimeMs << " |\n";
    }
    
    std::cout << "+--------------------------------------------------+\n";
    
    // Calculate and print speedup
    if (results.size() >= 2) {
        double standardTime = 0, buddyTime = 0;
        double standardMemory = 0, buddyMemory = 0;
        
        for (const auto& result : results) {
            if (result.method == "Std") {
                standardTime = result.processingTimeMs;
                standardMemory = result.memoryUsageMB;
            } else if (result.method == "Buddy") {
                buddyTime = result.processingTimeMs;
                buddyMemory = result.memoryUsageMB;
            }
        }
        
        if (standardTime > 0 && buddyTime > 0) {
            double timeSpeedup = standardTime / buddyTime;
            double memoryReduction = (standardMemory - buddyMemory) / standardMemory * 100.0;
            
            std::cout << "Time speedup with buddy system: " << std::fixed << std::setprecision(2) 
                      << timeSpeedup << "x\n";
            std::cout << "Memory reduction with buddy system: " << std::fixed << std::setprecision(2) 
                      << memoryReduction << "%\n";
        }
    }
}

// Function to run multiple benchmarks with different parameters
std::vector<PerformanceResult> runBenchmarks(const std::string& inputPath) {
    std::vector<PerformanceResult> results;
    std::vector<std::pair<int, float>> transformParams = {
        {0, 0.2},
        {45, 1.4},
        {0, 2.0},
        {30, 0.7}
    };
    
    // Function to measure memory usage
    auto getMemoryUsageMB = []() {
        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);
        return usage.ru_maxrss / 1024.0; // Convert KB to MB
    };
    
    // Run each benchmark with and without buddy system
    for (const auto& param : transformParams) {
        int angle = param.first;
        float scaleFactor = param.second;
        
        std::cout << "\nRunning benchmark with angle=" << angle 
                  << ", scale=" << scaleFactor << std::endl;
        
        // For each parameter set, run with standard allocation and buddy system
        for (bool useBuddy : {false, true}) {
            // Reset memory manager between runs
            if (buddyManager != nullptr) {
                delete buddyManager;
                buddyManager = nullptr;
            }
            
            // Measure initial memory
            double memoryBefore = getMemoryUsageMB();
            
            // Create image object
            Image img;
            img.image(inputPath.c_str());
            int width = img.getWidth();
            int height = img.getHeight();
            
            // Start timing for the full process
            auto start = std::chrono::high_resolution_clock::now();
            
            // Perform transformation
            // We need to create a modified version of transformImage that returns timing information
            // Here's a placeholder for how we'd call it:
            std::string outputPath = "./output/benchmark_" + 
                                    std::to_string(angle) + "_" + 
                                    std::to_string(static_cast<int>(scaleFactor * 10)) + 
                                    (useBuddy ? "_buddy.jpg" : "_std.jpg");
            
            // Time allocation separately
            auto allocStart = std::chrono::high_resolution_clock::now();
            
            // This would be the allocation part
            if (useBuddy && buddyManager == nullptr) {
                // Estimate size needed
                double radians = angle * M_PI / 180.0;
                int newWidth = abs(width * scaleFactor * cos(radians)) + 
                              abs(height * scaleFactor * sin(radians));
                int newHeight = abs(width * scaleFactor * sin(radians)) + 
                               abs(height * scaleFactor * cos(radians));
                size_t estimatedSize = newWidth * newHeight * img.getChannels() * 4; // Add some extra
                buddyManager = new BuddyMemoryManager(estimatedSize);
            }
            
            auto allocEnd = std::chrono::high_resolution_clock::now();
            auto allocDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
                allocEnd - allocStart).count();
            
            // Call the actual transformation
            img.transformImage(inputPath, outputPath, angle, scaleFactor, useBuddy);
            
            // End timing
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                end - start).count();
            
            // Measure memory after
            double memoryAfter = getMemoryUsageMB();
            double memoryUsed = memoryAfter - memoryBefore;
            
            // Store results
            results.push_back({
                useBuddy ? "Buddy" : "Std",
                angle,
                scaleFactor,
                width,
                height,
                memoryUsed,
                static_cast<double>(duration),
                static_cast<double>(allocDuration)
            });
        }
    }
    
    return results;
}

// Modified main function to run benchmarks
int main(int argc, char *argv[]) {
    std::string inputPath = "./test/fish.jpg";
    
    // Parse command line args for input image
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-entrada") == 0 && i + 1 < argc) {
            inputPath = argv[i + 1];
        }
    }
    
    std::cout << "Running performance benchmark with input: " << inputPath << std::endl;
    
    // Run benchmarks
    auto results = runBenchmarks(inputPath);
    
    // Print performance table
    printPerformanceTable(results);
    
    // Clean up
    if (buddyManager != nullptr) {
        delete buddyManager;
        buddyManager = nullptr;
    }
    
    return 0;
}