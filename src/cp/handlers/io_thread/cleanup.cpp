#include "time_tsc.h"
#include "cleanup.h"
#include "cloud_profiler.h"
#include "../id_handler.h"
#include "../../closure.h"
#include "block_data.h"
#include "io_thread.h"
#include "../compression_thread/comp_thread.h"
#include "../buffer_id_handler.h"
#include "os/cpu_util.h"

#include <thread>
#include <iostream>
#include <vector>
#include <mutex>
#include <cstring>
#include <csignal>
#include <queue>
#include <boost/lockfree/queue.hpp>

// get the channel info to close the channel
extern std::vector<struct closure *> ch_info;
extern std::mutex mtx_bk;

void closeAllChannels() {
  mtx_bk.lock();
  for (unsigned int i = 0; i < ch_info.size(); i++) {
    // NOTE: closing channel that has been closed would cause a warning log.
    // FIXME: closing channel with Buffer ID handler will
    // take one second per one channel.
    ch_info[i]->closeChannel();
  }
  mtx_bk.unlock();
}

void cleanUpAtNormalExit() {
  stop_oversubs_chk = true;

  // closing all of the channels is needed
  // because the application might not close them.
  closeAllChannels();

  // Wait until IO thread finish writing.
  if (buffer_write_thread != NULL) {
    buffer_write_thread.load()->join();
    delete buffer_write_thread;
  }
}

void cleanUpAtAbnormalExit(int signum)
{
  stop_oversubs_chk = true;

  if (num_buf_comp_ch != 0 || buffer_write_thread != NULL) {
    // If there is a IO thread, let it terminate the program.
    quick_term = true;
    closeAllChannels();
  } else {
    // Otherwise, terminate the program immediately.
    closeAllChannels();
    quick_exit(0);
  }
}

void registerAbnormalExitHandler() {
  struct sigaction new_action, old_action;

  /* Set up the structure to specify the new action. */
  new_action.sa_handler = cleanUpAtAbnormalExit;
  sigemptyset(&new_action.sa_mask);
  new_action.sa_flags = 0;

  // Register signal handler for SIGINT and SIGTERM
  sigaction(SIGINT, &new_action, &old_action);
  sigaction(SIGTERM, &new_action, &old_action);
}


void flushBlockInQueues(bool terminate) {
  block_data * data;
  timespec start, end;

  // Flush the block in incoming queue and free it
  while (io_queue.pop(data)) {
    getTS(&start);
    if (data->compress) {
      writeCompressedBlock(data);
    } else {
      writeUncompressedBlock(data);
    }
    getTS(&end);
    storeBlockWritingTime(start, end);
  }

  // TDB: free the outgoing_queue and blocks

  storeBlockWritingTime(start, end, true);

  if (terminate) {
    quick_exit(0);
  }
}