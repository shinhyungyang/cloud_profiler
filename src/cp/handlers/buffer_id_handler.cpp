#include "time_tsc.h"
#include "cloud_profiler.h"
#include "globals.h"
#include "closure.h"
#include "null_handler.h"
#include "buffer_id_handler.h"
#include "id_handler.h"
#include "io_thread/block_data.h"
#include "io_thread/io_thread.h"
#include "io_thread/cleanup.h"
#include "compression_thread/comp_thread.h"
#include "log.h"

#include <cstring>
#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <boost/lockfree/queue.hpp>

std::atomic<std::thread *> buffer_write_thread(NULL);
std::vector<std::thread *> comp_thread;
alignas(CACHELINE_SIZE) std::atomic<int> spinning_count(0);
alignas(CACHELINE_SIZE) std::atomic<bool> io_thread_created(false);
alignas(CACHELINE_SIZE) std::atomic<bool> comp_thread_created(false);
alignas(CACHELINE_SIZE) std::atomic<int> num_buf_ch(0);
alignas(CACHELINE_SIZE) std::atomic<int> num_buf_comp_ch(0);

struct alignas(CACHELINE_SIZE) buffer_closure : closure {
  boost::lockfree::
          spsc_queue<block_data *, boost::lockfree::capacity<RETURN_QUEUE_SIZE>> * queue;
  block_data * block;
  int block_cnt;
  bool first_logging;
  virtual void closeChannel(void);
};

// flush the buffer and free the dynamically allocated memory.
void buffer_closure::closeChannel() {
  if (handler_f.exchange(NullHandler) == BufferIdentityHandler) {
    // In case of abnormal exit,
    // the handler will close all of the channels one by one.
    // If you send current block without waiting for cleanup,
    // there might be a thread that writes to the block currently,
    // might leading to segmentation fault.
    // Thus, for safety, wait for a second after changing handler.
    // This will let other threads finish logTS call for the closing channel,
    // and further logTS() call will not be written due to handler change.
    // Then, send remaining block and let IO thread close the file.
    //std::this_thread::sleep_for(std::chrono::seconds(1));

    block->close_file = true;
    enqueueBlockUntilSuccess(block);

    if (block->compress)
      num_buf_comp_ch--;

    num_buf_ch--;

    // Do not close the file because IO thread might not finish writing.
    logStatus(__FILE__, __LINE__, "Channel " + *ch_logf_name +
                      " will be closed by IO thread after flushing.");
  }
}

void BufferIdentityHandler(void * cl, int64_t tuple) {
  buffer_closure * ch_cl = static_cast<buffer_closure *>(cl);
  struct timespec ts;
  clock_gettime_tsc(CLOCK_REALTIME, &ts);
  
  // write to the block
  writeOnBlock(ch_cl->block, tuple, &ts);

  // enqueue if block is full
  if (checkBlockIsFull(ch_cl->block)) {
    enqueueBlockUntilSuccess(ch_cl->block);
  
    // Get new empty block for next call and set the block_id
    dequeueEmptyBlockPolling(ch_cl->queue, &(ch_cl->block));
    ch_cl->block->block_id = ch_cl->block_cnt++;
  
    if (ch_cl->block_cnt == NUM_BUFFER_BLOCK + 1)
      logStatus(__FILE__, __LINE__, "The channel waits for a fresh block. "
                              "This might lead to performance degradation.");
  }
}

void BufferIdentityHandlerTsc(void * cl, int64_t tuple) {
  buffer_closure * ch_cl = static_cast<buffer_closure *>(cl);
  struct timespec ts;

  if (ch_cl->first_logging) {
    ch_cl->first_logging = false;
    clock_gettime_tsc(CLOCK_REALTIME, &ts);
    if (ch_cl->format == ASCII_TSC) {
      writeInAscii(&(ch_cl->ch_logf), tuple, &ts);
    } else if (ch_cl->format == BINARY_TSC ||
              ch_cl->format == ZSTD_TSC || ch_cl->format == LZO1X_TSC) {
      writeInBinary(&(ch_cl->ch_logf), tuple, &ts);
    }
  } else {
    clock_gettime_tsc_only(CLOCK_REALTIME, &ts);

    // write to the block
    writeOnBlockTsc(ch_cl->block, tuple, &ts);

    // enqueue if block is full
    if (checkBlockIsFull(ch_cl->block)) {
      enqueueBlockUntilSuccess(ch_cl->block);

      // Get new empty block for next call and set the block_id
      dequeueEmptyBlockPolling(ch_cl->queue, &(ch_cl->block));
      ch_cl->block->block_id = ch_cl->block_cnt++;
    }
  }
}

closure * initBufferIdentityHandler(log_format format, bool compress) {
  buffer_closure * tmp = new buffer_closure();
  tmp->parm_handler_f = NULL;
  tmp->block_cnt = 0;
  tmp->first_logging = true;

  switch (format)
  {
  case ASCII:
    tmp->handler_f = BufferIdentityHandler;
    tmp->write_handler_f = writeInAscii;
    break;
  case ASCII_TSC:
    tmp->handler_f = BufferIdentityHandlerTsc;
    tmp->write_handler_f = writeInAsciiTsc;
    break;
  case BINARY:
    tmp->handler_f = BufferIdentityHandler;
    tmp->write_handler_f =  writeInBinary;
    break;
  case BINARY_TSC:
    tmp->handler_f = BufferIdentityHandlerTsc;
    tmp->write_handler_f = writeInBinaryTsc;
    break;
  case ZSTD: case LZO1X:
    tmp->handler_f = BufferIdentityHandler;
    break;
  case ZSTD_TSC: case LZO1X_TSC:
    tmp->handler_f = BufferIdentityHandlerTsc;
  }

  // Create IO thread
  num_buf_ch++;
  if (!io_thread_created.exchange(true)) {
    buffer_write_thread = new std::thread(runIOThread);
  }

  if (compress)
    num_buf_comp_ch++;

  // Create Compression Thread
  if (compress && !comp_thread_created.exchange(true)) {
    for (int i = 0; i < COMP_THREAD_NUM; i++) {
      comp_thread.push_back(new std::thread(runCompThread));
      logStatus(__FILE__, __LINE__, "New compression thread created");
    }
  }

  // Create SPSC Queue for this channel
  tmp->queue = new boost::lockfree::
            spsc_queue<block_data *, boost::lockfree::capacity<RETURN_QUEUE_SIZE>>();

  // Push empty blocks
  for (int i = 0; i < NUM_BUFFER_BLOCK; i++) {
    if (!tmp->queue->push(new block_data(compress, tmp, tmp->queue)))
      logFail(__FILE__, __LINE__, "SPSC Queue is full during initialization!");
  }

  // set the block to write
  getBlockUntilSuccess(tmp->queue, &tmp->block);
  tmp->block->block_id = tmp->block_cnt++;

  return tmp;
}
