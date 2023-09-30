#ifndef __BUFFER_ID_HANDLER_H__
#define __BUFFER_ID_HANDLER_H__ // include-guard to prevent double-inclusion

// to evade mutual reference with closure.h
struct buffer_id_handler;
struct log_data;
struct queue_data_in;
struct queue_data_out;
struct block_data;

#define IO_QUEUE_SIZE 100000 // 10000 -> 100000 20220512
#define RETURN_QUEUE_SIZE 10000 // 1000 -> 10000 20220512

#include "time_tsc.h"
#include "io_thread/block_data.h"
#include "../closure.h"
#include "globals.h"

#include <cstdint>
#include <atomic>
#include <vector>
#include <thread>
#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>

extern std::atomic<std::thread *> buffer_write_thread;
extern std::vector<std::thread *> comp_thread;
extern std::atomic<int> spinning_count;
extern std::atomic<bool> io_thread_created;
extern std::atomic<bool> comp_thread_created;
extern std::atomic<int> num_buf_ch;
extern std::atomic<int> num_buf_comp_ch;

#define NUM_BUFFER_BLOCK 32 // 8 -> 32 20220512
//#define NUM_BUFFER_BLOCK 1001 // for cp_cppiotest
#define BUFFER_BLOCK_SIZE 1000000  // 1M
// Each tuple is 32 bytes.
// A block can store (BUFFER_BLOCK_SIZE) tuples.
// A channel that uses Buffer Identity Handler has (NUM_BUFFER_BLOCK) blocks.

// Thus, total buffer size per channel is
// (32 * BUFFER_BLOCK_SIZE * NUM_BUFFER_BLOCK) bytes.

struct log_data {
  int64_t tuple;
  timespec ts;
};

void BufferIdentityHandler(void * cl, int64_t tuple);
// Write time-stamp for "tuple" to block and enqueue it when it is full.

void BufferIdentityHandlerTsc(void * cl, int64_t tuple);
// Write tsc for "tuple" to block and enqueue it when it is full.

closure * initBufferIdentityHandler(log_format format, bool compress);
// Set up and return the closure of an buffer_id_handler.

#endif
