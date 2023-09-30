#ifndef __IO_THREAD_H__
#define __IO_THREAD_H__

#include "cloud_profiler.h"
#include "block_data.h"
#include <fstream>
#include <atomic>
#include <vector>
#include <queue>
#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>

// Set 1 to dump, 0 not to.
#define DUMP_BLOCK_WRITING_TIME 0
#define BLOCK_WRITING_TIME_BUFFER_SIZE 100000   // 0.1M

struct cmp {
  bool operator()(block_data * data1, block_data * data2) {
    return data1->block_id > data2->block_id;
  }
};

extern boost::lockfree::queue<block_data *> io_queue;
extern std::atomic<int> stop_io_thread;
extern std::atomic<bool> quick_term;
extern bool io_test;

void runIOThread();
// Function for the IO thread.
//
// 1) Dequeue a block from IO queue.
// 2) Write the block in a file.
// 3) Flush the remaining blocks in queue when there is a signal to stop.

bool enqueueBlock(block_data * block);
// Enqueue a block to compression/IO thread.
// This is determined by the block attribute, "compress".

void enqueueBlockUntilSuccess(block_data * block);
// Enqueue a block to compression/IO thread.
// The only difference with the function above is that it tries enqueuing
// until it succeeds.

bool dequeueEmptyBlock(boost::lockfree::
          spsc_queue<block_data *, boost::lockfree::capacity<RETURN_QUEUE_SIZE>> * queue,
          block_data ** block);
// Dequeue a empty block from spsc queue.
// The return value shows whether the dequeue operation is successful.
// The address of dequeued block is stored in parameter "block" after the call.

void dequeueEmptyBlockPolling(boost::lockfree::
          spsc_queue<block_data *, boost::lockfree::capacity<RETURN_QUEUE_SIZE>> * queue,
          block_data ** block);
// Try to dequeue a empty block from spsc queue until it succeeds.

void storeBlockWritingTime(timespec start, timespec end, bool print = false);
// If print = false, store the writing time of a block in memory.
// If print = true, write all the stored data to the file.

bool enqueueFullBlockFromCompThread(block_data * block);
// Enqueue the block to IO thread.

void writeCompressed(block_data * data);
// Write the compressed block data to the file.

void createPriorityQueue();
// Create priority queue for blocks that come early.

void writeUncompressedBlock(block_data * data);
// Write the uncompressed block data to the file.

void writeCompressedBlock(block_data * data);
// Write the compressed block data to the file according to the order.
// If the order is not right, delay the block to process.
// Also, close and free the block in right order.

#endif