#include "time_tsc.h"
#include "cloud_profiler.h"
#include "../buffer_id_handler.h"
#include "io_thread.h"
#include "block_data.h"
#include "../id_handler.h"
#include "cleanup.h"
#include "../compression_thread/comp_thread.h"

#include <boost/lockfree/queue.hpp>
#include <iostream>
#include <cstring>
#include <atomic>
#include <string>
#include <sstream>
#include <thread>
#include <vector>
#include <queue>

bool io_test = false;
char * block_writing_time[BLOCK_WRITING_TIME_BUFFER_SIZE];
int block_writing_time_idx = 0;
alignas(CACHELINE_SIZE) std::atomic<bool> quick_term(false);
boost::lockfree::queue<block_data *> io_queue(IO_QUEUE_SIZE);
alignas(CACHELINE_SIZE) std::atomic<int> stop_io_thread(0);
alignas(CACHELINE_SIZE) std::vector<int> next_block(200, 0);
std::vector<std::priority_queue
                  <block_data *, std::vector<block_data *>, cmp> *> pending;

void runIOThread() {
  block_data * data;
  timespec start, end;

  // create priority queue for blocks that are processed early
  createPriorityQueue();

  while ((comp_thread_created && (stop_io_thread != COMP_THREAD_NUM))
                            || (!comp_thread_created && num_buf_ch != 0)) {
    // dequeue item from incoming queue and process it
    if (io_queue.pop(data)) {
      // calculate the writing time
      getTS(&start);
      if (data->compress) {
        writeCompressedBlock(data);
      } else {
        writeUncompressedBlock(data);
      }
      getTS(&end);
      storeBlockWritingTime(start, end);
    } else if (io_test) {
      break;
    }
  }

  // flush and free remaining blocks in queues
  // when stop_io_thread signal is activated.
  flushBlockInQueues(quick_term);

  buffer_write_thread = NULL;
  io_thread_created = false;

  comp_thread_created = false;
  comp_thread.clear();
}

bool enqueueBlock(block_data * block) {
  return ((block->compress) ? compression_queue.bounded_push(block) : io_queue.bounded_push(block));
}

void enqueueBlockUntilSuccess(block_data * block) {
  while (!enqueueBlock(block)) { ; }
}

bool enqueueFullBlockFromCompThread(block_data * block) {
  return io_queue.bounded_push(block);
}

bool dequeueEmptyBlock(boost::lockfree::
          spsc_queue<block_data *, boost::lockfree::capacity<RETURN_QUEUE_SIZE>> * queue,
          block_data ** block) {
  return queue->pop(*block);
}

void dequeueEmptyBlockPolling(boost::lockfree::
          spsc_queue<block_data *, boost::lockfree::capacity<RETURN_QUEUE_SIZE>> * queue,
          block_data ** block) {
  while (!dequeueEmptyBlock(queue, block)) { ; }
}

void storeBlockWritingTime(timespec start, timespec end, bool write) {
  // do nothing if the macro is not enabled.
  if (!DUMP_BLOCK_WRITING_TIME)
    return;

  if (write) {
    // open file to write block writing time
    std::ofstream block_writing_log;
    block_writing_log.open("/tmp/cloud_profiler_block_writing_time.txt",
                                          std::ios::out | std::ios::trunc);

    for (int i = 0; i < block_writing_time_idx; i++) {
      block_writing_log << block_writing_time[i] << '\n';
    }

    block_writing_log.close();
  } else {
    timespec ts;
    char * s = new char[100];

    subtractTS(&ts, &start, &end);
    formatDuration(&ts, s, 1024, true);

    // stores writing time per block.
    block_writing_time[block_writing_time_idx++] = s;
  }
}

void writeCompressed(block_data * data) {
  int block_idx = data->block_idx;
  data->ch_cl->ch_logf.write((char *) &block_idx, sizeof(block_idx));
  data->ch_cl->ch_logf.write((char *) data->block, data->block_idx);
}

void createPriorityQueue() {
  for (int i = 0; i < 200; i++) {
    pending.push_back(new std::priority_queue
                  <block_data *, std::vector<block_data *>, cmp>());
  }
}

void writeUncompressedBlock(block_data * data) {
  for (int i = 0; i < data->block_idx; i++) {
    data->ch_cl->write_handler_f(&data->ch_cl->ch_logf,
                          data->block[i].tuple, &(data->block[i].ts));
  }

  // Close the file if needed.
  if (data->close_file)
    data->ch_cl->ch_logf.close();

  // send back block
  data->block_idx.exchange(0);
  sendBackBlockUntilSuccess(data);
}

void writeCompressedBlock(block_data * data) {
  // check whether the block is in order
  if (data->block_id != next_block[data->ch_cl->ch_nr]) {
    pending[data->ch_cl->ch_nr]->push(data);
    return;
  }

  writeCompressed(data);
  delete[] (uint8_t *) data->block;
  next_block[data->ch_cl->ch_nr]++;

  // write pending data if exist
  while (pending[data->ch_cl->ch_nr]->size() != 0 &&
      pending[data->ch_cl->ch_nr]->top()->block_id ==
                                      next_block[data->ch_cl->ch_nr]) {
    block_data * temp = pending[data->ch_cl->ch_nr]->top();
    writeCompressed(temp);
    delete[] (uint8_t *) temp->block;
    next_block[data->ch_cl->ch_nr]++;
    pending[data->ch_cl->ch_nr]->pop();

    if (temp->close_file)
      temp->ch_cl->ch_logf.close();
  }

  // Close the file if needed.
  if (data->close_file)
    data->ch_cl->ch_logf.close();

  delete data;
}