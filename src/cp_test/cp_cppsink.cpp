#include "time_tsc.h"

#include "cloud_profiler.h"
#include "globals.h"
extern "C" {
#include "util.h"
#include "net.h"
}

#include "cxxopts.hpp"

#include <errno.h>        // for using nanosleep
#include <vector>
#include <thread>
#include <zmq.hpp>

#define MEBIBYTE              (1024 * 1024)
#define QUEUE_SIZE            (500 * 1000)
#define POLL_ENQUEUE_OVERHEAD (1900)
#define TCP_BUF_SIZE          (MEBIBYTE * 12)
#define VAL_ZMQ_POLLITEMS     (1)
#define VAL_ZMQ_IMMEDIATE     (1)

std::string sv_addresses;                   // server ip address
bool        isServer          = false;      // run as a server or a client
int64_t     nr_threads        = 1;          // number of threads

int recvFromZMQQueues(int argc, char ** argv);
int fetchInt(int argc, char ** argv);

int recvMultiThreaded(int argc, char ** argv);
void thread_f(int64_t ch, int tid);

int main (int argc, char ** argv) {
  recvMultiThreaded(argc, argv);
//recvFromZMQQueues(argc, argv);
//fetchInt(argc, argv);
}

int recvMultiThreaded(int argc, char ** argv) {

  int NR_SOCKETS;
  //
  // parse command-line options:
  cxxopts::Options options("cp_cppsink",
                           "C++Sink receives data from data generators");
  options.add_options()
  ("sv_addr",             "DG ip_address:port",
   cxxopts::value<std::string>())
  ("h,help",              "Usage information")
  ;

  options.parse(argc, argv);

  if (options.count("h")) {
    std::cout << options.help({"", "Group"}) << std::endl;
    exit(EXIT_FAILURE);
  }

  // Set up data to request configuration:

  if (options.count("sv_addr")) {
    sv_addresses = options["sv_addr"].as<std::string>();
  } else {
    sv_addresses = "127.0.0.1:9106";
  }

  std::vector<std::string> addresses = std::vector<std::string>();
  std::vector<int64_t> arr_ch = std::vector<int64_t>();

  std::string delimiter = ";";
  size_t pos = 0;
  while ((pos = sv_addresses.find(delimiter)) != std::string::npos) {
    addresses.push_back(sv_addresses.substr(0, pos));
    sv_addresses.erase(0, pos + delimiter.length());
  }

  nr_threads = addresses.size();
  NR_SOCKETS = nr_threads;
  std::thread tids[NR_SOCKETS];

  for (int i = 0; i<NR_SOCKETS; ++i) {
    int64_t ch = openZMQBktOptChannel(addresses[i]);
    arr_ch.push_back(ch);
  }

  for (int i = 0; i<NR_SOCKETS; ++i) {
    tids[i] = std::thread(thread_f, arr_ch[i], i);
  }

  for (int i = 0; i<NR_SOCKETS; ++i) {
    tids[i].join();
  }

  return 0;
}

void thread_f(int64_t ch, int tid) {
  int64_t ch_fpc = openChannel("CPPDGSink            FPC_10MS", ASCII,
        NET_CONF);
  int64_t ch_bid = openChannel("CPPDGSink            IDENTITY", ASCII,
        NET_CONF);

  while (true) {
    zmq::message_t zmqmsg;
    int64_t p_zmqmsg = reinterpret_cast<intptr_t>(&zmqmsg);
    char* zmqBytes = static_cast<char*>(fetchZMQStrNB3(ch, p_zmqmsg));
    if (nullptr == zmqBytes) {
      continue;
    }
    uint64_t idx = *(reinterpret_cast<uint64_t *>(zmqBytes));
    logTS(ch_bid, idx);
    logTS(ch_fpc, 0L);
  }
  closeChannel(ch_fpc);
  closeChannel(ch_bid);
}

int recvFromZMQQueues(int argc, char ** argv) {

  //
  // parse command-line options:
  cxxopts::Options options("cp_cppsink",
                           "C++Sink receives data from data generators");
  options.add_options()
  ("sv_addr",             "DG ip_address:port",
   cxxopts::value<std::string>())
  ("h,help",              "Usage information")
  ;

  options.parse(argc, argv);

  if (options.count("h")) {
    std::cout << options.help({"", "Group"}) << std::endl;
    exit(EXIT_FAILURE);
  }

  // Set up data to request configuration:

  if (options.count("sv_addr")) {
    sv_addresses = options["sv_addr"].as<std::string>();
  } else {
    sv_addresses = "127.0.0.1:9106";
  }

  std::vector<std::string> addresses = std::vector<std::string>();

  std::string delimiter = ";";
  size_t pos = 0;
  while ((pos = sv_addresses.find(delimiter)) != std::string::npos) {
    addresses.push_back(sv_addresses.substr(0, pos));
    sv_addresses.erase(0, pos + delimiter.length());
  }

  nr_threads = addresses.size();

  int64_t ch = openTCPChannel(false, addresses, nr_threads);

  while (true) {
    const char * fetched = recvStr(ch);
    delete fetched;
  }

  return 0;
}

int fetchInt(int argc, char ** argv) {

  //
  // parse command-line options:
  cxxopts::Options options("cp_cppsink",
                           "A new version that fetches integers.");
  options.add_options()
  ("h,help",              "Usage information")
  ;

  options.parse(argc, argv);

  if (options.count("h")) {
    std::cout << options.help({"", "Group"}) << std::endl;
    exit(EXIT_FAILURE);
  }

  int64_t cl_cpp = prepareFetchingTest();

  while (true) { fetchInt(cl_cpp); }

  return 0;
}
