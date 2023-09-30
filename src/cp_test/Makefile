VPATH=../cp:../cp/handlers
PAPI=/opt/papi/5.5.1
CC=gcc
CXX=g++
CFLAGS=-g -Wall --std=c11 -O3 #-flto
CXXFLAGS=-g -Wall --std=c++11 -O3
CPPFLAGS=-I../cp -I../cxxopts/include -I$(PAPI)/include
LDFLAGS=-L $(PAPI)/lib
LDLIBS=-lpapi -Wl,-rpath,$(PAPI)/lib -lpthread -lm
LINK.o = $(LINK.cc)
OUTPUT_OPTION=-MMD -MP -o $@
JNIPATH=$(realpath $(CURDIR)/../../jni)
SOPATH=$(JNIPATH)/cp/native
JOPTJAR=$(realpath $(CURDIR)/../../src/jopt_simple/jopt-simple-5.0.4.jar)

SRC=util.c net.c
OBJ=$(SRC:.c=.o)
DEP=$(SRC:.c=.d)

CXX_SRC=time.cpp log.cpp cp_cpptest.cpp closure.cpp channels.cpp \
	null_handler.cpp id_handler.cpp median_handler.cpp \
	downsample_handler.cpp XoY_handler.cpp firstlast_handler.cpp
CXX_OBJ=$(CXX_SRC:.cpp=.o)
CXX_DEP=$(CXX_SRC:.cpp=.d)

.PHONY: clean

ALLPROGS=typetest cp_cpptest

all: $(ALLPROGS)

jtest : $(CP_LIBS) cp_jtest.class
	java -Djava.library.path=$(JNIPATH)/cp/native/ \
	     -cp $(JNIPATH)/cloud_profiler.jar:$(JOPTJAR):. cp_jtest 

cp_jtest.class : cp_jtest.java
	javac -cp $(JNIPATH)/cloud_profiler.jar:$(JOPTJAR):. $< 

cpptest : cp_cpptest
	./$<

cp_cpptest : cp_cpptest.o closure.o channels.o time.o log.o util.o net.o \
	     null_handler.o id_handler.o median_handler.o \
	     downsample_handler.o XoY_handler.o firstlast_handler.o

cp_cpptest_dyn : cp_cpptest.o
	g++ -L $(SOPATH) -o $@ $^ -lcloud_profiler -Wl,-rpath,$(SOPATH)

ktest : $(CP_LIBS) kernels.class
	java -Djava.library.path=$(JNIPATH)/cp/native/ \
	     -cp $(JNIPATH)/cloud_profiler.jar:$(JOPTJAR):. kernels 

kernels.class : kernels.java
	javac -cp $(JNIPATH)/cloud_profiler.jar:$(JOPTJAR):. $< 

test: 
	@gcc -E typetest.c | grep __time_t

-include $(DEP)

clean:
	rm -f $(ALLPROGS) $(DEP) $(OBJ) $(CXX_DEP) $(CXX_OBJ) *.class
