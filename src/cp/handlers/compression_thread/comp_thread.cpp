#include "time_tsc.h"
#include "comp_thread.h"
#include "../io_thread/io_thread.h"
#include "../id_handler.h"
#include "log.h"
#include <iostream>
#include <chrono>
#include <cstring>

// create queue to compression thread as a global variable
boost::lockfree::queue<block_data *> compression_queue(COMPRESS_QUEUE_SIZE);
alignas(CACHELINE_SIZE) std::atomic<bool> stop_comp_thread(false);

void runCompThread() {
  block_data * block;

  while (!stop_comp_thread && num_buf_comp_ch != 0) {
    // Get block from queue.
    if (getBlockFromQueue(&block))
      compressBlockAndSend(block);
  }

  // Get block from queue.
  while (getBlockFromQueue(&block)) {
    compressBlockAndSend(block);
  }

  stop_io_thread.fetch_add(1);
}

bool getBlockFromQueue(block_data ** block) {
  return compression_queue.pop(*block);
}

void getBlockFromQueueUntilSuccess(block_data ** block) {
  while (!getBlockFromQueue(block)) { ; }
}

uint8_t * compressBlock(block_data * block, size_t * compressed_size) {
  // Calculate uncompressed data size
  size_t uncompressed_length = block->block_idx * sizeof(*(block->block));

  // Get maximum value of compressed size for new allocation.
  const char * codec = (block->ch_cl->format == ZSTD) ? "zstd" : "lzo1x";
  const char * level = (block->ch_cl->format == ZSTD) ? "1" : "12";

  SquashCodec * c = squash_get_codec(codec);

  // set options
  SquashOptions * opts = squash_options_new(c, NULL);
  squash_object_ref_sink(opts);

  if (SQUASH_OK != squash_options_parse_option(opts, "level", level)) {
    std::cerr << "parsing fail" << std::endl;
  }

  size_t compressed_length =
              squash_codec_get_max_compressed_size(c, uncompressed_length);
  uint8_t * compressed_data = new uint8_t[compressed_length];

  auto begin = std::chrono::high_resolution_clock::now();
  SquashStatus result = squash_codec_compress_with_options(c, &compressed_length,
        compressed_data, uncompressed_length, (uint8_t *) block->block, opts);
  auto end = std::chrono::high_resolution_clock::now();
  auto dur = end - begin;
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();

  //std::cout << ms << '\n';

  // Check the result
  if (result != SQUASH_OK) {
    char log_buffer[1000] = "Unable to compress data: ";
    strcat(log_buffer, squash_status_to_string(result));
    logFail(__FILE__, __LINE__, log_buffer);
    return NULL;
  } else {
    *compressed_size = compressed_length;
    return compressed_data;
  }
}

void compressBlockAndSend(block_data * block) {
    size_t compressed_size;

    // Compress Data
    uint8_t * compressed = compressBlock(block, &compressed_size);

    // Copy the block and send back the block
    block->block_idx.store(0);
    block_data * new_block = new block_data(*block);
    sendBackBlockUntilSuccess(block);

    // enqueue to IO Queue
    new_block->block_idx.exchange(compressed_size);
    new_block->block = (log_data *) compressed;
    enqueueFullBlockFromCompThread(new_block);
}