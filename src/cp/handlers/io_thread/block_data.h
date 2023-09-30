#ifndef __QUEUE_DATA_H__
#define __QUEUE_DATA_H__

#include "../buffer_id_handler.h"
#include "cloud_profiler.h"
#include "../../closure.h"
#include "globals.h"
#include <fstream>
#include <boost/lockfree/spsc_queue.hpp>

struct alignas(CACHELINE_SIZE) block_data {
  int block_id;
  log_data * block;
  std::atomic<int> block_idx;
  bool compress;
  bool close_file;
  closure * ch_cl;
  boost::lockfree::
      spsc_queue<block_data *, boost::lockfree::capacity<RETURN_QUEUE_SIZE>> * queue;
  block_data(bool compress, closure * ch_cl, boost::lockfree::
      spsc_queue<block_data *, boost::lockfree::capacity<RETURN_QUEUE_SIZE>> * queue);
  block_data(const block_data& ref);
};

bool getBlock(boost::lockfree::
      spsc_queue<block_data *, boost::lockfree::capacity<RETURN_QUEUE_SIZE>> * queue,
      block_data ** block);

void getBlockUntilSuccess(boost::lockfree::
      spsc_queue<block_data *, boost::lockfree::capacity<RETURN_QUEUE_SIZE>> * queue,
      block_data ** block);

void sendBackBlockUntilSuccess(block_data * block);
// Enqueue the given block parameter to spsc queue.
// It tries enqueuing until it succeeds, and then return.
// Calling this function let the block go back to the channel
// where it belongs at first.

bool checkBlockIsFull(block_data * writing_block);
// Check whether the parameter block is full and returns according to it.

void writeOnBlock(block_data * writing_block, int64_t tuple, timespec * ts);
// Write one log data in block.

void writeOnBlockTsc(block_data * writing_block, int64_t tuple, timespec * ts);
// Write one log data of tsc in block.

#endif
