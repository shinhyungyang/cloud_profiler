#include "time_tsc.h"
#include "cloud_profiler.h"
#include "globals.h"
#include "handlers/io_thread/io_thread.h"
#include <iostream>
#include <string>

#define MILLION 1000000
#define TEN_MILLION (MILLION * 10)
#define HUND_MILLION (MILLION * 100)
#define BILLION (MILLION * 1000)

int main(void) {
  // Do not create IO thread
  io_thread_created.exchange(true);

  // Open Channel with Buffer ID handler
  int64_t channel = openChannel("buffer_id_handler_channel", ASCII, BUFFER_IDENTITY);

  // Fill queue
  std::cout << "Filling Queue" << std::endl;
  for (int i = 0; i < HUND_MILLION; i++) {
    logTS(channel, i);
    if (i % 100000 == 0)
      std::cout << i << '\n';
  }

  std::string str;
  std::cout << "Press Enter to Perform IO Operation" << std::endl;
  std::cin >> str;

  // Perform IO operation
  io_test = true;
  runIOThread();
  return 0;
}