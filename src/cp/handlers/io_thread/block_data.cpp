#include "block_data.h"
#include "../buffer_id_handler.h"

#include <fstream>
#include <boost/lockfree/spsc_queue.hpp>

block_data::block_data(bool compress, closure * ch_cl, boost::lockfree
  ::spsc_queue<block_data *, boost::lockfree::capacity<RETURN_QUEUE_SIZE>> * queue)
  : block_idx(0) {
  this->block_id = 0;
  this->block = new log_data[BUFFER_BLOCK_SIZE];
  this->ch_cl = ch_cl;
  this->compress = compress;
  this->close_file = false;
  this->queue = queue;
}

block_data::block_data(const block_data& ref) : block_idx(0) {
  this->block_id = ref.block_id;
  this->block = NULL;
  this->compress = true;
  this->close_file = ref.close_file;
  this->ch_cl = ref.ch_cl;
  this->queue = ref.queue;
}

bool getBlock(boost::lockfree::
      spsc_queue<block_data *, boost::lockfree::capacity<RETURN_QUEUE_SIZE>> * queue,
      block_data ** block) {
  return queue->pop(*block);
}

void getBlockUntilSuccess(boost::lockfree::
      spsc_queue<block_data *, boost::lockfree::capacity<RETURN_QUEUE_SIZE>> * queue,
      block_data ** block) {
  while (!getBlock(queue, block)) { ; }
}

void sendBackBlockUntilSuccess(block_data * block) {
  while (!block->queue->push(block)) { ; }
}

bool checkBlockIsFull(block_data * writing_block) {
  return ((writing_block->block_idx == BUFFER_BLOCK_SIZE) ? true : false);
}

void writeOnBlock(block_data * writing_block, int64_t tuple, timespec * ts) {
  writing_block->block[writing_block->block_idx].ts = *ts;
  writing_block->block[writing_block->block_idx].tuple = tuple;
  writing_block->block_idx.fetch_add(1);
}

void writeOnBlockTsc(block_data * writing_block, int64_t tuple, timespec * ts) {
  writing_block->block[writing_block->block_idx].ts.tv_sec = 0;
  writing_block->block[writing_block->block_idx].ts.tv_nsec = 0;
  writing_block->block[writing_block->block_idx].ts.tsc = ts->tsc;
  writing_block->block[writing_block->block_idx].tuple = tuple;
  writing_block->block_idx.fetch_add(1);
}
