# Light-weight, High-resolution Profiling of Data Streams

**cloud-profiler** is a native library for profiling tuples in data streams 
on the Cloud.

###### **News**
* [x] Sep 30, 2023: uploaded to GitHub
* [x] Jan 16, 2018: moved to CMake
* [x] Dec 9, 2017: added profiling for JVM GC overhead

## **cloud-profiler** in Java

**cloud-profiler** is a set of JNI bindings to C++ functions. Using the library
is as simple as the following code:

```java
// Open and parameterize a channel for the XoY handler
long ch0 = cloud_profiler.openChannel("XoY_2_1KiB",
                                      log_format.ASCII,
                                      handler_type.XOY);
if (-1 == cloud_profiler.parameterizeChannel(ch0, 0, 2)) {
  System.out.println("Error parameterizing channel 0, param 0");
}
if (-1 == cloud_profiler.parameterizeChannel(ch0, 1, "1KiB")) {
  System.out.println("Error parameterizing channel 0, param 1");
}

// Logging
cloud_profiler.logTS(ch0, tuple_id);

// Close a channel with a poison pill
cloud_profiler.logTS(ch0, -1);
```

## Prerequisites

To build the JNI interface, the SWIG tool is required from
[http://swig.org/](http://swig.org/)
It is recommended to install the newest version (4.0.0) from source, because currently available SWIG packages from Linux distributions such as Ubuntu are outdated.

**cloud_profiler** requires [GCC 7.1.0](http://ftp.gnu.org/gnu/gcc/gcc-7.1.0/gcc-7.1.0.tar.gz) or later for new features and C++11 memory model support.

[CMake 3.15.1](https://cmake.org/) or above is required for our building process.

**cloud-profiler** supports various configurations, which can be selected in the CMakeList.txt build
files. Such a build may require further packages: ZeroMQ, PAPI,
Boost >= version 1.66, Java and the Java JNI.

For GCC, CMake, ZeroMQ, BOOST and SWIG, please consider to use our [build scripts](https://git.elc.cs.yonsei.ac.kr/bburg/build-scripts) to compile and install recent versions on a Linux distribution. 

CMake can be told non-standard locations of packages by specifying the root-directory of a package
on the command-line. E.g., add<br/> 
```-DZMQ_ROOT=<path to your ZeroMQ installation>```. Following root variables are supported:
```BOOST_ROOT```, ```PAPI_ROOT``` and ```ZMQ_ROOT```.

## Build and test

To build the **cloud_profiler** in debug mode:
```sh
mkdir build_dbg
cd build_dbg
cmake ..
make
```

To build the **cloud_profiler** in release mode:
```sh
mkdir build_rel
cd build_rel
cmake ..
make
```

If the build cannot locate external libraries, CMake requires to include each
library path in the system variables. To use ```PKG_CONFIG_PATH```, please
install ```pkg-config``` on your system.

```sh
# install pkg-config on Ubuntu 18.04:
sudo apt install pkg-config
```

```sh
# System variables for ZeroMQ 4.3.0, SWIG 4.0.0, and Boost 1.69.0 in
# user-defined locations

BOOST_PATH="/opt/boost/1.69.0"
SWIG_PATH="/opt/swig/4.0.0"
ZMQ_PATH="/opt/zmq/4.3.0"

PATH=${PATH}:${SWIG_PATH}/bin
export LD_LIBRARY_PATH=${ZMQ_PATH}/lib:${BOOST_PATH}/lib:${LD_LIBRARY_PATH}

export PKG_CONFIG_PATH=${ZMQ_PATH}/lib/pkgconfig/:${PKG_CONFIG_PATH}

export ZMQ_LIBRARIES=${ZMQ_PATH}/lib
export ZMQ_INCLUDE_DIR=${ZMQ_PATH}/include

export Boost_ROOT=${BOOST_PATH}
```

To test the **cloud_profiler** with Java and C++:
```sh
cd build_dbg/bin
./cp_cpptest
bash ./jtest.sh
bash ./jkernels.sh
```

## Deployment

After the [Build and test](#build-and-test) steps, the shared library and Java wrapper files
have been build.  All deployable parts are in the jni subdirectory.
* `cloud_profiler.jar`: the Java wrapper class files
* `cp/native/libcloud_profiler.so`: the shared machine-code library from the
  C++ cloud profiler sources

See [**src/cp_test/cp_jtest.java**](http://git.elc.cs.yonsei.ac.kr/bburg/cloud_profiler/blob/master/src/cp_test/cp_jtest.java) for an example on how to use the profiler.

See the "jtest" target in the [**src/cp_test/Makefile**](http://git.elc.cs.yonsei.ac.kr/bburg/cloud_profiler/blob/master/src/cp_test/Makefile) on CLASSPATH settings etc.

## Using the cloud-profiler

* [**src/cp_test/cp_jtest.java**](http://git.elc.cs.yonsei.ac.kr/bburg/cloud_profiler/blob/master/src/cp_test/cp_jtest.java) shows how to load the library and call methods from
  the `cloud_profiler` Java wrapper class.
* [**src/cp/cloud_profiler.h**](http://git.elc.cs.yonsei.ac.kr/bburg/cloud_profiler/blob/master/src/cp/cloud_profiler.h) contains the interfaces and a short description of
  the functions call-able from Java.

## Files generated during a profiling run:

All files are generated in the `/tmp` directory of a node.

Two types of files are generated:
1. a status/error log file for each process of a node. 
2. a per-channel file containing profiled data of that channel.

### Status/error log files
The naming scheme is `/tmp/cloud_profiler_logfile:<IP>:<PID>.txt`
`<IP>` denotes the IP address of the node in dotted quad (e.g., 192.168.1.1) notation.
`<PID>` denotes the Linux process ID of the process that owned this log file.
If several Java threads are started as part of one Java program, those threads
will be light-weight processes of the same Java process and thus all share the
same PID.
If two JVMs are started, they will have different PIDs and thus use different
log-files. 

Per-channel profiling data: the naming scheme is
```sh
/tmp/cloud_profiler:<IP>:<PID>:<TID>:ch<number>:a:<ch_name>.txt
```

* `<IP>`: the IP address in dotted quad notation 
* `<PID>`: process ID 
* `<TID>`: thread ID (multiple Java/C++ etc. threads of a program are children 
of one process).
* `ch<number>`: the channels belonging to one PID are numbered from 0 .. N.
Numbers are allocated in the order the channels are opened. All threads of one
process are competing and there is no implied association of channel numbers to
threads.

[**src/cp_test/cp_jtest.java**](http://git.elc.cs.yonsei.ac.kr/bburg/cloud_profiler/blob/master/src/cp_test/cp_jtest.java) contains an example thread class that is logging using
two channels. 

## Channel handlers

Every channel is assigned a handler function of a specific type, which is
specified when the channel is opened.

### identity_handler

With every
call of `logTS(ch, tuple)`, a time-stamp will be taken and logged on channel `ch`,
tuple `tuple`. One line per `logTS()` call is generated in the underlying channel
log file.

### downsample_handler

The handler only captures and logs every *n*<sup>th</sup> tuple and its timestamp to 
efficiently analyze characteristics of target data streams
while reducing the sampling overhead.

### XoY_handler

It is initialized with two integers x and y. A tuple and time-stamp pair
is logged iff:

```
tuple_ID mod y < x
where y is a power of 2
```

The handler is particularly useful for reducing the underlying channel log file while
capable of tracing a set of tuples throughout the entire topology on the Cloud

### FirstLast_handler

Instrumenting the handler allows capturing and logging the first and last
tuple/time-stamp pairs
discarding all others in between. It is effective when measuring the accumulated
latency of an actor or a topology. A poison pill is required to log the last 
tuple/time-stamp pair before the channel is closed.
Consider using multiple FirstLast_handlers
in selected actors for different measurements.

### null_handler

It discards all incoming tuples. It is used to disable a channel to reduce sampling
overhead.

In `src/cp_test/` run `make jtest` and `ls -lrt /tmp/` and consider the generated
files for further information on the generated profiling data.

## Converting log files

`btoa_converter` is a tool that converts log files into ASCII format.

The following commmand converts all the log files in `/tmp` directory.

```sh
cd build_dbg/bin
./btoa_converter
```

There is a wrapper script for `btoa_converter` which can specify the name of the log files to convert.

It can be used as:

```sh
cd build_dbg/bin
bash btoa_file_converter.sh /tmp/cloud_profiler:127.0.0.1:1000:1000:ch0:z:buf.txt
bash btoa_file_converter.sh /tmp/cloud_profiler:127.0.0.1:1000:1000:ch0:z:buf.txt /tmp/cloud_profiler:127.0.0.1:1000:1000:ch1:z:buf.txt
bash btoa_file_converter.sh /tmp/cloud_profiler*\:b\:*.txt
bash btoa_file_converter.sh /tmp/cloud_profiler\:127.0.0.1\:1000\:100*
```
