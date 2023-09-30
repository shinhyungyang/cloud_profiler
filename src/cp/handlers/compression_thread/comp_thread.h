#ifndef __COMP_THREAD_H__
#define __COMP_THREAD_H__

#define COMP_THREAD_NUM 4
#define COMPRESS_QUEUE_SIZE 100000

#include "../io_thread/block_data.h"
#include <boost/lockfree/queue.hpp>
#include <squash.h>
#include <atomic>

// create queue to compression thread as a global variable
extern boost::lockfree::queue<block_data *> compression_queue;
extern std::atomic<bool> stop_comp_thread;

void runCompThread();
// Function for Compression thread.
// The number of compression thread is determined
// by macro COMP_THREAD_NUM above.
//
// What this function does is iterating over 1-4 until there is a signal to stop.
// 1) Dequeue a block from queue to compression thread.
// 2) Compress the block.
// 3) Send back the block. (for memory reusage)
// 4) Enqueue compressed block to IO thread.

bool getBlockFromQueue(block_data ** block);
// Get a block(dequeue) from queue to compression thread.
// This function should only be used by compression thread.
//
// Return:
// true if dequeue operation succeeds.
// false if dequeue operation fails.
//
// The dequeued data can be referenced by *block.
// Parameter: You should give the address of block_data * type.

void getBlockFromQueueUntilSuccess(block_data ** block);
// Same as the above function, getBlockFromQueue(block_data ** block).
// The only difference is that it does dequeue operation
// until it succeeds, and then return.
// Thus, there is no return value to notify
// whether the operation has succeeded.

uint8_t * compressBlock(block_data * block, size_t * compressed_size);
// Compress the block given by parameter block.
// Return:
// The pointer to compressed data.
// The size of the compressed data is also written in "compressed_size".
// Thus, you should pass the address of the size_t variable as a parameter.
//
// Note that the memory that compressed data reside is dynamically allocated.
// It should be freed after the usage.
// For now, the compressed data are created in compression thread and
// freed in IO thread after writing them to a file.
// Also, the content of the parameter "block" is not modified at all.

void compressBlockAndSend(block_data * block);
// Compress the block, send back the original block,
// and enqueue compressed data to IO thread.

#endif
