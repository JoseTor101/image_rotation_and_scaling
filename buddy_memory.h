#ifndef BUDDY_MEMORY_H
#define BUDDY_MEMORY_H

#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <unordered_map>

class BuddyMemoryManager {
private:
    struct Block {
        size_t offset;  // Offset from the base memory address
        size_t size;    // Size of the block in bytes
        bool free;      // Is the block free or allocated
        
        Block(size_t off, size_t sz, bool f) : offset(off), size(sz), free(f) {}
    };
    
    unsigned char* memory;           // Base memory pointer
    size_t totalSize;                // Total size of memory pool
    size_t minBlockSize;             // Minimum block size (power of 2)
    std::vector<std::vector<Block>> freeLists;  // Array of free lists, indexed by log2(size)
    std::unordered_map<void*, size_t> allocatedBlocks;  // Map of allocated pointers to their sizes
    
    // Helper functions
    size_t log2Ceil(size_t n) {
        size_t result = 0;
        size_t value = 1;
        while (value < n) {
            value <<= 1;
            result++;
        }
        return result;
    }
    
    size_t getSizeClass(size_t size) {
        return log2Ceil(size) - log2Ceil(minBlockSize);
    }
    
    size_t roundUpToNextPowerOf2(size_t n) {
        size_t result = 1;
        while (result < n) {
            result <<= 1;
        }
        return result;
    }
    
    Block* findBlock(size_t sizeClass) {
        // If free list is empty at this size class
        if (sizeClass >= freeLists.size() || freeLists[sizeClass].empty()) {
            // Try to get a block from a larger size class
            if (sizeClass + 1 >= freeLists.size()) {
                return nullptr;  // No larger blocks available
            }
            
            Block* largerBlock = findBlock(sizeClass + 1);
            if (!largerBlock) {
                return nullptr;  // No larger blocks available
            }
            
            // Split the larger block
            size_t halfSize = largerBlock->size / 2;
            size_t offset = largerBlock->offset;
            
            // Remove larger block from its free list
            auto& largerList = freeLists[sizeClass + 1];
            for (auto it = largerList.begin(); it != largerList.end(); ++it) {
                if (it->offset == largerBlock->offset) {
                    largerList.erase(it);
                    break;
                }
            }
            
            // Add the two halves to the current size class
            freeLists[sizeClass].push_back(Block(offset, halfSize, true));
            freeLists[sizeClass].push_back(Block(offset + halfSize, halfSize, true));
            
            // Return the first half
            return &freeLists[sizeClass][freeLists[sizeClass].size() - 2];
        }
        
        // Return the first free block in this size class
        return &freeLists[sizeClass][0];
    }
    
    bool mergeBuddies() {
        bool merged = false;
        
        for (size_t i = 0; i < freeLists.size() - 1; ++i) {
            auto& list = freeLists[i];
            size_t blockSize = minBlockSize << i;
            
            for (size_t j = 0; j < list.size(); ++j) {
                for (size_t k = j + 1; k < list.size(); ++k) {
                    Block& b1 = list[j];
                    Block& b2 = list[k];
                    
                    // Check if they're buddies (adjacent and aligned)
                    if (b1.free && b2.free && 
                        ((b1.offset ^ b2.offset) == blockSize) && 
                        (b1.offset & blockSize) == 0) {
                        
                        // Merge them into a higher-level block
                        size_t mergedOffset = std::min(b1.offset, b2.offset);
                        
                        // Remove both blocks from current list
                        if (j > k) {
                            list.erase(list.begin() + j);
                            list.erase(list.begin() + k);
                        } else {
                            list.erase(list.begin() + k);
                            list.erase(list.begin() + j);
                        }
                        
                        // Add merged block to the higher level free list
                        freeLists[i + 1].push_back(Block(mergedOffset, blockSize * 2, true));
                        
                        merged = true;
                        break;
                    }
                }
                if (merged) break;
            }
            if (merged) break;
        }
        
        return merged;
    }
    
public:
    BuddyMemoryManager(size_t size, size_t minSize = 64) {
        // Round sizes to powers of 2
        totalSize = roundUpToNextPowerOf2(size);
        minBlockSize = roundUpToNextPowerOf2(minSize);
        
        // Allocate memory pool
        memory = new unsigned char[totalSize];
        
        // Calculate number of size classes
        size_t numClasses = log2Ceil(totalSize / minBlockSize) + 1;
        freeLists.resize(numClasses);
        
        // Initialize with one large free block
        freeLists[numClasses - 1].push_back(Block(0, totalSize, true));
    }
    
    ~BuddyMemoryManager() {
        delete[] memory;
    }
    
    void* allocate(size_t size) {
        if (size == 0) return nullptr;
        
        // Round up size to the nearest multiple of minBlockSize
        size_t roundedSize = ((size + minBlockSize - 1) / minBlockSize) * minBlockSize;
        
        // Round up to the next power of 2
        size_t blockSize = roundUpToNextPowerOf2(roundedSize);
        blockSize = std::max(blockSize, minBlockSize);
        
        // Find the size class
        size_t sizeClass = getSizeClass(blockSize);
        
        if (sizeClass >= freeLists.size()) {
            std::cerr << "Requested block size too large" << std::endl;
            return nullptr;
        }
        
        // Find a suitable block
        Block* block = findBlock(sizeClass);
        if (!block) {
            std::cerr << "Out of memory" << std::endl;
            return nullptr;
        }
        
        // Mark as allocated
        block->free = false;
        
        // Remove from free list
        auto& list = freeLists[sizeClass];
        for (auto it = list.begin(); it != list.end(); ++it) {
            if (it->offset == block->offset) {
                list.erase(it);
                break;
            }
        }
        
        // Add to allocated map
        void* ptr = memory + block->offset;
        allocatedBlocks[ptr] = block->size;
        
        return ptr;
    }
    
    void deallocate(void* ptr) {
        if (!ptr) return;
        
        // Find the block size
        auto it = allocatedBlocks.find(ptr);
        if (it == allocatedBlocks.end()) {
            std::cerr << "Invalid pointer for deallocation" << std::endl;
            return;
        }
        
        size_t size = it->second;
        size_t offset = static_cast<unsigned char*>(ptr) - memory;
        
        // Remove from allocated map
        allocatedBlocks.erase(it);
        
        // Add to the appropriate free list
        size_t sizeClass = getSizeClass(size);
        freeLists[sizeClass].push_back(Block(offset, size, true));
        
        // Try to merge buddies
        while (mergeBuddies()) {
            // Keep merging until no more merges are possible
        }
    }
    
    bool isManaged(void* ptr) {
        return allocatedBlocks.find(ptr) != allocatedBlocks.end();
    }
    
    size_t getAllocatedSize(void* ptr) {
        auto it = allocatedBlocks.find(ptr);
        if (it != allocatedBlocks.end()) {
            return it->second;
        }
        return 0;  // Not managed by this allocator
    }
};

#endif // BUDDY_MEMORY_H