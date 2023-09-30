#ifndef __CLEANUP_H__
#define __CLEANUP_H__

#include <cstdlib>

// register a function using atexit().
// atexit() is called before normal termination of program.
// this means that when return 0 in main function
// or call of exit function in other thread
// would cause call to the registered function.
//
// Note that the thread that execute the termination of the program
// will run the registered function.
//
// Therefore, return 0 from main function will result in execution of
// registered function running in main thread.
// exit function not from main thread will result in execution of
// registered function running in that thread.
void cleanUpAtNormalExit();
// Callback function for cleanup at normal exit.
// Normal exit means program termination
// due to return statement in main function.
//
// 1) It closes all channels not to lose any log data.
// 2) Signal the compression thread or IO thread to stop.
// 3) Wait for IO thread finish flushing.

void registerAbnormalExitHandler();
// Register callback function for cleanup at abnormal exit.
// Abnormal exit means program termination due to SIGINT(Ctrl + C) or SIGTERM
// (the signal to terminate to program that Apache Storm gives).
//
// 1) It closes all channels not to lose any log data.
// 2) Signal the compression thread or IO thread to stop.

void flushBlockInQueues(bool terminate);
// Dequeue and write out the remaining block in queue to IO thread.
// After that, terminate the progream if needed.

#endif